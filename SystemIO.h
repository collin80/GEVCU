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
#include "TickHandler.h"
#include "Status.h"

class SystemIOConfiguration
{
public:
    uint8_t enableInput; // # of input for enable signal - required so that GEVCU enables the controller and requests torque/speed > 0
    uint16_t prechargeMillis; // milliseconds required for the pre-charge cycle
    uint8_t prechargeOutput; // # of output to use for the pre-charge relay or 255 if not used
    uint8_t hvPositiveOutput; // # of output to use for the HV positive relay (main contactor) or 255 if not used
    uint8_t hvNegativeOutput; // # of output to use for the HV negative relay or 255 if not used
    uint8_t enableOutput; // # of output to use for the enable signal/relay or 255 if not used

    uint8_t coolingOutput; // # of output to use for the cooling fan relay or 255 if not used
    uint8_t coolingTempOn; // temperature in degree celsius to start cooling
    uint8_t coolingTempOff; // temperature in degree celsius to stop cooling

    uint8_t brakeLightOutput; // #of output for brake light at regen or 255 if not used
    uint8_t reverseLightOutput; // #of output for reverse light or 255 if not used
};

class SystemIO : public TickObserver
{
public:
    static SystemIO *getInstance();
    void setup();
    void handleTick();

    void loadConfiguration();
    void saveConfiguration();
    SystemIOConfiguration *getConfiguration();

    bool getEnableInput();

    uint16_t getAnalogIn(uint8_t which);
    boolean getDigitalIn(uint8_t which);
    void setDigitalOut(uint8_t which, boolean active);
    boolean getDigitalOut(uint8_t which);
    void ADCPoll();
    uint32_t getNextADCBuffer();
    void printIOStatus();

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
    ADC_COMP adcComp[CFG_NUMBER_ANALOG_INPUTS];

    //the ADC values fluctuate a lot so smoothing is required.
//    int numberADCSamples;
//    uint16_t adcAverageBuffer[CFG_NUMBER_ANALOG_INPUTS][64];
//    uint8_t adcPointer[CFG_NUMBER_ANALOG_INPUTS]; //pointer to next position to use

    bool useRawADC;
    uint32_t preChargeStart; // time-stamp when pre-charge cycle has started
    bool coolflag;
    SystemIOConfiguration *configuration;
    Status *status;
    PrefHandler *prefsHandler;

    SystemIO();
    void initializePinTables();
    void initGevcu2PinTable();
    void initGevcu3PinTable();
    void initGevcu4PinTable();
    void initGevcuLegacyPinTable();
    void initializeDigitalIO();
    void initializeAnalogIO();

    uint16_t getDifferentialADC(uint8_t which);
    uint16_t getRawADC(uint8_t which);
    void setupFastADC();

    void updateDigitalInputStatus();
    void handleCooling();
    void handlePreCharge();

    void setPrechargeRelayOutput(bool);
    void setHvPositiveRelayOutput(bool);
    void setHvNegativeRelayOutput(bool);
    void setEnableRelayOutput(bool);
    void setCoolingRelayOutput(bool);
    void setBrakeLightOutput(bool);
    void setReverseLightOutput(bool);
};

#endif
