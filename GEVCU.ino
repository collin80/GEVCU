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
ThrottleDetector *throttleDetector;
CanHandler *canHandlerEV;
CanHandler *canHandlerCar;
TickHandler *tickHandler;
PrefHandler *sysPrefs;
MemCache *memCache;

bool runRamp = false;
bool runStatic = false;
bool runThrottle = false;
bool throttleDebug = false;
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
	Heartbeat *heartbeat = new Heartbeat();
	Logger::info("add: Heartbeat (%d)", heartbeat);
	heartbeat->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE
	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	//if min is less than max for a throttle then the pot goes low to high as pressed.
	//if max is less than min for a throttle then the pot goes high to low as pressed.
	Throttle *accelerator = new PotThrottle(0, 1);//specify the shield ADC ports to use for throttle 255 = not used (valid only for second value)
	Logger::info("add device: PotThrottle (%d)", accelerator);
	accelerator->setup();
	deviceManager->addDevice(accelerator);
	// Detect/calibrate the throttle. 
	throttleDetector = new ThrottleDetector(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE
	Throttle *accelerator = new CanThrottle();
	Logger::info("add device: CanThrottle (%d)", accelerator);
	accelerator->setup();
	deviceManager->addDevice(accelerator);
#endif
#ifdef CFG_ENABLE_DEVICE_POT_BRAKE
	Throttle *brake = new PotBrake(2, 255); //set up the brake input as the third ADC input from the shield.
	Logger::info("add device: PotBrake (%d)", brake);
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
}

void setup() {
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
	Logger::info("System Ready");
	printMenu();

	tickHandler->cleanBuffer(); // remove buffered tick events which clogged up already (might not be necessary)
}

void printMenu() {
	SerialUSB.println("System Menu:");
	SerialUSB.println("h = help (displays this message)");
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
	SerialUSB.println("A = dump system eeprom values");
	SerialUSB.println("B = dump dmoc eeprom values");
	SerialUSB.println("y = detect throttle min");
	SerialUSB.println("Y = detect throttle max");
	SerialUSB.println("z = detect throttle min/max  and other values");
	SerialUSB.println("Z = save detected throttle values");
	SerialUSB.println("");
}

void loop() {

#ifdef CFG_TIMER_USE_QUEUING
	tickHandler->process();
#endif

	// check if incoming frames are available in the can buffer and process them
	canHandlerEV->process();
	canHandlerCar->process();

	if (SerialUSB.available())
		serialEvent(); //While serial is interrupt driven this function is not automatically called but must be called.

	//this should still be here. It checks for a flag set during an interrupt
	sys_io_adc_poll();
}

/*Single character interpreter of commands over
 serial connection. There is a help menu (press H or h or ?)
 */
void serialEvent() {
	int incoming;
	uint8_t val;
	static int state = 0;
	DmocMotorController* dmoc = (DmocMotorController*) DeviceManager::getInstance()->getMotorController(); //TODO: direct reference to dmoc must be removed
	PotThrottle* accelerator = (PotThrottle*) DeviceManager::getInstance()->getAccelerator();
	incoming = SerialUSB.read();
	if (incoming == -1)
		return;
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
				Logger::info("Start Ramp Test");
				dmoc->setPowerMode(DmocMotorController::MODE_RPM);
			}
			else {
				Logger::info("End Ramp Test");
				dmoc->setPowerMode(DmocMotorController::MODE_TORQUE); //TODO: direct reference to dmoc must be removed
			}
			break;
		case 'd':
			dmoc->setGear(DmocMotorController::DRIVE); //TODO: direct reference to dmoc must be removed
			Logger::info("forward");
			break;
		case 'n':
			dmoc->setGear(DmocMotorController::NEUTRAL); //TODO: direct reference to dmoc must be removed
			Logger::info("neutral");
			break;
		case 'r':
			dmoc->setGear(DmocMotorController::REVERSE); //TODO: direct reference to dmoc must be removed
			Logger::info("reverse");
			break;
		case 'D':
			dmoc->setOpState(DmocMotorController::DISABLED); //TODO: direct reference to dmoc must be removed
			Logger::info("disabled");
			break;
		case 'S':
			dmoc->setOpState(DmocMotorController::STANDBY); //TODO: direct reference to dmoc must be removed
			Logger::info("standby");
			break;
		case 'E':
			dmoc->setOpState(DmocMotorController::ENABLE); //TODO: direct reference to dmoc must be removed
			Logger::info("enabled");
			break;
		case 'x':
			runStatic = !runStatic;
			if (runStatic) {
				Logger::info("Lock RPM rate");
			}
			else
				Logger::info("Unlock RPM rate");
			break;
		case 't':
			runThrottle = !runThrottle;
			if (runThrottle) {
				Logger::info("Use Throttle Pedal");
				dmoc->setPowerMode(DmocMotorController::MODE_TORQUE); //TODO: direct reference to dmoc must be removed
			}
			else {
				Logger::info("Ignore throttle pedal");
				dmoc->setPowerMode(DmocMotorController::MODE_RPM); //TODO: direct reference to dmoc must be removed
			}
			break;
		case 'L':
			throttleDebug = !throttleDebug;
			if (throttleDebug) {
				Logger::info("Output raw throttle");
			}
			else
				Logger::info("Cease raw throttle output");
			break;
		case 'U':
			Logger::info("Adding a sequence of values from 0 to 255 into eeprom");
			for (int i = 0; i < 256; i++)
				memCache->Write(1000 + i, (uint8_t) i);
			Logger::info("Flushing cache");
			memCache->FlushAllPages(); //write everything to eeprom
			memCache->InvalidateAll(); //remove all data from cache
			Logger::info("Operation complete.");
			break;
		case 'I':
			Logger::info("Retrieving data previously saved");
			for (int i = 0; i < 256; i++) {
				memCache->Read(1000 + i, &val);
				Logger::info("%d: %d", i, val);
			}
			break;
		case 'A':
			Logger::info("Retrieving System EEPROM values");
			for (int i = 0; i < 256; i++) {
				memCache->Read(EE_SYSTEM_START + i, &val);
				Logger::info("%d: %d", i, val);
			}
			break;
		case 'B':
			Logger::info("Retrieving DMOC EEPROM values");
			for (int i = 0; i < 256; i++) {
				memCache->Read(EE_MOTORCTL_START + i, &val);
				Logger::info("%d: %d", i, val);
			}
			break;

		case 'K': //set all outputs high
			setOutput(0, true);
			setOutput(1, true);
			setOutput(2, true);
			setOutput(3, true);
			Logger::info("all outputs: ON");
			break;
		case 'J': //set the four outputs low
			setOutput(0, false);
			setOutput(1, false);
			setOutput(2, false);
			setOutput(3, false);
			Logger::info("all outputs: OFF");
			break;
                case 'y': // detect throttle min
			accelerator->detectThrottleMin();
			break;
                case 'Y': // detect throttle max
			accelerator->detectThrottleMax();
			break;
		case 'z': // detect throttle min/max & other details
			accelerator->detectThrottle();
			break;
		case 'Z': // save throttle settings
                        accelerator->saveConfiguration();
			break;
		}
	}
}
