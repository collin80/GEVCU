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

 New, new plan: Allow for an arbitrary # of devices that can have both tick and canbus handlers. These devices register themselves
 into the handler framework and specify which sort of device they are. They can have custom tick intervals and custom can filters.
 The system automatically manages when to call the tick handlers and automatically filters canbus and sends frames to the devices.
 There is a facility to send data between devices by targetting a certain type of device. For instance, a motor controller
 can ask for any throttles and then retrieve the current throttle position from them.
 */

#include "GEVCU.h"

// The following includes are required in the .ino file by the Arduino IDE in order to properly
// identify the required libraries for the build.
#include <due_rtc.h>
#include <due_can.h>
#include <due_wire.h>
#include <DueTimer.h>

//RTC_clock rtc_clock(XTAL); //init RTC with the external 32k crystal as a reference

Throttle *accelerator;
Throttle *brake;
MotorController* motorController; //generic motor controller - instantiate some derived class to fill this out
PrefHandler sysPrefs(EE_SYSTEM_START);
CanHandler *canHandler0, *canHandler1;
Heartbeat *heartbeat;

//Evil, global variables
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

void initializeDevices() {
#ifdef CFG_ENABLE_DEVICE_HEARTBEAT
	heartbeat = new Heartbeat();
	Logger::info("add device: Heartbeat");
	heartbeat->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE_ACCEL
	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	//if min is less than max for a throttle then the pot goes low to high as pressed.
	//if max is less than min for a throttle then the pot goes high to low as pressed.
	accelerator = new PotThrottle(0, 255, true);//specify the shield ADC ports to use for throttle 255 = not used (valid only for second value)
	Logger::info("add device: PotThrottle accelerator");
	accelerator->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE_ACCEL
	accelerator = new CanThrottle(canHandler1);
	Logger::info("add device: CanThrottle accelerator");
	accelerator->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE_BRAKE
	brake = new PotThrottle(2, 255, false); //set up the brake input as the third ADC input from the shield.
	Logger::info("add device: PotThrottle brake");
	brake->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE_BRAKE
	brake = new CanThrottle();
	Logger::info("add device: CanThrottle brake");
	brake->setup();
#endif
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
	motorController = new DmocMotorController(canHandler0, accelerator, brake); //instantiate a DMOC645 device controller as our motor controller
	Logger::info("add device: DMOC 645");
	motorController->setup();
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

	canHandler0 = new CanHandler(0, CFG_CAN0_SPEED);
	canHandler1 = new CanHandler(1, CFG_CAN1_SPEED);

	Wire.begin();
	Logger::info("TWI init ok");

	if (!sysPrefs.checksumValid()) { //checksum is good, read in the values stored in EEPROM
		initSysEEPROM();
	}

	//rtc_clock.init();
	//Now, we have no idea what the real time is but the EEPROM should have stored a time in the past.
	//It's better than nothing while we try to figure out the proper time.
	/*
	 uint32_t temp;
	 sysPrefs.read(EESYS_RTC_TIME, &temp);
	 rtc_clock.change_time(temp);
	 sysPrefs.read(EESYS_RTC_DATE, &temp);
	 rtc_clock.change_date(temp);
	 
	 Logger::info("RTC init ok");
	 */

	setup_sys_io(); //get calibration data for system IO
	Logger::info("SYSIO init ok");

	initializeDevices();
	Logger::info("System Ready");
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

void loop() {
	static CANFrame message;

	//TODO: Canhandlers should use interupts -> once they do, this can be removed
	if (canHandler0->readFrame(message)) {
		motorController->handleCanFrame(message);
	}

	if (SerialUSB.available())
		serialEvent(); //due doesnt have interrupt driven serial yet
}

/*Single character interpreter of commands over
 serial connection. There is a help menu (press H or h or ?)
 */
void serialEvent() {
	int incoming;
	static int state = 0;
	DmocMotorController* dmoc = (DmocMotorController*) motorController; //TODO: direct reference to dmoc must be removed
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
		case 'Y':
			Logger::info("Trying to save 0x45 to eeprom location 10");
			uint8_t temp;
			memCache.Write(10, (uint8_t) 0x45);
			memCache.Read(10, &temp);
			Logger::info("Got back value of %d", temp);
			break;
		case 'U':
			Logger::info("Adding a sequence of values from 0 to 255 into eeprom");
			for (int i = 0; i < 256; i++)
				memCache.Write(1000 + i, (uint8_t) i);
			Logger::info("Flushing cache");
			memCache.FlushAllPages(); //write everything to eeprom
			memCache.InvalidateAll(); //remove all data from cache
			Logger::info("Operation complete.");
			break;
		case 'I':
			Logger::info("Retrieving data previously saved");
			uint8_t val;
			for (int i = 0; i < 256; i++) {
				memCache.Read(1000 + i, &val);
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
		}
	}

}
