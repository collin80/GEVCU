/*
 * sys_io.cpp
 *
 * Handles the low level details of system I/O
 *
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

some portions based on code credited as:
Arduino Due ADC->DMA->USB 1MSPS
by stimmer

*/ 

#include "sys_io.h"

#undef HID_ENABLED

uint8_t dig[NUM_DIGITAL];
uint8_t adc[NUM_ANALOG][2];
uint8_t out[NUM_OUTPUT];

volatile int bufn,obufn;
volatile uint16_t adc_buf[NUM_ANALOG][256];   // 4 buffers of 256 readings
uint16_t adc_values[NUM_ANALOG * 2];
uint16_t adc_out_vals[NUM_ANALOG];

//the ADC values fluctuate a lot so smoothing is required.
//we'll smooth the last 8 values into an average and use
//rolling buffers. 
uint16_t adc_buffer[NUM_ANALOG][NUM_ADC_SAMPLES];
uint8_t adc_pointer[NUM_ANALOG]; //pointer to next position to use

extern PrefHandler *sysPrefs;

ADC_COMP adc_comp[NUM_ANALOG];

bool useRawADC = false;

//forces the digital I/O ports to a safe state. This is called very early in initialization.
void sys_early_setup() {
	int i;

	//the first order of business is to figure out what hardware we are running on and fill in
	//the pin tables.

	uint8_t rawadc;
	sysPrefs->read(EESYS_RAWADC, &rawadc);
	if (rawadc != 0) {
		useRawADC = true;
		Logger::info("Using raw ADC mode");
	}
	else useRawADC = false;

	uint8_t sys_type;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
	if (sys_type == 2) {
		Logger::info("Running on GEVCU2/DUED hardware.");
		dig[0]=9; dig[1]=11; dig[2]=12; dig[3]=13;
		adc[0][0] = 1; adc[0][1] = 0;
		adc[1][0] = 3; adc[1][1] = 2;
		adc[2][0] = 5; adc[2][1] = 4;
		adc[3][0] = 7; adc[3][1] = 6;
		out[0] = 52; out[1] = 22; out[2] = 48; out[3] = 32;
		out[4] = 255; out[5] = 255; out[6] = 255; out[7] = 255;
	} else if (sys_type == 3) {
		Logger::info("Running on GEVCU3 hardware");
		dig[0]=48; dig[1]=49; dig[2]=50; dig[3]=51;
		adc[0][0] = 3; adc[0][1] = 255;
		adc[1][0] = 2; adc[1][1] = 255;
		adc[2][0] = 1; adc[2][1] = 255;
		adc[3][0] = 0; adc[3][1] = 255;
		out[0] = 9; out[1] = 8; out[2] = 7; out[3] = 6;
		out[4] = 255; out[5] = 255; out[6] = 255; out[7] = 255;
		useRawADC = true; //this board does require raw adc so force it.
	} else if (sys_type == 4) {
		Logger::info("Running on GEVCU 4.x hardware");
		dig[0]=48; dig[1]=49; dig[2]=50; dig[3]=51;
		adc[0][0] = 3; adc[0][1] = 255;
		adc[1][0] = 2; adc[1][1] = 255;
		adc[2][0] = 1; adc[2][1] = 255;
		adc[3][0] = 0; adc[3][1] = 255;
		out[0] = 4; out[1] = 5; out[2] = 6; out[3] = 7;
		out[4] = 2; out[5] = 3; out[6] = 8; out[7] = 9;
		useRawADC = true; //this board does require raw adc so force it.
	} else {
		Logger::info("Running on legacy hardware?");
		dig[0]=11; dig[1]=9; dig[2]=13; dig[3]=12;
		adc[0][0] = 1; adc[0][1] = 0;
		adc[1][0] = 2; adc[1][1] = 3;
		adc[2][0] = 4; adc[2][1] = 5;
		adc[3][0] = 7; adc[3][1] = 6;
		out[0] = 52; out[1] = 22; out[2] = 48; out[3] = 32;
		out[4] = 255; out[5] = 255; out[6] = 255; out[7] = 255;
	}
	
	for (i = 0; i < NUM_DIGITAL; i++) pinMode(dig[i], INPUT);
	for (i = 0; i < NUM_OUTPUT; i++) {
		if (out[i] != 255) {
			pinMode(out[i], OUTPUT);
			digitalWrite(out[i], LOW);
		}
	}
}

void setup_sys_io() {
  int i;
  
if (!useRawADC)
  setupFastADC();
else
  analogReadResolution(12);

  //requires the value to be contiguous in memory
  for (i = 0; i < NUM_ANALOG; i++) {
    sysPrefs->read(EESYS_ADC0_GAIN + 4*i, &adc_comp[i].gain);
    sysPrefs->read(EESYS_ADC0_OFFSET + 4*i, &adc_comp[i].offset);
	//Logger::debug("ADC:%d GAIN: %d Offset: %d", i, adc_comp[i].gain, adc_comp[i].offset);
    for (int j = 0; j < NUM_ADC_SAMPLES; j++) adc_buffer[i][j] = 0;
    adc_pointer[i] = 0;
    //adc_comp[i].gain = 1024;
    //adc_comp[i].offset = 0;
    adc_values[i] = 0;
	adc_out_vals[i] = 0;
  }

}

uint16_t getDiffADC(uint8_t which) {
  uint32_t low, high;
  
  low = adc_values[adc[which][0]];
  high = adc_values[adc[which][1]];

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
	
    if (which >= NUM_ANALOG) which = 0;

	if (!useRawADC)
		return adc_out_vals[which]; //return precalculated ADC reading
	else {
		return analogRead(adc[which][0]);		
	}
}

//get value of one of the 4 digital inputs
boolean getDigital(uint8_t which) {
	if (which >= NUM_DIGITAL) which = 0;
	return !(digitalRead(dig[which]));
}

//set output high or not
void setOutput(uint8_t which, boolean active) {
	if (which >= NUM_OUTPUT) return;
	if (out[which] == 255) return;
	if (active)
		digitalWrite(out[which], HIGH);
	else digitalWrite(out[which], LOW);
}

//get current value of output state (high?)
boolean getOutput(uint8_t which) {
	if (which >= NUM_OUTPUT) return false;
	if (out[which] == 255) return false;
	return digitalRead(out[which]);
}

/*
When the ADC reads in the programmed # of readings it will do two things:
1. It loads the next buffer and buffer size into current buffer and size
2. It sends this interrupt
This interrupt then loads the "next" fields with th proper values. This is 
done with a four position buffer. In this way the ADC is constantly sampling
*/
void ADC_Handler(){     // move DMA pointers to next buffer
  int f=ADC->ADC_ISR;
  if (f & (1<<27)){ //receive counter end of buffer
    bufn=(bufn+1)&3;
    adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
    ADC->ADC_MR = (1 << 7) //free running
              + (1 << 8) //4x clock divider
              + (1 << 20) //extra settling time, 5 counts
              + (1 << 24) //2 adc periods tracking time
              + (1 << 28);//5 clocks transfer time
  
    ADC->ADC_CHER=0xFF; //enable A0-A7

    NVIC_EnableIRQ(ADC_IRQn);
    ADC->ADC_IDR=~(1<<27); //dont disable the ADC interrupt for rx end
    ADC->ADC_IER=1<<27; //do enable it
    ADC->ADC_RPR=(uint32_t)adc_buf[bufn];   // DMA buffer
    ADC->ADC_RCR=256; //# of samples to take
    ADC->ADC_RNPR=(uint32_t)adc_buf[(bufn + 1) & 3]; // next DMA buffer
    ADC->ADC_RNCR=256; //# of samples to take
    ADC->ADC_PTCR=1; //enable dma mode
    ADC->ADC_CR=2; //start conversions

   //bufn=(bufn+1)&3;
   //ADC->ADC_RNPR=(uint32_t)adc_buf[bufn];
   //ADC->ADC_RNCR=256;  
   
  } 
}

//setup the ADC hardware to use DMA and run in the background at 1M samples per second. We won't really
//get 1M because 8 channels are set up. We'll probably be lucky to get about 200K but thats still fast.
void setupFastADC(){
  pmc_enable_periph_clk(ID_ADC);
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
  ADC->ADC_MR = (1 << 7) //free running
              + (1 << 8) //4x clock divider
              + (1 << 20) //extra settling time, 5 counts
              + (1 << 24) //2 adc periods tracking time
              + (1 << 28);//5 clocks transfer time
  
  ADC->ADC_CHER=0xFF; //enable A0-A7

  NVIC_EnableIRQ(ADC_IRQn);
  ADC->ADC_IDR=~(1<<27); //dont disable the ADC interrupt for rx end
  ADC->ADC_IER=1<<27; //do enable it
  ADC->ADC_RPR=(uint32_t)adc_buf[0];   // DMA buffer
  ADC->ADC_RCR=256; //# of samples to take
  ADC->ADC_RNPR=(uint32_t)adc_buf[1]; // next DMA buffer
  ADC->ADC_RNCR=256; //# of samples to take
  bufn=obufn=0;
  ADC->ADC_PTCR=1; //enable dma mode
  ADC->ADC_CR=2; //start conversions

  Logger::debug("Fast ADC Mode Enabled");
}

//polls	for the end of an adc conversion event. Then processe buffer to extract the averaged
//value. It takes this value and averages it with the existing value in an 8 position buffer
//which serves as a super fast place for other code to retrieve ADC values
// This is only used when RAWADC is not defined
void sys_io_adc_poll() {
	if (obufn != bufn && !useRawADC) {
		uint32_t tempbuff[8] = {0,0,0,0,0,0,0,0}; //make sure its zero'd
	
		//the eight enabled adcs are interleaved in the buffer
		//this is a somewhat unrolled for loop with no incrementer. it's odd but it works
		for (int i = 0; i < 256;) {	   
			tempbuff[7] += adc_buf[obufn][i++];
			tempbuff[6] += adc_buf[obufn][i++];
			tempbuff[5] += adc_buf[obufn][i++];
			tempbuff[4] += adc_buf[obufn][i++];
			tempbuff[3] += adc_buf[obufn][i++];
			tempbuff[2] += adc_buf[obufn][i++];
			tempbuff[1] += adc_buf[obufn][i++];
			tempbuff[0] += adc_buf[obufn][i++];
		}	

		//for (int i = 0; i < 256;i++) Logger::debug("%i - %i", i, adc_buf[obufn][i]);

		//now, all of the ADC values are summed over 32 readings. So, divide by 32 (shift by 5) to get the average
		//then add that to the old value we had stored and divide by two to average those. Lots of averaging going on.
		for (int j = 0; j < 8; j++) {
			adc_values[j] += (tempbuff[j] >> 5);
			adc_values[j] = adc_values[j] >> 1;
			//Logger::debug("A%i: %i", j, adc_values[j]);
		}
    
		for (int i = 0; i < NUM_ANALOG; i++) {
			int val = getDiffADC(i);
			addNewADCVal(i, val);
			adc_out_vals[i] = getADCAvg(i);
		}

		obufn = bufn;    
	}
}


