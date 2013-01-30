/*
 * GEVCU.cpp
 *
 * Created: 1/4/2013 1:34:14 PM
 *  Author: Collin
 *
 *	Need to add a class for keeping track of the various modules installed. This
 *	class has a list of all the modules so we can call the proper functions
 *
 *	Additionally, all modules should inherit from one of the (as of yet unwritten)
 *	base device classes (motor controller, bms, charger, display)
 *
 *
 */ 

#include "Arduino.h"
#include "SPI.h"
#include "MCP2515.h"
#include "pedal_pot.h"
#include "modulemanager.h"
#include "device.h"
#include "dmoc.h"
#include "timer.h"

// Pin definitions specific to how the MCP2515 is wired up.
#define CS_PIN    85
#define RESET_PIN  7
#define INT_PIN    84

// Create CAN object with pins as defined
MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);
THROTTLE Throttle(0, 1);
DMOC dmoc(&CAN);
MODULEMANAGER modules();

void printMenu();

bool runRamp = false;

void CANHandler() {
	CAN.intHandler();
}

void setup() {
	Serial.begin(115200);
	
	Serial.println("GEVCU alpha 01-18-2013");
	
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
	CAN.SetRXMask(MASK0, 0x7F0, 0); //match all but bottom four bits
	CAN.SetRXFilter(FILTER0, 0x230, 0); //allows 0x230 - 0x23F
	CAN.SetRXFilter(FILTER1, 0x650, 0); //allows 0x650 - 0x65F
	
	//modules.add(DMOC);
	
	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	Serial.println("Using dual pot throttle");
	Throttle.setT1Min(82);
	Throttle.setT1Max(410);	
	Throttle.setT2Min(158);
	Throttle.setT2Max(810);
	//these are based off of throttle 1
	Throttle.setRegenEnd(125); //so, regen from 82 - 125
	Throttle.setMaxRegen(30); //thats 30% of forward power
	Throttle.setFWDStart(165); //deadzone 126 to 164, then forward from there to 410
	Throttle.setMAP(350); //but 1/2 way power is at 350 so it's gradual until near the end and then it gets brutal
	
	//This could eventually be configurable.
	setupTimer(10000); //10ms / 10000us ticks / 100Hz
	Serial.println("100hz update frequency");
	
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
	Serial.println("");
}

byte i=0;

// CAN message frame (actually just the parts that are exposed by the MCP2515 RX/TX buffers)
Frame message;

void loop() {	
	static byte dotTick = 0;
	static byte throttle = 0;
	static byte count = 0;
	if (CAN.GetRXFrame(message)) {
		dmoc.handleFrame(message);
	}
	if (tickReady) { 
		//if (dotTick == 0) Serial.print('.'); //print . every 256 ticks (2.56 seconds)
		dotTick = dotTick + 1;
		tickReady = false;
		//do tick related stuff
		//Throttle.handleTick(); //gets ADC values, calculates throttle position
		count++;
		if (count > 50) {
			count = 0;
			throttle++;
		}
		if (throttle > 80) throttle = 0;
		if (!runRamp) {
			throttle = 0;			
		}
		dmoc.setThrottle(throttle * (int)5);
		dmoc.handleTick();
	}
}


/*Extremely simple and very unhelpful serial processor 
Gives no onscreen help at all. Here's your menu:
Space - Start/Stop accelerator ramp
(note, they're lower case)
d - switch to drive
n - switch to neutral
r - switch to reverse

Op States: (note, they're upper case)
D - disable operational state
S - standby operational state
E - enable operational state

*/
void serialEvent() {
	int incoming;
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
		dmoc.setGear(DRIVE);
		Serial.println("forward");
		break;
	case 'n':
		dmoc.setGear(NEUTRAL);
		Serial.println("neutral");
		break;
	case 'r':
		dmoc.setGear(REVERSE);
		Serial.println("reverse");
		break;
	case 'D':
		dmoc.setOpState(DISABLED);
		Serial.println("disabled");
		break;
	case 'S':
		dmoc.setOpState(STANDBY);
		Serial.println("standby");
		break;
	case 'E':
		dmoc.setOpState(ENABLE);
		Serial.println("enabled");
		break;		
	}
}