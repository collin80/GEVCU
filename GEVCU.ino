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

#include "GEVCU.h"

// The following includes are required in the .ino file by the Arduino IDE in order to properly
// identify the required libraries for the build.
#ifdef __arm__ // Arduino Due specific implementation
#include <due_rtc.h>
#include <due_can.h>
#include <due_wire.h>
#include <DueTimer.h>
#elif defined(__AVR__) // Machina specific implementation
#include <EEPROM.h>
#include <SPI.h>
#include <MCP2515.h>
#endif
#ifdef CFG_LCD_MONITOR_ENABLED
#include <LiquidCrystal.h>
#endif

#ifdef __arm__ // Arduino Due specific implementation
//RTC_clock rtc_clock(XTAL); //init RTC with the external 32k crystal as a reference
#endif

THROTTLE *accelerator; 
THROTTLE *brake; 
MOTORCTRL* motorcontroller; //generic motor controller - instantiate some derived class to fill this out
PREFHANDLER sysPrefs(EE_SYSTEM_START);
CANHandler *canbus;

#ifdef CFG_LCD_MONITOR_ENABLED
LiquidCrystal lcd(CFG_LCD_MONITOR_PINS);
#endif

//Evil, global variables
bool runRamp = false;
bool runStatic = false;
bool runThrottle = false;
bool throttleDebug = false;
byte i=0;


//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void initSysEEPROM() {
  //three temporary storage places to make saving to EEPROM easy
  uint8_t eight;
  uint16_t sixteen;
  uint32_t thirtytwo; 

  eight = SYSTEM_DUE;
  sysPrefs.write(EESYS_SYSTEM_TYPE, eight);
  
  sixteen = 1024; //no gain
  sysPrefs.write(EESYS_ADC0_GAIN, sixteen);
  sysPrefs.write(EESYS_ADC1_GAIN, sixteen);
  sysPrefs.write(EESYS_ADC2_GAIN, sixteen);
  sysPrefs.write(EESYS_ADC3_GAIN, sixteen);
  
  sixteen = 0; //no offset
  sysPrefs.write(EESYS_ADC0_OFFSET, sixteen);
  sysPrefs.write(EESYS_ADC1_OFFSET, sixteen);
  sysPrefs.write(EESYS_ADC2_OFFSET, sixteen);
  sysPrefs.write(EESYS_ADC3_OFFSET, sixteen);

  sixteen = 500; //multiplied by 1000 so 500k baud
  sysPrefs.write(EESYS_CAN0_BAUD, sixteen);
  sysPrefs.write(EESYS_CAN1_BAUD, sixteen);
  
  sixteen = 11520; //multiplied by 10
  sysPrefs.write(EESYS_SERUSB_BAUD, sixteen);
  
  sixteen = 100; //multiplied by 1000
  sysPrefs.write(EESYS_TWI_BAUD, sixteen);

  sixteen = 100; //number of ticks per second
  sysPrefs.write(EESYS_TICK_RATE, sixteen);

  thirtytwo = 0;
  sysPrefs.write(EESYS_RTC_TIME, thirtytwo);
  sysPrefs.write(EESYS_RTC_DATE, thirtytwo);
  
  eight = 5; //how many RX mailboxes
  sysPrefs.write(EESYS_CAN_RX_COUNT, eight);
  
  thirtytwo = 0x7f0; //standard frame, ignore bottom 4 bits
  sysPrefs.write(EESYS_CAN_MASK0, thirtytwo);
  sysPrefs.write(EESYS_CAN_MASK1, thirtytwo);
  sysPrefs.write(EESYS_CAN_MASK2, thirtytwo);
  sysPrefs.write(EESYS_CAN_MASK3, thirtytwo);
  sysPrefs.write(EESYS_CAN_MASK4, thirtytwo);
  
  thirtytwo = 0x230;
  sysPrefs.write(EESYS_CAN_FILTER0, thirtytwo);
  sysPrefs.write(EESYS_CAN_FILTER1, thirtytwo);
  sysPrefs.write(EESYS_CAN_FILTER2, thirtytwo);
  
  thirtytwo = 0x650;
  sysPrefs.write(EESYS_CAN_FILTER3, thirtytwo);
  sysPrefs.write(EESYS_CAN_FILTER4, thirtytwo);
  
  thirtytwo = 0; //ok, not technically 32 bytes but the four zeros still shows it is unused.
  sysPrefs.write(EESYS_WIFI0_SSID, thirtytwo);
  sysPrefs.write(EESYS_WIFI1_SSID, thirtytwo);
  sysPrefs.write(EESYS_WIFI2_SSID, thirtytwo);
  sysPrefs.write(EESYS_WIFIX_SSID, thirtytwo);

  eight = 0; //no channel, DHCP off, B mode
  sysPrefs.write(EESYS_WIFI0_CHAN, eight);
  sysPrefs.write(EESYS_WIFI0_DHCP, eight);
  sysPrefs.write(EESYS_WIFI0_MODE, eight);

  sysPrefs.write(EESYS_WIFI1_CHAN, eight);
  sysPrefs.write(EESYS_WIFI1_DHCP, eight);
  sysPrefs.write(EESYS_WIFI1_MODE, eight);

  sysPrefs.write(EESYS_WIFI2_CHAN, eight);
  sysPrefs.write(EESYS_WIFI2_DHCP, eight);
  sysPrefs.write(EESYS_WIFI2_MODE, eight);

  sysPrefs.write(EESYS_WIFIX_CHAN, eight);
  sysPrefs.write(EESYS_WIFIX_DHCP, eight);
  sysPrefs.write(EESYS_WIFIX_MODE, eight);
  
  thirtytwo = 0;
  sysPrefs.write(EESYS_WIFI0_IPADDR, thirtytwo);
  sysPrefs.write(EESYS_WIFI1_IPADDR, thirtytwo);
  sysPrefs.write(EESYS_WIFI2_IPADDR, thirtytwo);
  sysPrefs.write(EESYS_WIFIX_IPADDR, thirtytwo);

  sysPrefs.write(EESYS_WIFI0_KEY, thirtytwo);
  sysPrefs.write(EESYS_WIFI1_KEY, thirtytwo);
  sysPrefs.write(EESYS_WIFI2_KEY, thirtytwo);
  sysPrefs.write(EESYS_WIFIX_KEY, thirtytwo);
  
  sysPrefs.saveChecksum();
}


void setup() {
  
  pinMode(BLINKLED, OUTPUT);
  digitalWrite(BLINKLED, LOW);
  
  SerialUSB.begin(115200);

  SerialUSB.println("GEVCU alpha 05-05-2013");

  canbus = new CANHandler();
  
   motorcontroller = new DMOC(canbus); //instantiate a DMOC645 device controller as our motor controller      
   motorcontroller->handleTick();
  
#ifdef __arm__ // Arduino Due specific implementation
    Wire.begin();

    SerialUSB.println("TWI INIT OK");

    if (!sysPrefs.checksumValid()) { //checksum is good, read in the values stored in EEPROM
        initSysEEPROM();
    }
    
    motorcontroller->handleTick();
    //rtc_clock.init();
    //Now, we have no idea what the real time is but the EEPROM should have stored a time in the past.
    //It's better than nothing while we try to figure out the proper time.
    /*
    uint32_t temp;
    sysPrefs.read(EESYS_RTC_TIME, &temp);
    rtc_clock.change_time(temp);
    sysPrefs.read(EESYS_RTC_DATE, &temp);
    rtc_clock.change_date(temp);
	 
    SerialUSB.println("RTC INIT OK");
    */
#endif

    motorcontroller->setupDevice();
   
    setup_sys_io(); //get calibration data for system IO
    SerialUSB.println("SYSIO INIT OK");

#ifdef CFG_LCD_MONITOR_ENABLED
      lcd.begin(CFG_LCD_MONITOR_COLUMNS, CFG_LCD_MONITOR_ROWS);
      lcd.print("GEVCU is running");
#endif

    //The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
    //if min is less than max for a throttle then the pot goes low to high as pressed.
    //if max is less than min for a throttle then the pot goes high to low as pressed.

    accelerator = new POT_THROTTLE(0,255, true); //specify the shield ADC ports to use for throttle 255 = not used (valid only for second value)

    brake = new POT_THROTTLE(2, 255, false); //set up the brake input as the third ADC input from the shield.
  
    accelerator->setupDevice();
    brake->setupDevice();
    
    motorcontroller->handleTick();
        
    //This could eventually be configurable.
    setupTimer(10000); //10ms / 10000us ticks / 100Hz
        
    //This will not be hard coded soon. It should be a list of every hardware support module
    //compiled into the ROM
    //Serial.println("Installed devices: DMOC645");

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
  SerialUSB.println("U,I = test EEPROM routines");
  SerialUSB.println("");
}


//Note that the loop uses the motorcontroller object which is of the MOTORCTRL class. This allows
//the loop to be generic while still supporting a variety of hardware. Let's try to keep it this way.
void loop() {
  static CANFrame message;
  static byte dotTick = 0;
  static byte throttleval = 0;
  static byte count = 0;
  static bool LED = false;
  uint16_t adcval;
  
  sys_io_adc_poll();

  if (canbus->readFrame(message)) {
    motorcontroller->handleFrame(message);
  }

  //if the first digital input is high we'll enable drive so we can go!
  if (getDigital(0)) {
    ((DMOC *)motorcontroller)->setGear(DMOC::DRIVE);
    runThrottle = true;
    ((DMOC *)motorcontroller)->setPowerMode(DMOC::MODE_TORQUE);
  }
  
  //but, if the second input is high we cancel the whole thing and disable the drive.
  if (getDigital(1) || !getDigital(0)) {
    ((DMOC *)motorcontroller)->setOpState(DMOC::DISABLED);
    runThrottle = false;
  }
  
  if (SerialUSB.available()) serialEvent(); //due doesnt have int driven serial yet
  if (tickReady) {
    if (dotTick == 0) {
      SerialUSB.print('.'); //print . every 256 ticks (2.56 seconds)
      if (LED) {
        digitalWrite(BLINKLED, HIGH);
      }
      else {
        digitalWrite(BLINKLED, LOW);
      }
      LED = !LED;
    }
    dotTick = dotTick + 1;
    tickReady = false;
    //do tick related stuff

#ifdef __arm__ // Arduino Due specific implementation
    MemCache.handleTick();
#endif

#ifdef CFG_LCD_MONITOR_ENABLED
    lcd.setCursor(0, 1);
    lcd.print(count);
#endif

    accelerator->handleTick(); //gets ADC values, calculates throttle position
    brake->handleTick();
    //Serial.println(Throttle.getThrottle());
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
       SerialUSB.print(" A1: ");
       SerialUSB.print(adcval);
       adcval = getAnalog(2);
       SerialUSB.print(" A2: ");
       SerialUSB.print(adcval);
       adcval = getAnalog(3);
       SerialUSB.print(" A3: ");
       SerialUSB.print(adcval);
       if (getDigital(0)) SerialUSB.print(" D0: HIGH");
         else SerialUSB.print(" D0: LOW");
       if (getDigital(1)) SerialUSB.print(" D1: HIGH");
         else SerialUSB.print(" D1: LOW");
       if (getDigital(2)) SerialUSB.print(" D2: HIGH");
         else SerialUSB.print(" D2: LOW");
       if (getDigital(3)) SerialUSB.print(" D3: HIGH");
         else SerialUSB.print(" D3: LOW");
         
       int throttlepos = accelerator->getThrottle();
       if (brake->getThrottle() != 0) throttlepos = brake->getThrottle();       
       SerialUSB.print("  A:");
       SerialUSB.print(throttlepos);
       SerialUSB.println("");
     }
   }
   if (!runThrottle) { //ramping test      
      if (!runRamp) {
        throttleval = 0;
      }
      motorcontroller->setThrottle(throttleval * (int)12); //with throttle 0-80 this sets throttle to 0 - 960
    } 
    else { //use the installed throttle
	  int throttlepos = accelerator->getThrottle();
	  if (brake->getThrottle() != 0) { //if the brake has been pressed it overrides the accelerator.
            throttlepos = brake->getThrottle();
	  }
      motorcontroller->setThrottle(throttlepos);
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
  static int state = 0;
  DMOC* dmoc = (DMOC*)motorcontroller;
  incoming = SerialUSB.read();
  if (incoming == -1) return;
 if (state == 0) {
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
      else {
        SerialUSB.println("Ignore throttle pedal");
        dmoc->setPowerMode(DMOC::MODE_RPM);
      }
      break;
    case 'L':
      throttleDebug = !throttleDebug;
      if (throttleDebug) {
        SerialUSB.println("Output raw throttle");
      }
      else SerialUSB.println("Cease raw throttle output");
      break;
#ifdef __arm__ // Arduino Due specific implementation
    case 'U':
      SerialUSB.println("Adding a sequence of values from 0 to 255 into eeprom");
      for (int i = 0; i<256; i++) MemCache.Write(1000+i,(uint8_t)i);
      SerialUSB.println("Flushing cache");
      MemCache.FlushAllPages(); //write everything to eeprom
      MemCache.InvalidateAll(); //remove all data from cache
      SerialUSB.println("Operation complete.");
      break;
    case 'I':
      SerialUSB.println("Retrieving data previously saved");
      uint8_t val;
      for (int i = 0; i < 256; i++) {
        MemCache.Read(1000 + i, &val);
        SerialUSB.print(val);
        SerialUSB.print(" ");
      }
      SerialUSB.println("");
      break;
#endif
    case 'K': //set all outputs high
      setOutput(0, true);
      setOutput(1, true);
      setOutput(2, true);
      setOutput(3, true);
      SerialUSB.println("Setting all outputs ON");
      break;
    case 'J': //set the four outputs low
      setOutput(0, false);
      setOutput(1, false);
      setOutput(2, false);
      setOutput(3, false);    
      SerialUSB.println("Setting all outputs OFF");      
      break;
  }
 }
 
}
