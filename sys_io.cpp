/*
 * sys_io.cpp
 *
 * Handles the low level details of system I/O
 *
 * Created: 03/14/2013
 *  Author: Collin Kidder
 */ 

#include "sys_io.h"
#include "eeprom_layout.h"
#include "pref_handler.h"

//pin definitions for system IO
uint8_t adc[NUM_ANALOG][2] = {{6,7}, {4,5}, {2,3}, {0,1}}; //low, high
uint8_t dig[] = {8, 11, 12, 13};
uint8_t out[] = {50, 26, 44, 36};
ADC_COMP adc_comp[NUM_ANALOG];

void setup_sys_io() {
  int i;
  //requires the value to be contiguous in memory
  for (i = 0; i < NUM_ANALOG; i++) {
    //sysPrefs.Read(EESYS_ADC0_GAIN + 4*i, &adc_comp[i].gain);
    //sysPrefs.Read(EESYS_ADC0_OFFSET + 4*i, &adc_comp[i].offset);
    adc_comp[i].gain = 1024;
    adc_comp[i].offset = 0;
  }
}

//get value of one of the 4 analog inputs
//Probably handles scaling, bias, and differential input
//but does nothing to smooth the output. Subsequent stages
//must handle that.
uint16_t getAnalog(uint8_t which) {
	uint32_t low, high;
	
	if (which >= NUM_ANALOG) which = 0;
	
	low = analogRead(adc[which][0]);
	high = analogRead(adc[which][1]);

        //first remove the bias to bring us back to where it rests at zero input volts
        low -= adc_comp[which].offset;
        high -= adc_comp[which].offset;
        
        //gain multiplier is 1024 for 1 to 1 gain, less for lower gain, more for higher.
        low *= adc_comp[which].gain;
        low = low >> 10; //divide by 1024 again to correct for gain multiplier
        high *= adc_comp[which].gain;
        high = high >> 10;
	
        //Lastly, the input scheme is basically differential so we have to subtract
        //low from high to get the actual value
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
