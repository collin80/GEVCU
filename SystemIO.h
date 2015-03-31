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
#include "CanHandler.h"
#include "Status.h"

// messages to send
#define CAN_ID_GEVCU_STATUS     0x724 // I/O status message id
#define CAN_ID_GEVCU_ANALOG_IO  0x725 // analog I/O status message id

// messages to listen to
#define CAN_ID_GEVCU_EXT_TEMPERATURE     0x728 // Temperature CAN message        11100101000
#define CAN_MASK                0x7ff // mask for above id's                     11111111111
#define CAN_MASKED_ID           0x728 // masked id for id's from 0x258 to 0x268  11100101000

class SystemIOConfiguration
{
public:
    uint8_t enableInput; // # of input for enable signal - required so that GEVCU enables the controller and requests torque/speed > 0
    uint16_t prechargeMillis; // milliseconds required for the pre-charge cycle
    uint8_t prechargeOutput; // # of output to use for the pre-charge relay or 255 if not used
    uint8_t mainContactorOutput; // # of output to use for the main contactor relay (main contactor) or 255 if not used
    uint8_t secondaryContactorOutput; // # of output to use for the secondary contactor relay or 255 if not used
    uint8_t enableOutput; // # of output to use for the enable signal/relay or 255 if not used

    uint8_t coolingFanOutput; // # of output to use for the cooling fan relay or 255 if not used
    uint8_t coolingTempOn; // temperature in degree celsius to start cooling
    uint8_t coolingTempOff; // temperature in degree celsius to stop cooling

    uint8_t brakeLightOutput; // #of output for brake light at regen or 255 if not used
    uint8_t reverseLightOutput; // #of output for reverse light or 255 if not used
};

class SystemIO : public TickObserver, CanObserver
{
public:
    // Message id=0x724, GEVCU_STATUS
    // The value is composed of 2 bytes: (data[1] << 0) | (data[0] << 8)
    enum GEVCU_RawIO {
        digitalOut8         = 1 << 0,  // 0x0001, data[1], Motorola bit 15
        digitalOut7         = 1 << 1,  // 0x0002, data[1], Motorola bit 14
        digitalOut6         = 1 << 2,  // 0x0004, data[1], Motorola bit 13
        digitalOut5         = 1 << 3,  // 0x0008, data[1], Motorola bit 12
        digitalOut4         = 1 << 4,  // 0x0010, data[1], Motorola bit 11
        digitalOut3         = 1 << 5,  // 0x0020, data[1], Motorola bit 10
        digitalOut2         = 1 << 6,  // 0x0040, data[1], Motorola bit 9
        digitalOut1         = 1 << 7,  // 0x0080, data[1], Motorola bit 8

        digitalIn4          = 1 << 12, // 0x1000, data[0], Motorola bit 3
        digitalIn3          = 1 << 13, // 0x2000, data[0], Motorola bit 2
        digitalIn2          = 1 << 14, // 0x4000, data[0], Motorola bit 1
        digitalIn1          = 1 << 15  // 0x8000, data[0], Motorola bit 0
    };

    // The value is composed of 2 bytes: (data[3] << 0) | (data[2] << 8)
    enum GEVCU_LogicIO {
        heatingPump          = 1 << 3,  // 0x0008, data[1], Motorola bit 28
        batteryHeater        = 1 << 4,  // 0x0010, data[1], Motorola bit 27
        chargePowerAvailable = 1 << 5,  // 0x0020, data[1], Motorola bit 26
        activateCharger      = 1 << 6,  // 0x0040, data[1], Motorola bit 25
        reverseLight         = 1 << 7,  // 0x0080, data[1], Motorola bit 24

        brakeLight           = 1 << 8,  // 0x0100, data[0], Motorola bit 23
        coolingPump          = 1 << 9,  // 0x0200, data[0], Motorola bit 22
        coolingFan           = 1 << 10, // 0x0400, data[0], Motorola bit 21
        secondayContactor    = 1 << 11, // 0x0400, data[0], Motorola bit 20
        mainContactor        = 1 << 12, // 0x1000, data[0], Motorola bit 19
        preChargeRelay       = 1 << 13, // 0x2000, data[0], Motorola bit 18
        enableSignalOut      = 1 << 14, // 0x4000, data[0], Motorola bit 17
        enableSignalIn       = 1 << 15  // 0x8000, data[0], Motorola bit 16
    };

    // The value is composed of a 1 byte integer value (not a bitfield): data[4]
    enum GEVCU_State {
        unknown         = 0, // at start-up the system state is unknown
        init            = 1, // the system is being initialized
        preCharge       = 2, // the system is executing the pre-charge cycle
        preCharged      = 3, // the pre-charge cycle is finished
        batteryHeating  = 4, // before charging, heat the batteries
        charging        = 5, // the batteries are being charged
        charged         = 6, // the charging is finished
        ready           = 7, // the system is ready to accept commands but the motor controller's power stage is inactive
        running         = 8, // the system is running and the power stage of the motor controller is active
        error           = 99
    };

    // The value is composed of 1 byte: (data[5] << 0)
    enum GEVCU_Status {
        warning              = 1 << 6,  // 0x0040, data[1], Motorola bit 41
        powerLimitation      = 1 << 7,  // 0x0080, data[1], Motorola bit 40
    };


    static SystemIO *getInstance();
    void setup();
    void handleCanFrame(CAN_FRAME *frame);
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
    CanHandler *canHandlerEv;
    CAN_FRAME outputFrame; // the output CAN frame;


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
    void processExternalTemperature(byte []);
    void sendIOStatus();
    void handleCooling();
    void handlePreCharge();

    void setPrechargeRelayOutput(bool);
    void setMainContactorRelayOutput(bool);
    void setSecondaryContactorRelayOutput(bool);
    void setEnableRelayOutput(bool);
    void setCoolingFanRelayOutput(bool);
    void setBrakeLightOutput(bool);
    void setReverseLightOutput(bool);
};

#endif
