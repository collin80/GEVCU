/*
 * sys_io.c
 *
 * Handles the low level details of system I/O
 *
 * Created: 03/14/2013
 *  Author: Collin Kidder
 */ 

#include "sys_io.h"

//pin definitions for system IO
uint8_t adc[NUM_ANALOG][2] = {{6,7}, {4,5}, {2,3}, {0,1}}; //low, high
uint8_t dig[] = {8, 11, 12, 13};
uint8_t out[] = {50, 26, 44, 36};

//get value of one of the 4 analog inputs
uint16_t getAnalog(uint8_t which) {
	uint16_t low, high;
	
	if (which >= NUM_ANALOG) which = 0;
	
	low = analogRead(adc[which][0]);
	high = analogRead(adc[which][1]);
	
	high = high - low;
	
	return high;
}

//get value of one of the 4 digital inputs
boolean getDigital(uint8_t which) {
	if (which >= NUM_DIGITAL) which = 0;
	return !digitalRead(dig[which]);
}

//set output high or not
void setOutput(uint8_t which, boolean active) {
	if (which >= NUM_OUTPUT) which = 0;
	if (active)
		digitalWrite(out[which], HIGH);
	else digitalWrite(out[which], LOW);
}

//get current value of output state (high?)
boolean getOutput(uint8_t which) {
	if (which >= NUM_OUTPUT) which = 0;
	return digitalRead(out[which]);
}
