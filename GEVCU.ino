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

#include "Arduino.h"
#include "SPI.h"
#include "MCP2515.h"
#include "pedal_pot.h"
#include "device.h"
#include "motorctrl.h"
#include "dmoc.h"
#include "timer.h"

// Pin definitions specific to how the MCP2515 is wired up.
#define CS_PIN    85
#define RESET_PIN  7
#define INT_PIN    84

// Create CAN object with pins as defined
MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);
POT_THROTTLE throttle(0, 1); //specify the ADC ports to use for throttle 255 = not used (valid only for second value)
MOTORCTRL* motorcontroller; //generic motor controller - instantiate some derived class to fill this out

void printMenu();

//Evil, global variables
bool runRamp = false;
bool runStatic = false;
bool runThrottle = false;
byte i=0;
Frame message;

void CANHandler() {
	CAN.intHandler();
}

void setup() {
	Serial.begin(115200);

	Serial.println("GEVCU alpha 02-03-2013");

	Serial.println("Initializing ...");

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

	attachInterrupt(6, CANHandler, FALLING);
	CAN.InitFilters(false);

	//Setup CANBUS comm to allow DMOC command and status messages through
	CAN.SetRXMask(MASK0, 0x7F0, 0); //match all but bottom four bits, use standard frames
	CAN.SetRXFilter(FILTER0, 0x230, 0); //allows 0x230 - 0x23F
	CAN.SetRXFilter(FILTER1, 0x650, 0); //allows 0x650 - 0x65F

	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	Serial.println("Using dual pot throttle");
	//if min is less than max for a throttle then the pot goes low to high as pressed.
	//if max is less than min for a throttle then the pot goes high to low as pressed.
	throttle.setT1Min(82);
	throttle.setT1Max(410);
	throttle.setT2Min(158);
	throttle.setT2Max(810);
	//these are now based on tenths of a percent of pedal (0 - 1000 in other words)
	throttle.setRegenEnd(0); //no regen
	throttle.setMaxRegen(30); //thats 30% of forward power
	throttle.setFWDStart(131); //13.1% throttle
	throttle.setMAP(665); //half way throttle is at 2/3 of pedal travel

	//This could eventually be configurable.
	setupTimer(10000); //10ms / 10000us ticks / 100Hz
	Serial.println("100hz update frequency");

        motorcontroller = new DMOC(&CAN); //instantiate a DMOC645 device controller as our motor controller
        
        //This will not be hard coded soon. It should be a list of every hardware support module
	//compiled into the ROM
	Serial.println("Installed devices: DMOC645");

	Serial.print("System Ready ");
	printMenu();

}

void printMenu() {
	Serial.println("System Menu:");
	Serial.println("To start: enter letters slowly in this order:");
	Serial.println("D, S, E, d, <space>");
	Serial.println("D = disabled op state");
	Serial.println("S = standby op state");
	Serial.println("E = enabled op state");
	Serial.println("n = neutral gear");
	Serial.println("d = DRIVE gear");
	Serial.println("r = reverse gear");
	Serial.println("<space> = start/stop RPM ramp test");
	Serial.println("x = lock RPM at current value (toggle)");
	Serial.println("t = Use accelerator pedal? (toggle)");
	Serial.println("");
}


//Note that the loop uses the motorcontroller object which is of the MOTORCTRL class. This allows
//the loop to be generic while still supporting a variety of hardware. Let's try to keep it this way.
void loop() {
	static byte dotTick = 0;
	static byte throttleval = 0;
	static byte count = 0;
	if (CAN.GetRXFrame(message)) {
		motorcontroller->handleFrame(message);
	}
	if (tickReady) {
		//if (dotTick == 0) Serial.print('.'); //print . every 256 ticks (2.56 seconds)
		dotTick = dotTick + 1;
		tickReady = false;
		//do tick related stuff
		throttle.handleTick(); //gets ADC values, calculates throttle position
		//Serial.println(Throttle.getThrottle());
		if (!runThrottle) {
            count++;
            if (count > 50) {
                count = 0;
                if (!runStatic) throttleval++;
            }
            if (throttleval > 80) throttleval = 0;
            if (!runRamp) {
                throttleval = 0;
            }
            motorcontroller->setThrottle(throttleval * (int)12); //with throttle 0-80 this sets throttle to 0 - 960
	}
	else {
            motorcontroller->setThrottle(throttle.getThrottle());
//            Serial.println(throttle.getThrottle());  //just for debugging
		}
		motorcontroller->handleTick();
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
		}
		else Serial.println("End Ramp Test");
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
		}
		else Serial.println("Ignore throttle pedal");
		break;
	}
}
