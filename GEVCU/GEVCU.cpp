/*
 * GEVCU.cpp
 *
 * Created: 1/4/2013 1:34:14 PM
 *  Author: Collin
 */ 

#include "Arduino.h"
#include "SPI.h"
#include "MCP2515.h"

// Pin definitions specific to how the MCP2515 is wired up.
#define CS_PIN    85
#define RESET_PIN  7
#define INT_PIN    84

// Create CAN object with pins as defined
MCP2515 CAN(CS_PIN, RESET_PIN, INT_PIN);

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
	
	// Initialise MCP2515 CAN controller at the specified speed and clock frequency
	// In this case 125kbps with a 16MHz oscillator
	// (Note:  This is the oscillator attached to the MCP2515, not the Arduino oscillaltor)
	if(CAN.Init(250,16))
	{
		Serial.println("MCP2515 Init OK ...");
	} else {
		Serial.println("MCP2515 Init Failed ...");
	}
	
	attachInterrupt(6, CANHandler, FALLING);

	Serial.println("Ready ...");
}

byte i=0;

// CAN message frame (actually just the parts that are exposed by the MCP2515 RX/TX buffers)
Frame message;

void loop() {
	
	if (CAN.GetRXFrame(message)) {
		// Print message
		Serial.print("ID: ");
		Serial.println(message.id,HEX);
		Serial.print("Extended: ");
		if(message.ide) {
			Serial.println("Yes");
		} else {
			Serial.println("No");
		}
		Serial.print("DLC: ");
		Serial.println(message.dlc,DEC);
		for(i=0;i<message.dlc;i++) {
			Serial.print(message.data[i],HEX);
			Serial.print(" ");
		}
		Serial.println();
		Serial.println();

		// Send out a return message for each one received
		// Simply increment message id and data bytes to show proper transmission
		// Note:  Please see explanation at top of sketch.  You might want to comment this out!
		 message.id++;
		 for(i=0;i<message.dlc;i++) {
			message.data[i]++;
		 }
		 CAN.LoadBuffer(TXB0, message);
		 CAN.SendBuffer(TXB0); 
	}
	delay(100);
	Serial.print(CAN.Status());
	Serial.print(" ");
	
}

