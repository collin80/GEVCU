/*
 * sys_io.h
 *
 * Handles raw interaction with system I/O
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

 */ 


#ifndef SYS_IO_H_
#define SYS_IO_H_

#include <Arduino.h>
#include "config.h"
#include "eeprom_layout.h"
#include "PrefHandler.h"

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
uint16_t getDiffADC(uint8_t which);
boolean getDigital(uint8_t which); //get value of one of the 4 digital inputs
void setOutput(uint8_t which, boolean active); //set output high or not
boolean getOutput(uint8_t which); //get current value of output state (high?)
void setupFastADC();
void sys_io_adc_poll();
void sys_early_setup();

#endif
