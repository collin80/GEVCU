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


  Starting a conversion of source to work for Due as well. Issues:
  -Due library uses two different structures and they aren't the same as the Frame structure from MCP2515 library. Need to reconcile them.
  -Timer code is very hardware specific. Has to be totally redone for Due
  -Due has no EEPROM and must emulate it with a wrapper library.
 */

#include <Arduino.h>

#if defined(__SAM3X8E__)
  #include "variant.h"
  #include <CAN.h>
  #include <DueTimer.h>
  #include <Wire.h>
#else
  #include <SPI.h>
  #include "MCP2515.h"
#endif

#include "mem_cache.h"
#include "throttle.h"
#include "pedal_pot.h"
#include "device.h"
#include "motorctrl.h"
#include "dmoc.h"
#include "timer.h"
#include "mem_cache.h"


#if defined(__SAM3X8E__)
#else
  // Pin definitions specific to how the MCP2515 is wired up.
  #define CS_PIN    85
  #define RESET_PIN  7
  #define INT_PIN    84
  // Create CAN object with pins as defined
  MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);
#endif

THROTTLE *throttle; 
MOTORCTRL* motorcontroller; //generic motor controller - instantiate some derived class to fill this out

void printMenu();
void serialEvent();

//Evil, global variables
bool runRamp = false;
bool runStatic = false;
bool runThrottle = false;
bool throttleDebug = false;
byte i=0;
#ifdef __SAM3X8E__
  RX_CAN_FRAME message;
#else
  Frame message;
#endif

#ifndef __SAM3X8E__
  void CANHandler() {
    CAN.intHandler();
  }
#endif

#ifndef __SAM3X8E__
void setup_avr() {
  // Set up SPI Communication
  // dataMode can be SPI_MODE0 or SPI_MODE3 only for MCP2515
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
  SPI.begin();

  // Initialize MCP2515 CAN controller at the specified speed and clock frequency
  // (Note:  This is the oscillator attached to the MCP2515, not the Arduino oscillator)
  //speed in KHz, clock in MHz
  if(CAN.Init(500,16))   //DMOC defaults to 500Khz
  {
    Serial.println("MCP2515 Init OK ...");
  } else {
    Serial.println("MCP2515 Init Failed ...");
  }

  CAN.InitFilters(false);

  //Setup CANBUS comm to allow DMOC command and status messages through
  CAN.SetRXMask(MASK0, 0x7F0, 0); //match all but bottom four bits, use standard frames
  CAN.SetRXFilter(FILTER0, 0x230, 0); //allows 0x230 - 0x23F
  CAN.SetRXFilter(FILTER1, 0x650, 0); //allows 0x650 - 0x65F
}
#endif

#ifdef __SAM3X8E__
void setup_due() {
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
  CAN.mailbox_set_id(0, 0x230, false); CAN2.mailbox_set_id(1, 0x230, false); CAN2.mailbox_set_id(2, 0x230, false);
  CAN.mailbox_set_id(3, 0x650, false); CAN2.mailbox_set_id(4, 0x650, false);
  
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

}
#endif

void setup() {
  
  Serial.begin(115200);

  Serial.println("GEVCU alpha 02-25-2013");

#ifdef __SAM3X8E__
  setup_due();
#else
  setup_avr();
#endif  

  //The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
  //if min is less than max for a throttle then the pot goes low to high as pressed.
  //if max is less than min for a throttle then the pot goes high to low as pressed.

  throttle = new POT_THROTTLE(0,1); //specify the ADC ports to use for throttle 255 = not used (valid only for second value)
  POT_THROTTLE* pot = (POT_THROTTLE *)throttle;   //since throttle is of the generic base class type we have to cast to get access to
                                                        //the special functions of a pedal pot. Of course this must not be done in production.
  throttle->setupDevice();
        
  //This could eventually be configurable.
  setupTimer(10000); //10ms / 10000us ticks / 100Hz

  motorcontroller = new DMOC(&CAN); //instantiate a DMOC645 device controller as our motor controller
        
  motorcontroller->setupDevice();
        
  //This will not be hard coded soon. It should be a list of every hardware support module
  //compiled into the ROM
  //Serial.println("Installed devices: DMOC645");
#ifndef  __SAM3X8E__
  attachInterrupt(6, CANHandler, FALLING);
#endif

  Serial.print("System Ready ");
  printMenu();

}

void printMenu() {
  Serial.println("System Menu:");
  Serial.println("D = disabled op state");
  Serial.println("S = standby op state");
  Serial.println("E = enabled op state");
  Serial.println("n = neutral gear");
  Serial.println("d = DRIVE gear");
  Serial.println("r = reverse gear");
  Serial.println("<space> = start/stop ramp test");
  Serial.println("x = lock ramp at current value (toggle)");
  Serial.println("t = Use accelerator pedal? (toggle)");
  Serial.println("L = output raw throttle values (toggle)");
  Serial.println("Y = test EEPROM routines");
  Serial.println("");
}


//Note that the loop uses the motorcontroller object which is of the MOTORCTRL class. This allows
//the loop to be generic while still supporting a variety of hardware. Let's try to keep it this way.
void loop() {
  static byte dotTick = 0;
  static byte throttleval = 0;
  static byte count = 0;
#ifdef __SAM3X8E__
  if (CAN.rx_avail()) {
    CAN.get_rx_buff(&message);
    motorcontroller->handleFrame(message);
  }
  if (Serial.available()) serialEvent(); //due doesnt have int driven serial yet
#else
  if (CAN.GetRXFrame(message)) {
    motorcontroller->handleFrame(message);
  }
#endif
  if (tickReady) {
    if (dotTick == 0) Serial.print('.'); //print . every 256 ticks (2.56 seconds)
    dotTick = dotTick + 1;
    tickReady = false;
    //do tick related stuff
    
    MemCache.handleTick();
    
    throttle->handleTick(); //gets ADC values, calculates throttle position
    //Serial.println(Throttle.getThrottle());
   count++;
   if (count > 50) {
     count = 0;
     if (!runStatic) throttleval++;
     if (throttleval > 80) throttleval = 0;
     if (throttleDebug) {
       POT_THROTTLE *pot = (POT_THROTTLE *)throttle;
	Serial.print("T1: ");
	Serial.print(pot->getRawThrottle1());
	Serial.print(" T2: ");
	Serial.print(pot->getRawThrottle2());
        Serial.print(" Out: ");
        Serial.println(pot->getThrottle());
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
      //Serial.println(throttle.getThrottle());  //just for debugging
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
  incoming = Serial.read();
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
	  Serial.println("Start Ramp Test");
          dmoc->setPowerMode(DMOC::MODE_RPM);
        }
	else {
          Serial.println("End Ramp Test");
          dmoc->setPowerMode(DMOC::MODE_TORQUE);
        }
	break;
    case 'd':
      dmoc->setGear(DMOC::DRIVE);
      Serial.println("forward");
      break;
    case 'n':
      dmoc->setGear(DMOC::NEUTRAL);
      Serial.println("neutral");
      break;
    case 'r':
      dmoc->setGear(DMOC::REVERSE);
      Serial.println("reverse");
      break;
    case 'D':
      dmoc->setOpState(DMOC::DISABLED);
      Serial.println("disabled");
      break;
    case 'S':
      dmoc->setOpState(DMOC::STANDBY);
      Serial.println("standby");
      break;
    case 'E':
      dmoc->setOpState(DMOC::ENABLE);
      Serial.println("enabled");
      break;
    case 'x':
      runStatic = !runStatic;
      if (runStatic) {
        Serial.println("Lock RPM rate");
      }
      else Serial.println("Unlock RPM rate");
      break;
    case 't':
      runThrottle = !runThrottle;
      if (runThrottle) {
        Serial.println("Use Throttle Pedal");
        dmoc->setPowerMode(DMOC::MODE_TORQUE);
      }
      else Serial.println("Ignore throttle pedal");
      break;
    case 'L':
      throttleDebug = !throttleDebug;
      if (throttleDebug) {
        Serial.println("Output raw throttle");
      }
      else Serial.println("Cease raw throttle output");
      break;
    case 'Y':
      Serial.println("Trying to save 0x45 to eeprom location 10");
      uint8_t temp;
      MemCache.Write(10, (uint8_t) 0x45);
      MemCache.Read(10, &temp);
      Serial.print("Got back value of ");
      Serial.println(temp);      
      break;
    }
}
