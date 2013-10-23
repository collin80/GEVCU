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
Changelog (Update with very brief message along with build # - Dates wouldn't hurt either
Sept 9 2013:
1000 - First build # enabled build - Comment out hard coded line for # of throttles
1001 - Proper fix for issue corrected by build 1000
1002 - Show build # on startup and in menu
1003 - Implementation of RC precharging mode for motor controller
Sept 12 2013:
1004 - Change some variables in brake to 32 bit
1005 - bug fix in in ichip routines
Sept 19 2013:
1006 - Added ability to configure debugging output - switched DeviceManager to not create null objects
1007 - Allow the DMOC to start up ready to go. It now must be explicitly disabled via the second digital input
1008 - Implement (initial support) reverse limit, switch brake to using indep. min value, get existing values settable with serial console
1009 - Implemented message passing and hooked up the rest of the code to use it.
1010 - Support for GEVCU3 boards
1011 - Added support for Think City battery packs with BMS
1012 - Tons of changes to MotorController and SerialConsole
1013 - New release in master - Addition of Wifi configuration pages, throttle changes
1014 - Brand new system in place that allows devices to be configured based on EEPROM instead of compiler defines.
*/


/*
Random comments on things that should be coded up soon:
1. Wifi code needs to be finished. It should read in settings from EEPROM, etc. And start up a webserver. Then
the code should scan for changed parameters occassionally and set them in eeprom
2. Serial console needs to be able to set the wifi stuff
3. Most of the support code for precharge based on RC is done. But, it still must be tested. Also, it should
	check to see if the motor controller reports voltage and make sure the voltage is reported at least up
	to the set nominal voltage before closing the main contactor. If it takes too long then fault and open
	everything. Also, open contactors in case of a serious fault (but not for just any fault. Opening contactors under load can be nasty!)
4. It is a possibility that there should be support for actually controlling the power to some of the devices.
	For instance, power could be controlled to the +12V connection at the DMOC so that it can be power cycled
	in software. But, that uses up an input and people can just cycle the key (though that resets the GEVCU too)
5. Some people (like me, Collin) have a terrible habit of mixing several coding styles. It would be beneficial to
	continue to harmonize the source code - Perhaps use a tool to do this.
6. It should be possible to limit speed and/or torque in reverse so someone doesn't kill themselves or someone else
	while gunning it in reverse - The configuration variable is there and settable now. Just need to integrate it.
7. The DMOC code duplicates a bunch of functionality that the base class also used to implement. We've got to figure
	out where the overlaps are and fix it up so that as much as possible is done generically at the base MotorController
	class and not directly in the Dmoc class.
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
SerialConsole *serialConsole;

byte i = 0;

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void initSysEEPROM() {
	//three temporary storage places to make saving to EEPROM easy
	uint8_t eight;
	uint16_t sixteen;
	uint32_t thirtytwo;

	eight = SYSTEM_DUED;
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

	//heartbeat is always enabled now
	heartbeat = new Heartbeat();
	Logger::info("add: Heartbeat (id: %X, %X)", HEARTBEAT, heartbeat);
	heartbeat->setup();

	// Specify the shield ADC port(s) to use for throttle
	// CFG_THROTTLE_NONE = not used (valid only for second value and should not be needed due to calibration/detection)
	Throttle *paccelerator = new PotThrottle(CFG_THROTTLE1_PIN, CFG_THROTTLE2_PIN);
	if (paccelerator->isEnabled()) {
		Logger::info("add device: PotThrottle (id: %X, %X)", POTACCELPEDAL, paccelerator);
		deviceManager->addDevice(paccelerator);
	}

	Throttle *caccelerator = new CanThrottle();
	if (caccelerator->isEnabled()) {
		Logger::info("add device: CanThrottle (id: %X, %X)", CANACCELPEDAL, caccelerator);
		deviceManager->addDevice(caccelerator);
	}

	Throttle *pbrake = new PotBrake(CFG_BRAKE_PIN); //set up the brake input as the third ADC input from the shield.
	if (pbrake->isEnabled()) {
		Logger::info("add device: PotBrake (id: %X, %X)", POTBRAKEPEDAL, pbrake);
		deviceManager->addDevice(pbrake);
	}

	Throttle *cbrake = new CanBrake();
	if (cbrake->isEnabled()) {
		Logger::info("add device: CanBrake (id: %X, %X)", CANBRAKEPEDAL, cbrake);
		deviceManager->addDevice(cbrake);
	}

	MotorController *dmotorController = new DmocMotorController(); //instantiate a DMOC645 device controller as our motor controller
	if (dmotorController->isEnabled()) {
		Logger::info("add device: DMOC645 (id:%X, %X)", DMOC645, dmotorController);
		deviceManager->addDevice(dmotorController);
	}

	MotorController *bmotorController = new BrusaMotorController(); //instantiate a Brusa DMC5 device controller as our motor controller
	if (bmotorController->isEnabled()) {
		Logger::info("add device: Brusa DMC5 (id: %X, %X)", BRUSA_DMC5, bmotorController);
		deviceManager->addDevice(bmotorController);
	}

	BatteryManager *BMS = new ThinkBatteryManager();
	if (BMS->isEnabled()) {
		Logger::info("add device: Th!nk City BMS (id: %X, %X)", THINKBMS, BMS);
		deviceManager->addDevice(BMS);
	}

// add wifi as last device, because ICHIPWIFI::loadParameters() depends on pre-loaded preferences
	Logger::info("Trying WIFI");
	ICHIPWIFI *iChip = new ICHIPWIFI();
	if (iChip->isEnabled()) {
		Logger::info("add device: iChip 2128 WiFi (id: %X, %X)", ICHIP2128, iChip);
		deviceManager->addDevice(iChip);
	}

	/*
	 *	We defer setting up the devices until here. This allows all objects to be instantiated
	 *	before any of them set up. That in turn allows the devices to inspect what else is
	 *	out there as they initialize. For instance, a motor controller could see if a BMS
	 *	exists and supports a function that the motor controller wants to access.
	 */
	deviceManager->sendMessage(DEVICE_ANY, INVALID, MSG_STARTUP, NULL);

}

void setup() {

	pinMode(BLINK_LED, OUTPUT);
	digitalWrite(BLINK_LED, LOW);

	SerialUSB.begin(CFG_SERIAL_SPEED);
	SerialUSB.println(CFG_VERSION);
	SerialUSB.print("Build number: ");
	SerialUSB.println(CFG_BUILD_NUM);

	Wire.begin();
	Logger::info("TWI init ok");

	memCache = new MemCache();
	Logger::info("add MemCache (id: %X, %X)", MEMCACHE, memCache);
	memCache->setup();
	sysPrefs = new PrefHandler(SYSTEM);
	if (!sysPrefs->checksumValid()) {
		Logger::info("Initializing EEPROM");
		initSysEEPROM();
	} else {  //checksum is good, read in the values stored in EEPROM
		Logger::info("Using existing EEPROM values");
	}

	sys_early_setup();
        
	tickHandler = TickHandler::getInstance();

	canHandlerEV = CanHandler::getInstanceEV();
	canHandlerCar = CanHandler::getInstanceCar();
	canHandlerEV->initialize();
	canHandlerCar->initialize();

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

	Device *tempDevice;
	tempDevice = DeviceManager::getInstance()->getDeviceByID(ICHIP2128);


#ifdef CFG_TIMER_USE_QUEUING
	tickHandler->process();
#endif

	// check if incoming frames are available in the can buffer and process them
	canHandlerEV->process();
	canHandlerCar->process();

	serialConsole->loop();

	if ( tempDevice != NULL ) {
		((ICHIPWIFI*)tempDevice)->loop();
	}

	//this should still be here. It checks for a flag set during an interrupt
	sys_io_adc_poll();
}


