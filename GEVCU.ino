/*
  GEVCU.ino
 
  Created: 1/4/2013 1:34:14 PM
   Author: Collin Kidder
 
   New plan of attack:
	For ease of development and to keep the code simple, devices supported are broken into categories (motor controller,
	bms, charger, display, misc). The number of supported devices is hard coded. That is, there can be one motor controller. It can
	be any motor controller but there is no support for two. The exception is misc which can have three devices. All hardware
	is to be subclassed from the proper master parent class for that form of hardware. That is, a motor controller will derive
	from the base motor controller class. This allows a standard interface, defined by that base class, to be used to access
	any hardware of that category. 
 */

#include <Arduino.h>
#include "variant.h"
#include <due_can.h>
#include <DueTimer.h>
#include <due_wire.h>
#include <due_rtc.h>
#include "mem_cache.h"
#include "throttle.h"
#include "pedal_pot.h"
#include "device.h"
#include "motorctrl.h"
#include "dmoc.h"
#include "timer.h"
#include "mem_cache.h"
#include "sys_io.h"

THROTTLE *throttle; 
MOTORCTRL* motorcontroller; //generic motor controller - instantiate some derived class to fill this out
RTC_clock rtc_clock(XTAL); //init RTC with the external 32k crystal as a reference
PREFHANDLER sysPrefs(EE_SYSTEM_START);

void printMenu();
void SerialEvent();

//Evil, global variables
bool runRamp = false;
bool runStatic = false;
bool runThrottle = false;
bool throttleDebug = false;
byte i=0;

RX_CAN_FRAME message;

void setup_due() {
  uint32_t temp;
  
    // Initialize CAN0 and CAN1, baudrate at 500Kb/s
  CAN.init(SystemCoreClock, CAN_BPS_500K);
  //CAN2.init(SystemCoreClock, CAN_BPS_500K);
  
  // Disable all CAN0 & CAN1 interrupts
  CAN.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
  //CAN2.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
  
  CAN.reset_all_mailbox();
  //CAN2.reset_all_mailbox();
  
  //Now, each canbus device has 8 mailboxes which can freely be assigned to either RX or TX.
  //The firmware does a lot of both so 5 boxes are for RX, 3 for TX.
  
  for(uint8_t count = 0; count < 5; count++) {
    CAN.mailbox_init(count);
    CAN.mailbox_set_mode(count, CAN_MB_RX_MODE);
    CAN.mailbox_set_accept_mask(count, 0x7F0, false);
  }
  //First three mailboxes listen for 0x23x frames, last two listen for 0x65x frames
  CAN.mailbox_set_id(0, 0x230, false); CAN.mailbox_set_id(1, 0x230, false); CAN.mailbox_set_id(2, 0x230, false);
  CAN.mailbox_set_id(3, 0x650, false); CAN.mailbox_set_id(4, 0x650, false);
  
  for(uint8_t count = 5; count < 8; count++) {
    CAN.mailbox_init(count);
    CAN.mailbox_set_mode(count, CAN_MB_TX_MODE);
    CAN.mailbox_set_priority(count, 10);
    CAN.mailbox_set_accept_mask(count, 0x7FF, false);
  }
  
  //Enable interrupts for the RX boxes. TX interrupts aren't wired up yet
  CAN.enable_interrupt(CAN_IER_MB0 | CAN_IER_MB1 | CAN_IER_MB2 | CAN_IER_MB3 | CAN_IER_MB4);
  
  NVIC_EnableIRQ(CAN0_IRQn); //tell the nested interrupt controller to turn on our interrupt
  
  Wire.begin();
  
  rtc_clock.init();
  //Now, we have no idea what the real time is but the EEPROM should have stored a time in the past.
  //It's better than nothing while we try to figure out the proper time.
  sysPrefs.Read(EESYS_RTC_TIME, &temp);
  rtc_clock.change_time(temp);
  sysPrefs.Read(EESYS_RTC_DATE, &temp);
  rtc_clock.change_date(temp);
  
  setup_sys_io(); //get calibration data for system IO
}

void setup() {
  
  SerialUSB.begin(115200);

  SerialUSB.println("GEVCU alpha 03-21-2013");

  setup_due();
  
  //The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
  //if min is less than max for a throttle then the pot goes low to high as pressed.
  //if max is less than min for a throttle then the pot goes high to low as pressed.

  throttle = new POT_THROTTLE(0,1); //specify the shield ADC ports to use for throttle 255 = not used (valid only for second value)
  POT_THROTTLE* pot = (POT_THROTTLE *)throttle;   //since throttle is of the generic base class type we have to cast to get access to
                                                        //the special functions of a pedal pot. Of course this must not be done in production.
  throttle->setupDevice();
        
  //This could eventually be configurable.
  setupTimer(10000); //10ms / 10000us ticks / 100Hz

  motorcontroller = new DMOC(&CAN); //instantiate a DMOC645 device controller as our motor controller
        
  motorcontroller->setupDevice();
        
  //This will not be hard coded soon. It should be a list of every hardware support module
  //compiled into the ROM
  //SerialUSB.println("Installed devices: DMOC645");

  SerialUSB.print("System Ready ");
  printMenu();

}

void printMenu() {
  SerialUSB.println("System Menu:");
  SerialUSB.println("D = disabled op state");
  SerialUSB.println("S = standby op state");
  SerialUSB.println("E = enabled op state");
  SerialUSB.println("n = neutral gear");
  SerialUSB.println("d = DRIVE gear");
  SerialUSB.println("r = reverse gear");
  SerialUSB.println("<space> = start/stop ramp test");
  SerialUSB.println("x = lock ramp at current value (toggle)");
  SerialUSB.println("t = Use accelerator pedal? (toggle)");
  SerialUSB.println("L = output raw input values (toggle)");
  SerialUSB.println("K = set all outputs high");
  SerialUSB.println("J = set all outputs low");
  SerialUSB.println("Y,U,I = test EEPROM routines");
  SerialUSB.println("");
}


//Note that the loop uses the motorcontroller object which is of the MOTORCTRL class. This allows
//the loop to be generic while still supporting a variety of hardware. Let's try to keep it this way.
void loop() {
  static byte dotTick = 0;
  static byte throttleval = 0;
  static byte count = 0;
  uint16_t adcval;
  if (CAN.rx_avail()) {
    CAN.get_rx_buff(&message);
    motorcontroller->handleFrame(message);
  }
  if (SerialUSB.available()) serialEvent(); //due doesnt have int driven serial yet
  if (tickReady) {
    if (dotTick == 0) SerialUSB.print('.'); //print . every 256 ticks (2.56 seconds)
    dotTick = dotTick + 1;
    tickReady = false;
    //do tick related stuff
    
    MemCache.handleTick();
    
    throttle->handleTick(); //gets ADC values, calculates throttle position
    //SerialUSB.println(Throttle.getThrottle());
   count++;
   if (count > 50) {
     count = 0;
     if (!runStatic) throttleval++;
     if (throttleval > 80) throttleval = 0;
     if (throttleDebug) {
       adcval = getAnalog(0);
       SerialUSB.print("A0: ");
       SerialUSB.print(adcval);
       adcval = getAnalog(1);
       SerialUSB.print("A1: ");
       SerialUSB.print(adcval);
       adcval = getAnalog(2);
       SerialUSB.print("A2: ");
       SerialUSB.print(adcval);
       adcval = getAnalog(3);
       SerialUSB.print("A3: ");
       SerialUSB.println(adcval);
     }
   }
   if (!runThrottle) { //ramping test      
      if (!runRamp) {
        throttleval = 0;
      }
      motorcontroller->setThrottle(throttleval * (int)12); //with throttle 0-80 this sets throttle to 0 - 960
    } 
    else { //use the installed throttle
      motorcontroller->setThrottle(throttle->getThrottle());
      //SerialUSB.println(throttle.getThrottle());  //just for debugging
    }
    motorcontroller->handleTick(); //intentionally far down here so that the throttle is set before this is called
  }
}


/*Single single character interpreter of commands over
serial connection. There is a help menu (press H or h or ?)

This function casts the motorcontroller object to DMOC which breaks the
proper generic interface but is necessary for testing. 
TODO: This all has to eventually go away.
*/
void serialEvent() {
  int incoming;
  DMOC* dmoc = (DMOC*)motorcontroller;
  incoming = SerialUSB.read();
  if (incoming == -1) return;
  switch (incoming) {
    case 'h':
    case '?':
    case 'H':
      printMenu();
      break;
    case ' ':
      runRamp = !runRamp;
        if (runRamp) {
	  SerialUSB.println("Start Ramp Test");
          dmoc->setPowerMode(DMOC::MODE_RPM);
        }
	else {
          SerialUSB.println("End Ramp Test");
          dmoc->setPowerMode(DMOC::MODE_TORQUE);
        }
	break;
    case 'd':
      dmoc->setGear(DMOC::DRIVE);
      SerialUSB.println("forward");
      break;
    case 'n':
      dmoc->setGear(DMOC::NEUTRAL);
      SerialUSB.println("neutral");
      break;
    case 'r':
      dmoc->setGear(DMOC::REVERSE);
      SerialUSB.println("reverse");
      break;
    case 'D':
      dmoc->setOpState(DMOC::DISABLED);
      SerialUSB.println("disabled");
      break;
    case 'S':
      dmoc->setOpState(DMOC::STANDBY);
      SerialUSB.println("standby");
      break;
    case 'E':
      dmoc->setOpState(DMOC::ENABLE);
      SerialUSB.println("enabled");
      break;
    case 'x':
      runStatic = !runStatic;
      if (runStatic) {
        SerialUSB.println("Lock RPM rate");
      }
      else SerialUSB.println("Unlock RPM rate");
      break;
    case 't':
      runThrottle = !runThrottle;
      if (runThrottle) {
        SerialUSB.println("Use Throttle Pedal");
        dmoc->setPowerMode(DMOC::MODE_TORQUE);
      }
      else SerialUSB.println("Ignore throttle pedal");
      break;
    case 'L':
      throttleDebug = !throttleDebug;
      if (throttleDebug) {
        SerialUSB.println("Output raw throttle");
      }
      else SerialUSB.println("Cease raw throttle output");
      break;
    case 'Y':
      SerialUSB.println("Trying to save 0x45 to eeprom location 10");
      uint8_t temp;
      MemCache.Write(10, (uint8_t) 0x45);
      MemCache.Read(10, &temp);
      SerialUSB.print("Got back value of ");
      SerialUSB.println(temp);      
      break;
    case 'U':
      SerialUSB.println("Adding a sequence of values from 0 to 255 into eeprom");
      for (int i = 0; i<256; i++) MemCache.Write(1000+i,(uint8_t)i);
      MemCache.FlushAllPages(); //write everything to eeprom
      MemCache.InvalidateAll(); //remove all data from cache
      break;
    case 'I':
      SerialUSB.println("Retrieving data previously saved");
      uint8_t val;
      for (int i = 0; i < 256; i++) {
        MemCache.Read(1000 + i, &val);
        SerialUSB.print(val);
        SerialUSB.print(" ");
      }
      break;
    case 'K': //set all outputs high
      setOutput(0, true);
      setOutput(1, true);
      setOutput(2, true);
      setOutput(3, true);
      break;
    case 'J': //set the four outputs low
      setOutput(0, false);
      setOutput(1, false);
      setOutput(2, false);
      setOutput(3, false);    
      break;
  }
}
