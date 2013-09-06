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

/*
Random comments on things that should be coded up soon:
1. Wifi code needs to be finished. It should read in settings from EEPROM, etc. And start up a webserver. Then
the code should scan for changed parameters occassionally and set them in eeprom
2. Serial console needs to be able to set the wifi stuff
3. A new module needs to be coded that implements precharging. There are two basic types of precharge:
	- If a BMS exists we can grab the pack voltage from there. Otherwise use a set nominal voltage
	- If a motor controller reports voltage then ask it what it sees as we're precharging to see where we are
	- Otherwise wait for 3-5 RC (resistance * capacitance) time constant. Note that the DMOC has about 11,000uf
		capacitance so default to that for C. We have no idea what the user will use for R.
	- Set whether there is a precharge relay and/or main contactor relay and control them.
	- On fault open both if they exist.
4. So, obviously the serial console needs to allow setting R,C,how many RC to wait, whether to ask BMS, motor ctrl
	as well as precharge and main contactor details. 
5. It wouldn't be a bad idea to finish the code for message passing. There are certain messages that all devices
	should support. For one, setup could be done automatically by sending each device a system starting up message.
	Also, in case of fault there should be a way to send a universal "system faulted" message so that each device
	can render its hardware safe. I'm sure there are other messages that should be implemented. Maybe disable/enable a device.
6. It is a possibility that there should be support for actually controlling the power to some of the devices.
	For instance, power could be controlled to the +12V connection at the DMOC so that it can be power cycled
	in software. But, that uses up an input and people can just cycle the key (though that resets the GEVCU too)
7. Support has been added for saving how many throttle pots there are and which throttle type. Make the throttle
	code respect these parameters and use them. Quite hard coding so much stuff.
8. Some people (like me, Collin) have a terrible habit of mixing several coding styles. It would be beneficial to
	continue to harmonize the source code.
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
TickHandler *tickHandler;
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
	heartbeat = new Heartbeat();
	Logger::info("add: Heartbeat (%d)", heartbeat);
	heartbeat->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE
	// Specify the shield ADC port(s) to use for throttle
	// CFG_THROTTLE_NONE = not used (valid only for second value and should not be needed due to calibration/detection)
	Throttle *accelerator = new PotThrottle(CFG_THROTTLE1_PIN, CFG_THROTTLE2_PIN);	Logger::info("add device: PotThrottle (%d)", accelerator);
	accelerator->setup();
	deviceManager->addDevice(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE
	Throttle *accelerator = new CanThrottle();
	Logger::info("add device: CanThrottle (%d)", accelerator);
	accelerator->setup();
	deviceManager->addDevice(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_POT_BRAKE
	Throttle *brake = new PotBrake(CFG_BRAKE_PIN, CFG_THROTTLE_NONE); //set up the brake input as the third ADC input from the shield.
	Logger::info("add device: PotBrake (%d)", brake);
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
	MotorController *motorController = new DmocMotorController(); //instantiate a DMOC645 device controller as our motor controller
	Logger::info("add device: DMOC645 (%d)", motorController);
	motorController->setup();
	deviceManager->addDevice(motorController);
#endif
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
	Logger::info("add device: Brusa DMC5");
#endif
#ifdef CFG_ENABLE_DEVICE_ICHIP2128_WIFI
	Logger::info("add device: iChip 2128 WiFi");
	ICHIPWIFI *iChip = new ICHIPWIFI();
	iChip->setup();
	deviceManager->addDevice(iChip);
#endif
}

void setup() {

	sys_early_setup();

	SerialUSB.begin(CFG_SERIAL_SPEED);
	SerialUSB.println(CFG_VERSION);
        
	pinMode(BLINK_LED, OUTPUT);
	digitalWrite(BLINK_LED, LOW);

	tickHandler = TickHandler::getInstance();

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
#ifdef CFG_TIMER_USE_QUEUING
	//tickHandler->cleanBuffer(); // remove buffered tick events which clogged up already (might not be necessary)
#endif
}

void loop() {

#ifdef CFG_ENABLE_DEVICE_ICHIP2128_WIFI	
	//Evilness... Find a better way to reference the wifi stuff
	Device *tempDevice;
	tempDevice = DeviceManager::getInstance()->getDeviceByID(Device::ICHIP2128);
#endif

#ifdef CFG_TIMER_USE_QUEUING
	tickHandler->process();
#endif

	// check if incoming frames are available in the can buffer and process them
	canHandlerEV->process();
	canHandlerCar->process();

	serialConsole->loop();

#ifdef CFG_ENABLE_DEVICE_ICHIP2128_WIFI	
	((ICHIPWIFI*)tempDevice)->loop();
#endif


	//this should still be here. It checks for a flag set during an interrupt
	sys_io_adc_poll();
}


