/*
 GEVCU.ino
 
 Created: 1/4/2013 1:34:14 PM
 Author: Collin Kidder
 
 New, new plan: Allow for an arbitrary # of devices that can have both tick and canbus handlers. These devices register themselves
 into the handler framework and specify which sort of device they are. They can have custom tick intervals and custom can filters.
 The system automatically manages when to call the tick handlers and automatically filters canbus and sends frames to the devices.
 There is a facility to send data between devices by targetting a certain type of device. For instance, a motor controller
 can ask for any throttles and then retrieve the current throttle position from them.

Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include "GEVCU.h"

// The following includes are required in the .ino file by the Arduino IDE in order to properly
// identify the required libraries for the build.
#include <due_rtc.h>
#include <due_can.h>
#include <due_wire.h>
#include <DueTimer.h>

//RTC_clock rtc_clock(XTAL); //init RTC with the external 32k crystal as a reference

//Evil, global variables
CanHandler *canHandlerEV;
CanHandler *canHandlerCar;
PrefHandler *sysPrefs;
MemCache *memCache;
Heartbeat *heartbeat;
SerialConsole * serialConsole;

byte i = 0;

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void initSysEEPROM() {
	//three temporary storage places to make saving to EEPROM easy
	uint8_t eight;
	uint16_t sixteen;
	uint32_t thirtytwo;

	eight = SYSTEM_DUE;
	sysPrefs->write(EESYS_SYSTEM_TYPE, eight);

	sixteen = 1024; //no gain
	sysPrefs->write(EESYS_ADC0_GAIN, sixteen);
	sysPrefs->write(EESYS_ADC1_GAIN, sixteen);
	sysPrefs->write(EESYS_ADC2_GAIN, sixteen);
	sysPrefs->write(EESYS_ADC3_GAIN, sixteen);

	sixteen = 0; //no offset
	sysPrefs->write(EESYS_ADC0_OFFSET, sixteen);
	sysPrefs->write(EESYS_ADC1_OFFSET, sixteen);
	sysPrefs->write(EESYS_ADC2_OFFSET, sixteen);
	sysPrefs->write(EESYS_ADC3_OFFSET, sixteen);

	sixteen = 500; //multiplied by 1000 so 500k baud
	sysPrefs->write(EESYS_CAN0_BAUD, sixteen);
	sysPrefs->write(EESYS_CAN1_BAUD, sixteen);

	sixteen = 11520; //multiplied by 10
	sysPrefs->write(EESYS_SERUSB_BAUD, sixteen);

	sixteen = 100; //multiplied by 1000
	sysPrefs->write(EESYS_TWI_BAUD, sixteen);

	sixteen = 100; //number of ticks per second
	sysPrefs->write(EESYS_TICK_RATE, sixteen);

	thirtytwo = 0;
	sysPrefs->write(EESYS_RTC_TIME, thirtytwo);
	sysPrefs->write(EESYS_RTC_DATE, thirtytwo);

	eight = 5; //how many RX mailboxes
	sysPrefs->write(EESYS_CAN_RX_COUNT, eight);

	thirtytwo = 0x7f0; //standard frame, ignore bottom 4 bits
	sysPrefs->write(EESYS_CAN_MASK0, thirtytwo);
	sysPrefs->write(EESYS_CAN_MASK1, thirtytwo);
	sysPrefs->write(EESYS_CAN_MASK2, thirtytwo);
	sysPrefs->write(EESYS_CAN_MASK3, thirtytwo);
	sysPrefs->write(EESYS_CAN_MASK4, thirtytwo);

	thirtytwo = 0x230;
	sysPrefs->write(EESYS_CAN_FILTER0, thirtytwo);
	sysPrefs->write(EESYS_CAN_FILTER1, thirtytwo);
	sysPrefs->write(EESYS_CAN_FILTER2, thirtytwo);

	thirtytwo = 0x650;
	sysPrefs->write(EESYS_CAN_FILTER3, thirtytwo);
	sysPrefs->write(EESYS_CAN_FILTER4, thirtytwo);

	thirtytwo = 0; //ok, not technically 32 bytes but the four zeros still shows it is unused.
	sysPrefs->write(EESYS_WIFI0_SSID, thirtytwo);
	sysPrefs->write(EESYS_WIFI1_SSID, thirtytwo);
	sysPrefs->write(EESYS_WIFI2_SSID, thirtytwo);
	sysPrefs->write(EESYS_WIFIX_SSID, thirtytwo);

	eight = 0; //no channel, DHCP off, B mode
	sysPrefs->write(EESYS_WIFI0_CHAN, eight);
	sysPrefs->write(EESYS_WIFI0_DHCP, eight);
	sysPrefs->write(EESYS_WIFI0_MODE, eight);

	sysPrefs->write(EESYS_WIFI1_CHAN, eight);
	sysPrefs->write(EESYS_WIFI1_DHCP, eight);
	sysPrefs->write(EESYS_WIFI1_MODE, eight);

	sysPrefs->write(EESYS_WIFI2_CHAN, eight);
	sysPrefs->write(EESYS_WIFI2_DHCP, eight);
	sysPrefs->write(EESYS_WIFI2_MODE, eight);

	sysPrefs->write(EESYS_WIFIX_CHAN, eight);
	sysPrefs->write(EESYS_WIFIX_DHCP, eight);
	sysPrefs->write(EESYS_WIFIX_MODE, eight);

	thirtytwo = 0;
	sysPrefs->write(EESYS_WIFI0_IPADDR, thirtytwo);
	sysPrefs->write(EESYS_WIFI1_IPADDR, thirtytwo);
	sysPrefs->write(EESYS_WIFI2_IPADDR, thirtytwo);
	sysPrefs->write(EESYS_WIFIX_IPADDR, thirtytwo);

	sysPrefs->write(EESYS_WIFI0_KEY, thirtytwo);
	sysPrefs->write(EESYS_WIFI1_KEY, thirtytwo);
	sysPrefs->write(EESYS_WIFI2_KEY, thirtytwo);
	sysPrefs->write(EESYS_WIFIX_KEY, thirtytwo);

	sysPrefs->saveChecksum();
}

void initializeDevices() {
	DeviceManager *deviceManager = DeviceManager::getInstance();

#ifdef CFG_ENABLE_DEVICE_HEARTBEAT
	Logger::info("add: Heartbeat");
	heartbeat = new Heartbeat();
	heartbeat->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE_ACCEL
	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	//if min is less than max for a throttle then the pot goes low to high as pressed.
	//if max is less than min for a throttle then the pot goes high to low as pressed.
	Logger::info("add device: PotThrottle accelerator");
	Throttle *accelerator = new PotThrottle(0, 1);//specify the shield ADC ports to use for throttle 255 = not used (valid only for second value)
	accelerator->setup();
	deviceManager->addDevice(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE_ACCEL
	Logger::info("add device: CanThrottle accelerator");
	Throttle *accelerator = new CanThrottle(canHandler1);
	accelerator->setup();
	deviceManager->addDevice(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE_BRAKE
	Logger::info("add device: PotThrottle brake");
	Throttle *brake = new PotBrake(2, 255); //set up the brake input as the third ADC input from the shield.
	brake->setup();
	deviceManager->addDevice(brake);
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE_BRAKE
	Logger::info("add device: CanThrottle brake");
	Throtle *brake = new CanThrottle();
	brake->setup();
	deviceManager->addDevice(brake);
#endif
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
	Logger::info("add device: DMOC 645");
	MotorController *motorController = new DmocMotorController(); //instantiate a DMOC645 device controller as our motor controller
	motorController->setup();
	deviceManager->addDevice(motorController);
#endif
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
	Logger::info("add device: Brusa DMC5");
#endif
}

void setup() {
	SerialUSB.begin(CFG_SERIAL_SPEED);
	SerialUSB.println(CFG_VERSION);

	TickHandler::initialize(); // initialize the TickHandler

	pinMode(BLINK_LED, OUTPUT);
	digitalWrite(BLINK_LED, LOW);

	canHandlerEV = CanHandler::getInstanceEV();
	canHandlerCar = CanHandler::getInstanceCar();
	canHandlerEV->initialize();
	canHandlerCar->initialize();

	Wire.begin();
	Logger::info("TWI init ok");

	memCache = new MemCache();
	memCache->setup();
	sysPrefs = new PrefHandler(EE_SYSTEM_START);
	if (!sysPrefs->checksumValid()) {
		Logger::info("Initializing EEPROM");
		initSysEEPROM();
	} else {  //checksum is good, read in the values stored in EEPROM
		Logger::info("Using existing EEPROM values");
	}

	//rtc_clock.init();
	//Now, we have no idea what the real time is but the EEPROM should have stored a time in the past.
	//It's better than nothing while we try to figure out the proper time.
	/*
	 uint32_t temp;
	 sysPrefs->read(EESYS_RTC_TIME, &temp);
	 rtc_clock.change_time(temp);
	 sysPrefs->read(EESYS_RTC_DATE, &temp);
	 rtc_clock.change_date(temp);
	 
	 Logger::info("RTC init ok");
	 */

	setup_sys_io(); //get calibration data for system IO
	Logger::info("SYSIO init ok");

	initializeDevices();

        serialConsole = new SerialConsole(memCache, heartbeat);
        Logger::info("Serial console Ready");
        
	Logger::info("System Ready");
	serialConsole->printMenu();
}

void loop() {
	// check if incoming frames are available in the can buffer and process them
	canHandlerEV->processInput();
	canHandlerCar->processInput();

	serialConsole->loop();

	//this should still be here. It checks for a flag set during an interrupt
	sys_io_adc_poll();
}
