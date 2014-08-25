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
 
/*Changelog removed. All changes are logged to GIT */

/*
Random comments on things that should be coded up soon:
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
Device *wifiDevice;
Device *btDevice;

byte i = 0;

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void initSysEEPROM() {
	//three temporary storage places to make saving to EEPROM easy
	uint8_t eight;
	uint16_t sixteen;
	uint32_t thirtytwo;

	eight = 4; //GEVCU4 or GEVCU5 boards
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

	eight = 1;
	sysPrefs->write(EESYS_LOG_LEVEL, eight);

	sysPrefs->saveChecksum();
}

void createObjects() {
	PotThrottle *paccelerator = new PotThrottle();
	CanThrottle *caccelerator = new CanThrottle();
	PotBrake *pbrake = new PotBrake();
	CanBrake *cbrake = new CanBrake();
	DmocMotorController *dmotorController = new DmocMotorController();
        CodaMotorController *cmotorController = new CodaMotorController();
	BrusaMotorController *bmotorController = new BrusaMotorController();
	ThinkBatteryManager *BMS = new ThinkBatteryManager();
	ELM327Emu *emu = new ELM327Emu();
	ICHIPWIFI *iChip = new ICHIPWIFI();
}

void initializeDevices() {
	DeviceManager *deviceManager = DeviceManager::getInstance();

	//heartbeat is always enabled now
	heartbeat = new Heartbeat();
	Logger::info("add: Heartbeat (id: %X, %X)", HEARTBEAT, heartbeat);
	heartbeat->setup();

	/*
	We used to instantiate all the objects here along with other code. To simplify things this is done somewhat
	automatically now. Just instantiate your new device object in createObjects above. This takes care of the details
	so long as you follow the template of how other devices were coded.
	*/
	createObjects(); 

	/*
	 *	We defer setting up the devices until here. This allows all objects to be instantiated
	 *	before any of them set up. That in turn allows the devices to inspect what else is
	 *	out there as they initialize. For instance, a motor controller could see if a BMS
	 *	exists and supports a function that the motor controller wants to access.
	 */
	deviceManager->sendMessage(DEVICE_ANY, INVALID, MSG_STARTUP, NULL);

}

void setup() {
        delay(5000);

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

	uint8_t loglevel;
	sysPrefs->read(EESYS_LOG_LEVEL, &loglevel);
	Logger::setLoglevel((Logger::LogLevel)loglevel);

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
        
	Logger::info("System Ready");
	serialConsole->printMenu();

	wifiDevice = DeviceManager::getInstance()->getDeviceByID(ICHIP2128);
	btDevice = DeviceManager::getInstance()->getDeviceByID(ELM327EMU);

#ifdef CFG_TIMER_USE_QUEUING
	//tickHandler->cleanBuffer(); // remove buffered tick events which clogged up already (might not be necessary)
#endif
}

void loop() {

#ifdef CFG_TIMER_USE_QUEUING
	tickHandler->process();
#endif

	// check if incoming frames are available in the can buffer and process them
	canHandlerEV->process();
	canHandlerCar->process();

	serialConsole->loop();

	//TODO: this is dumb... shouldn't have to manually do this. Devices should be able to register loop functions
	if ( wifiDevice != NULL ) {
		((ICHIPWIFI*)wifiDevice)->loop();
	}

	//if (btDevice != NULL) {
	//	((ELM327Emu*)btDevice)->loop();
	//}

	//this should still be here. It checks for a flag set during an interrupt
	sys_io_adc_poll();
}


