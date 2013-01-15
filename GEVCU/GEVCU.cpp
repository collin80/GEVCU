/*
 * GEVCU.cpp
 *
 * Created: 1/4/2013 1:34:14 PM
 *  Author: Collin
 */ 

#include "Arduino.h"
#include "SPI.h"
#include "MCP2515.h"
#include "pedal_pot.h"
#include "dmoc.h"
#include "timer.h"

// Pin definitions specific to how the MCP2515 is wired up.
#define CS_PIN    85
#define RESET_PIN  7
#define INT_PIN    84

// Create CAN object with pins as defined
MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);
THROTTLE Throttle();
DMOC dmoc();

void CANHandler() {
	CAN.intHandler();
}

void setup() {
	Serial.begin(115200);
	
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
	
	//The pedal I have has two pots and one should be twice the value of the other normally (within tolerance)
	Throttle.setT1Min(82);
	Throttle.setT1Max(410);	
	Throttle.setT2Min(158);
	Throttle.setT2Max(810);
	//these are based off of throttle 1
	Throttle.setRegenEnd(125); //so, regen from 82 - 125
	Throttle.setMaxRegen(30); //thats 30% of forward power
	Throttle.setFWDStart(165); //deadzone 126 to 164, then forward from there to 410
	Throttle.setMAP(350); //but 1/2 way power is at 350 so it's gradual until near the end and then it gets brutal
	
	setupTimer(10000); //10ms or 10000us ticks

	Serial.println("Ready ...");
}

byte i=0;

// CAN message frame (actually just the parts that are exposed by the MCP2515 RX/TX buffers)
Frame message;

void loop() {	
	if (CAN.GetRXFrame(message)) {
		dmoc.handleFrame(message);
	}
	if (tickReady) {
		tickReady = false;
		//do tick related stuff
		Throttle.handleTick(); //gets ADC values, calculates throttle position
		dmoc.setThrottle(Throttle.getThrottle());
		dmoc.handleTick();
	}
}

