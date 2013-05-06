/*
 * sys_io.cpp
 *
 * Handles the low level details of system I/O
 *
 * Created: 03/14/2013
 *  Author: Collin Kidder
 */ 

#include "sys_io.h"

//pin definitions for system IO
uint8_t adc[NUM_ANALOG][2] = {{1,0}, {2,3}, {4,5}, {7,6}}; //low, high
uint8_t dig[] = {11, 9, 13, 12};
uint8_t out[] = {55, 22, 48, 34};

//the ADC values fluctuate a lot so smoothing is required.
//we'll smooth the last 8 values into an average and use
//rolling buffers. 
uint16_t adc_buffer[NUM_ANALOG][NUM_ADC_SAMPLES];
uint8_t adc_pointer[NUM_ANALOG]; //pointer to next position to use

extern PREFHANDLER sysPrefs;

ADC_COMP adc_comp[NUM_ANALOG];

void setup_sys_io() {
  int i;
  //requires the value to be contiguous in memory
  for (i = 0; i < NUM_ANALOG; i++) {
    sysPrefs.read(EESYS_ADC0_GAIN + 4*i, &adc_comp[i].gain);
    sysPrefs.read(EESYS_ADC0_OFFSET + 4*i, &adc_comp[i].offset);
    for (int j = 0; j < NUM_ADC_SAMPLES; j++) adc_buffer[i][j] = 0;
    adc_pointer[i] = 0;
    //adc_comp[i].gain = 1024;
    //adc_comp[i].offset = 0;
  }
  pinMode(dig[0], INPUT);
  pinMode(dig[1], INPUT);
  pinMode(dig[2], INPUT);
  pinMode(dig[3], INPUT);
  
  pinMode(out[0], OUTPUT);
  pinMode(out[1], OUTPUT);
  pinMode(out[2], OUTPUT);
  pinMode(out[3], OUTPUT);
  
}

uint16_t getDiffADC(uint8_t which) {
  uint32_t low, high;
  low = analogRead(adc[which][0]);
  high = analogRead(adc[which][1]);

  if (low < high) {

    //first remove the bias to bring us back to where it rests at zero input volts

    if (low >= adc_comp[which].offset) low -= adc_comp[which].offset;
      else low = 0;
    if (high >= adc_comp[which].offset) high -= adc_comp[which].offset;
      else high = 0;

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
  }
  else high = 0;
        
  if (high > 4096) high = 0; //if it somehow got wrapped anyway then set it back to zero
  
  return high;
}

void addNewADCVal(uint8_t which, uint16_t val) {
  adc_buffer[which][adc_pointer[which]] = val;
  adc_pointer[which] = (adc_pointer[which] + 1) % NUM_ADC_SAMPLES;
}

uint16_t getADCAvg(uint8_t which) {
  uint32_t sum;
  sum = 0;
  for (int j = 0; j < NUM_ADC_SAMPLES; j++) sum += adc_buffer[which][j];
  sum = sum / NUM_ADC_SAMPLES;
  return ((uint16_t)sum);
}

//get value of one of the 4 analog inputs
//Properly handles scaling, bias, and differential input
//Also tries to smooth the output a bit
uint16_t getAnalog(uint8_t which) {
    uint16_t val;
	
    //analogResolution(12);
	
    if (which >= NUM_ANALOG) which = 0;

    val = getDiffADC(which);
    addNewADCVal(which, val);
    
    val = getDiffADC(which);
    addNewADCVal(which, val);
    
    return getADCAvg(which);
}

//get value of one of the 4 digital inputs
boolean getDigital(uint8_t which) {
	if (which >= NUM_DIGITAL) which = 0;
	return !(digitalRead(dig[which]));
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
