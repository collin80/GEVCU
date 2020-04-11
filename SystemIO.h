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

class Status;

enum SystemType {
    GEVCU1 = 1,
    GEVCU2 = 2,
    GEVCU3 = 3,
    GEVCU4 = 4
};

class SystemIOConfiguration
{
public:
    uint8_t enableInput; // # of input for enable signal - required so that GEVCU enables the controller and requests torque/speed > 0
    uint8_t chargePowerAvailableInput; // # of input to signal availability of charging power (shore power)
    uint8_t interlockInput; // # of input to signal if the interlock circuit is closed and HV voltage can be applied
    uint8_t reverseInput; // # of input to signal if reverse mode is selected
    uint8_t absInput; // # of input to signal if ABS system is active
    uint8_t gearChangeInput; // # of intput to signal that a gear change is in progress

    uint16_t prechargeMillis; // milliseconds required for the pre-charge cycle
    uint8_t prechargeRelayOutput; // # of output to use for the pre-charge relay or 255 if not used
    uint8_t mainContactorOutput; // # of output to use for the main contactor relay (main contactor) or 255 if not used
    uint8_t secondaryContactorOutput; // # of output to use for the secondary contactor relay or 255 if not used
    uint8_t fastChargeContactorOutput; // # of output to use for the fast charge contactor relay or 255 if not used

    uint8_t enableMotorOutput; // # of output to use for the enable signal/relay or 255 if not used
    uint8_t enableChargerOutput; // # of output to activate the charger
    uint8_t enableDcDcOutput; // # of output to enable the DC to DC cenverter
    uint8_t enableHeaterOutput; // # of output to enable heater
    uint8_t heaterTemperatureOn; // temperature in deg C below which the heater is turned on

    uint8_t heaterValveOutput; // # of output to control heater valve (heat cabin or batteries)
    uint8_t heaterPumpOutput; // # of output to control heater pump
    uint8_t coolingPumpOutput; // # of output to control cooling pump
    uint8_t coolingFanOutput; // # of output to use for the cooling fan relay or 255 if not used
    uint8_t coolingTempOn; // temperature in degree celsius to start cooling
    uint8_t coolingTempOff; // temperature in degree celsius to stop cooling

    uint8_t brakeLightOutput; // #of output for brake light at regen or 255 if not used
    uint8_t reverseLightOutput; // #of output for reverse light or 255 if not used
    uint8_t powerSteeringOutput; // #of output for power steering or 255 if not used
    uint8_t unusedOutput; // #of output for ... or 255 if not used

    uint8_t warningOutput; // #of output for warning light/relay or 255 if not used
    uint8_t powerLimitationOutput; // #of output for power limitation light or 255 if not used
    uint8_t stateOfChargeOutput; // #of output to indicate the SoC or 255 if not used
    uint8_t statusLightOutput; // #of output for the status light or 255 if not used

    SystemType systemType; // the system type
    Logger::LogLevel logLevel; // the system's loglevel
};

class SystemIO : public TickObserver
{
public:
    SystemIO();
    virtual ~SystemIO();
    void setup();
    void handleTick();

    void loadConfiguration();
    void saveConfiguration();
    SystemIOConfiguration *getConfiguration();

    bool isEnableSignalPresent();
    bool isChargePowerAvailable();
    bool isInterlockPresent();
    bool isReverseSignalPresent();
    bool isABSActive();
    bool isGearChangeActive();

    void setEnableMotor(bool);
    void setEnableCharger(bool);
    void setEnableDcDc(bool);
    void setEnableHeater(bool);

    void setHeaterValve(bool);
    void setHeaterPump(bool);
    void setCoolingPump(bool);
    void setCoolingFan(bool);

    void setBrakeLight(bool);
    void setReverseLight(bool);
    void setPowerSteering(bool);
    void setUnused(bool);

    void setWarning(bool);
    void setPowerLimitation(bool);
    void setStateOfCharge(uint8_t);
    void setStatusLight(uint8_t);

    uint16_t getAnalogIn(uint8_t which);
    void setDigitalOut(uint8_t which, boolean active);
    bool getDigitalOut(uint8_t which);
    void ADCPoll();
    uint32_t getNextADCBuffer();
    void printIOStatus();

    void setSystemType(SystemType);
    SystemType getSystemType();
    void setLogLevel(Logger::LogLevel);
    Logger::LogLevel getLogLevel();
    void measurePreCharge();

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
    SystemIOConfiguration *configuration;
    PrefHandler *prefsHandler;
    bool deactivatedPowerSteering, deactivatedHeater;

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
    void powerDownSystem();

    bool handleState();
    void handleCooling();
    void handlePreCharge();
    void handleCharging();
    void handleBrakeLight();
    void handleReverseLight();
    void handleHighPowerDevices();

    bool getDigitalIn(uint8_t which);
    void setAnalogOut(uint8_t which, uint8_t value);

    // for security reasons, these should stay private
    void setPrechargeRelay(bool);
    void setMainContactor(bool);
    void setSecondaryContactor(bool);
    void setFastChargeContactor(bool);

    void logPreCharge();

};

extern SystemIO systemIO;

#endif
