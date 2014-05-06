/*
 * SystemIO.h
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
#include "Logger.h"
#include "Device.h"

extern PrefHandler *sysPrefs; //TODO: replace with own prefs handler

class SystemIO : public Device
{
public:
    static SystemIO *getInstance();
    void setup();
    void handleTick();
    void ADCPoll();
    uint16_t getAnalog(uint8_t which);
    boolean getDigital(uint8_t which);
    void setOutput(uint8_t which, boolean active);
    boolean getOutput(uint8_t which);
    uint32_t getNextADCBuffer();
    void printIOStatus();

    //TODO: remove from public
    void earlySetup();
    void setupSysIO();


protected:

private:
    typedef struct {
        uint16_t offset;
        uint16_t gain;
    } ADC_COMP;

    uint8_t dig[CFG_NUMBER_DIGITAL_INPUTS];
    uint8_t adc[CFG_NUMBER_ANALOG_INPUTS][2];
    uint8_t out[CFG_NUMBER_DIGITAL_OUTPUTS];

    volatile int bufn, obufn;
    volatile uint16_t adcBuffer[CFG_NUMBER_ANALOG_INPUTS][256]; // 4 buffers of 256 readings
    uint16_t adcValues[CFG_NUMBER_ANALOG_INPUTS * 2];
    uint16_t adcOutValues[CFG_NUMBER_ANALOG_INPUTS];

//    int numberADCSamples;
    ADC_COMP adcComp[CFG_NUMBER_ANALOG_INPUTS];

    //the ADC values fluctuate a lot so smoothing is required.
//    uint16_t adcAverageBuffer[CFG_NUMBER_ANALOG_INPUTS][64];
//    uint8_t adcPointer[CFG_NUMBER_ANALOG_INPUTS]; //pointer to next position to use

    bool useRawADC;

    uint16_t getDifferentialADC(uint8_t which);
    uint16_t getRawADC(uint8_t which);
//    void earlySetup();
//    void setupSysIO();
    void setupFastADC();
};


#endif
