/*
 * sys_io.h
 *
 * Handles raw interaction with system I/O
 *
 * Created: 3/14/2013 
 *  Author: Collin Kidder
 */ 


#ifndef SYS_IO_H_
#define SYS_IO_H_

#include <Arduino.h>
#include "config.h"
#include "eeprom_layout.h"
#include "pref_handler.h"

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	4

#define NUM_ADC_SAMPLES  16

typedef struct {
  uint16_t offset;
  uint16_t gain;
} ADC_COMP;

void setup_sys_io();
uint16_t getAnalog(uint8_t which); //get value of one of the 4 analog inputs
boolean getDigital(uint8_t which); //get value of one of the 4 digital inputs
void setOutput(uint8_t which, boolean active); //set output high or not
boolean getOutput(uint8_t which); //get current value of output state (high?)
void setupFastADC();
void sys_io_adc_poll();

extern PREFHANDLER sysPrefs;
	
#endif
