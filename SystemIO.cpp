/*
 * SystemIO.cpp
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

#include "SystemIO.h"
#include "DeviceManager.h"

#undef HID_ENABLED

extern PrefHandler *sysPrefs;

/*
 * Constructor
 */
SystemIO::SystemIO() {
    configuration = new SystemIOConfiguration();
    prefsHandler = new PrefHandler(SYSTEM);
    status = Status::getInstance();
    preChargeStart = 0;
}

/*
 * Get the instance of the SystemIO (singleton pattern)
 */
SystemIO *SystemIO::getInstance() {
    static SystemIO *systemIO = new SystemIO();
    return systemIO;
}

void SystemIO::setup() {
    TickHandler::getInstance()->detach(this);

    loadConfiguration();

    initializePinTables();
    initializeDigitalIO();
    initializeAnalogIO();
    printIOStatus();

    TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_SYSTEM_IO);
}

void SystemIO::handleTick() {
    Status::SystemState state = status->getSystemState();

    if (state == Status::error) {
        powerDownSystem();
        return;
    }

    if (!isInterlockPresent()) {
        Logger::error("Interlock circuit open - security risk, disabling HV !!");
        status->setSystemState(Status::error);
    }

    if (isEnableSignalPresent()) {
        // if the system is ready and the enable input is high, then switch to state "running", this should enable the motor controller
        if (state == Status::ready) {
            status->setSystemState(Status::running);
        }
    } else {
        // if enable input is low and the motor controller is running, then disable it by switching to state "ready"
        if (state == Status::running) {
            status->setSystemState(Status::ready);
        }
    }

    if (state == Status::preCharge) {
        handlePreCharge();
    }

    if (state == Status::preCharged) {
        state = status->setSystemState(Status::ready);
    }

    handleCooling();
    handleCharging();

    updateDigitalInputStatus();
}

/*
 * Update the status flags so the input signal can be monitored in the status web page.
 */
void SystemIO::updateDigitalInputStatus() {
    status->digitalInput[0] = getDigitalIn(0);
    status->digitalInput[1] = getDigitalIn(1);
    status->digitalInput[2] = getDigitalIn(2);
    status->digitalInput[3] = getDigitalIn(3);
}

/**
 * Perform the necessary steps to power down the system (incl. HV contactors).
 */
void SystemIO::powerDownSystem() {
    setPrechargeRelay(false);
    setMainContactor(false);
    setSecondaryContactor(false);
    setFastChargeContactor(false);

    setEnableMotor(false);
    setEnableCharger(false);
    setEnableDcDc(false);
    setEnableHeater(false);

    setHeaterValve(false);
    setHeaterPump(false);
    setCoolingPump(false);
    setCoolingFan(false);

    setBrakeLight(false);
    setReverseLight(false);
    setWarning(false);
    setPowerLimitation(false);
}

/*
 * Handle the pre-charge sequence.
 */
void SystemIO::handlePreCharge() {
    if (configuration->prechargeMillis == 0) { // we don't want to pre-charge
        Logger::info("Pre-charging not enabled");
        setMainContactor(true);
        status->setSystemState(Status::preCharged);
        return;
    }

    if (preChargeStart == 0) {
        if (millis() > CFG_PRE_CHARGE_START) {
            Logger::info("Starting pre-charge sequence");
            printIOStatus();

            preChargeStart = millis();
            setPrechargeRelay(true);

#ifdef CFG_THREE_CONTACTOR_PRECHARGE
            delay(CFG_PRE_CHARGE_RELAY_DELAY);
            setSecondaryContactor(true);
#endif
        }
    } else {
        if ((millis() - preChargeStart) > configuration->prechargeMillis) {
            setMainContactor(true);
            delay(CFG_PRE_CHARGE_RELAY_DELAY);
            setPrechargeRelay(false);

            status->setSystemState(Status::preCharged);
            Logger::info("Pre-charge sequence complete after %i milliseconds", millis() - preChargeStart);
        }
    }
}

/*
 * Control an optional cooling fan output depending on the temperature of
 * the motor controller
 */
void SystemIO::handleCooling() {
    Status::SystemState state = status->getSystemState();

    if ((state == Status::ready || state == Status::running || state == Status::charging || state == Status::charged
            || state == Status::batteryHeating) && !status->coolingPump) {
        setCoolingPump(true);
    }
//TODO: what if it stays in mode "charged" for hours ?? will drain the battery , probably power off after 1h ?
    if ((state != Status::ready && state != Status::running && state != Status::charging && state != Status::charged
            && state != Status::batteryHeating) && status->coolingPump) {
        setCoolingPump(false);
    }

    if (status->temperatureController == CFG_NO_TEMPERATURE_DATA) {
        return;
    }

    if (status->temperatureController / 10 > configuration->coolingTempOn && !status->coolingFan) {
        setCoolingFan(true);
    }

    if (status->temperatureController / 10 < configuration->coolingTempOff && status->coolingFan) {
        setCoolingFan(false);
    }
}

/*
 * Verify if we're connected to a charge station and handle the charging process
 * (including an evtl. heating period for the batteries)
 */
void SystemIO::handleCharging() {
    Status::SystemState state = status->getSystemState();

    if (isChargePowerAvailable()) { // we're connected to "shore" power
        if (state == Status::running) {
            state = status->setSystemState(Status::ready);
        }
        if (state == Status::ready || state == Status::batteryHeating) {
            int16_t batteryTemp = status->getLowestExternalTemperature();
            if (batteryTemp == CFG_NO_TEMPERATURE_DATA || batteryTemp >= CFG_MIN_BATTERY_CHARGE_TEMPERATURE) {
                state = status->setSystemState(Status::charging);
            } else {
                state = status->setSystemState(Status::batteryHeating);
            }
        }
    } else {
        // terminate all charge related activities and return to ready if GEVCU is still powered on
        if (state == Status::charging || state == Status::charged || state == Status::batteryHeating) {
            state = status->setSystemState(Status::ready);
        }
    }
}

/*
 * Get the the input signal for the car's enable signal.
 */
bool SystemIO::isEnableSignalPresent() {
    bool flag = getDigitalIn(configuration->enableInput);
    status->enableIn = flag;
    return flag;
}

/*
 * Get the the input signal which indicates if the car is connected to a charge station
 */
bool SystemIO::isChargePowerAvailable() {
    bool flag = getDigitalIn(configuration->chargePowerAvailableInput);
    status->chargePowerAvailable = flag;
    return flag;
}

/*
 * Get the the input signal which indicates if the car is connected to a charge station
 */
bool SystemIO::isInterlockPresent() {
    // if not configured, return true in order not to power-down the system
    if (configuration->interlockInput == CFG_OUTPUT_NONE) {
        return true;
    }

    bool flag = getDigitalIn(configuration->interlockInput);
    status->interlockPresent = flag;
    return flag;
}

/*
 * Enable / disable the pre-charge relay output and set the status flag.
 */
void SystemIO::setPrechargeRelay(bool enabled) {
    setDigitalOut(configuration->prechargeRelayOutput, enabled);
    status->preChargeRelay = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the main contactor relay output and set the status flag.
 */
void SystemIO::setMainContactor(bool enabled) {
    setDigitalOut(configuration->mainContactorOutput, enabled);
    status->mainContactor = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the secondary contactor relay output and set the status flag.
 */
void SystemIO::setSecondaryContactor(bool enabled) {
    setDigitalOut(configuration->secondaryContactorOutput, enabled);
    status->secondaryContactor = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the fast charge contactor relay output and set the status flag.
 */
void SystemIO::setFastChargeContactor(bool enabled) {
    setDigitalOut(configuration->fastChargeContactorOutput, enabled);
    status->fastChargeContactor = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the 'enable' relay output and set the status flag.
 */
void SystemIO::setEnableMotor(bool enabled) {
    setDigitalOut(configuration->enableMotorOutput, enabled);
    status->enableMotor = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the charger activation output and set the status flag.
 */
void SystemIO::setEnableCharger(bool enabled) {
    setDigitalOut(configuration->enableChargerOutput, enabled);
    status->enableCharger = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the battery heater output and set the status flag.
 */
void SystemIO::setEnableDcDc(bool enabled) {
    setDigitalOut(configuration->enableDcDcOutput, enabled);
    status->enableDcDc = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the battery heater output and set the status flag.
 */
void SystemIO::setEnableHeater(bool enabled) {
    setDigitalOut(configuration->enableHeaterOutput, enabled);
    status->enableHeater = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the heating pump output and set the status flag.
 */
void SystemIO::setHeaterValve(bool enabled) {
    setDigitalOut(configuration->heaterValveOutput, enabled);
    status->heaterValve = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the heating pump output and set the status flag.
 */
void SystemIO::setHeaterPump(bool enabled) {
    setDigitalOut(configuration->heaterPumpOutput, enabled);
    status->heaterPump = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the cooling pump output and set the status flag.
 */
void SystemIO::setCoolingPump(bool enabled) {
    setDigitalOut(configuration->coolingPumpOutput, enabled);
    status->coolingPump = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the cooling fan output and set the status flag.
 */
void SystemIO::setCoolingFan(bool enabled) {
    setDigitalOut(configuration->coolingFanOutput, enabled);
    status->coolingFan = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the brake light output and set the status flag.
 */
void SystemIO::setBrakeLight(bool enabled) {
    setDigitalOut(configuration->brakeLightOutput, enabled);
    status->brakeLight = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the brake light output and set the status flag.
 */
void SystemIO::setReverseLight(bool enabled) {
    setDigitalOut(configuration->reverseLightOutput, enabled);
    status->reverseLight = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the warning light output and set the status flag.
 */
void SystemIO::setWarning(bool enabled) {
    setDigitalOut(configuration->warningOutput, enabled);
    status->warning = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Enable / disable the brake light output and set the status flag.
 */
void SystemIO::setPowerLimitation(bool enabled) {
    setDigitalOut(configuration->powerLimitationOutput, enabled);
    status->limitationTorque = enabled;
    DeviceManager::getInstance()->sendMessage(DEVICE_IO, CANIO, MSG_UPDATE, NULL);
}

/*
 * Polls for the end of an ADC conversion event. Then processes buffer to extract the averaged
 * value. It takes this value and averages it with the existing value in an 8 position buffer
 * which serves as a super fast place for other code to retrieve ADC values.
 * This is only used when RAWADC is not defined.
 */
void SystemIO::ADCPoll() {
    if (obufn != bufn) {
        uint32_t tempbuff[8] = { 0, 0, 0, 0, 0, 0, 0, 0 }; //make sure its zero'd

        //the eight or four enabled adcs are interleaved in the buffer
        //this is a somewhat unrolled for loop with no incrementer. it's odd but it works
        if (useRawADC) {
            for (int i = 0; i < 256;) {
                tempbuff[3] += adcBuffer[obufn][i++];
                tempbuff[2] += adcBuffer[obufn][i++];
                tempbuff[1] += adcBuffer[obufn][i++];
                tempbuff[0] += adcBuffer[obufn][i++];
            }
        } else {
            for (int i = 0; i < 256;) {
                tempbuff[7] += adcBuffer[obufn][i++];
                tempbuff[6] += adcBuffer[obufn][i++];
                tempbuff[5] += adcBuffer[obufn][i++];
                tempbuff[4] += adcBuffer[obufn][i++];
                tempbuff[3] += adcBuffer[obufn][i++];
                tempbuff[2] += adcBuffer[obufn][i++];
                tempbuff[1] += adcBuffer[obufn][i++];
                tempbuff[0] += adcBuffer[obufn][i++];
            }
        }

        //for (int i = 0; i < 256;i++) Logger::debug("%i - %i", i, adcBuf[obufn][i]);

        //now, all of the ADC values are summed over 32/64 readings. So, divide by 32/64 (shift by 5/6) to get the average
        //then add that to the old value we had stored and divide by two to average those. Lots of averaging going on.
        if (useRawADC) {
            for (int j = 0; j < 4; j++) {
                adcValues[j] += (tempbuff[j] >> 6);
                adcValues[j] = adcValues[j] >> 1;
            }
        } else {
            for (int j = 0; j < 8; j++) {
                adcValues[j] += (tempbuff[j] >> 5);
                adcValues[j] = adcValues[j] >> 1;
                //Logger::debug("A%i: %i", j, adc_values[j]);
            }
        }

        for (int i = 0; i < CFG_NUMBER_ANALOG_INPUTS; i++) {
            int val;

            if (useRawADC) {
                val = getRawADC(i);
            } else {
                val = getDifferentialADC(i);
            }
//          addNewADCVal(i, val);
//          adc_out_vals[i] = getADCAvg(i);
            adcOutValues[i] = val;
        }
        obufn = bufn;
    }
}

/*
 * Get value of one of the 4 analog inputs.
 *
 * Uses a special buffer which has smoothed and corrected ADC values.
 * This call is very fast because the actual work is done via DMA and
 * then a separate polled step.
 */
uint16_t SystemIO::getAnalogIn(uint8_t which) {
    if (which >= CFG_NUMBER_ANALOG_INPUTS) {
        which = 0;
    }
    return adcOutValues[which];
}

/*
 * Get value of one of the 4 digital inputs.
 * If input is not configured, false is returned.
 */
boolean SystemIO::getDigitalIn(uint8_t which) {
    if (which >= CFG_NUMBER_DIGITAL_INPUTS) {
        return false;
    }
    if (dig[which] == CFG_OUTPUT_NONE) {
        return false;
    }

    return !(digitalRead(dig[which]));
}

/*
 * Set digital output to high or low.
 */
void SystemIO::setDigitalOut(uint8_t which, boolean active) {
    if (which >= CFG_NUMBER_DIGITAL_OUTPUTS) {
        return;
    }
    if (out[which] == CFG_OUTPUT_NONE) {
        return;
    }

    digitalWrite(out[which], active ? HIGH : LOW);
    status->digitalOutput[which] = active;
    printIOStatus();
}

/*
 * Get current state of digital output (high or low?)
 */
boolean SystemIO::getDigitalOut(uint8_t which) {
    if (which >= CFG_NUMBER_DIGITAL_OUTPUTS) {
        return false;
    }
    if (out[which] == 255) {
        return false;
    }

    return digitalRead(out[which]);
}

/*
 * Some of the boards are differential and thus require subtracting
 * one ADC from another to obtain the true value. This function
 * handles that case. It also applies gain and offset.
 */
uint16_t SystemIO::getDifferentialADC(uint8_t which) {
    uint32_t low, high;

    low = adcValues[adc[which][0]];
    high = adcValues[adc[which][1]];

    if (low < high) {

        //first remove the bias to bring us back to where it rests at zero input volts

        if (low >= adcComp[which].offset) {
            low -= adcComp[which].offset;
        } else {
            low = 0;
        }

        if (high >= adcComp[which].offset) {
            high -= adcComp[which].offset;
        } else {
            high = 0;
        }

        //gain multiplier is 1024 for 1 to 1 gain, less for lower gain, more for higher.
        low *= adcComp[which].gain;
        low = low >> 10; //divide by 1024 again to correct for gain multiplier
        high *= adcComp[which].gain;
        high = high >> 10;

        //Lastly, the input scheme is basically differential so we have to subtract
        //low from high to get the actual value
        high = high - low;
    } else {
        high = 0;
    }

    if (high > 4096) {
        high = 0;    //if it somehow got wrapped anyway then set it back to zero
    }

    return high;
}

/*
 * Exactly like the previous function but for non-differential boards
 * (all the non-prototype boards are non-differential).
 */
uint16_t SystemIO::getRawADC(uint8_t which) {
    uint32_t val;

    val = adcValues[adc[which][0]];

    //first remove the bias to bring us back to where it rests at zero input volts

    if (val >= adcComp[which].offset) {
        val -= adcComp[which].offset;
    } else {
        val = 0;
    }

    //gain multiplier is 1024 for 1 to 1 gain, less for lower gain, more for higher.
    val *= adcComp[which].gain;
    val = val >> 10; //divide by 1024 again to correct for gain multiplier

    if (val > 4096) {
        val = 0;    //if it somehow got wrapped anyway then set it back to zero
    }

    return val;
}

/*
 *  Figure out what hardware we are running on and fill in the pin tables.
 */
void SystemIO::initializePinTables() {
    uint8_t rawadc;
    sysPrefs->read(EESYS_RAWADC, &rawadc);

    if (rawadc != 0) {
        useRawADC = true;
        Logger::info("Using raw ADC mode");
    } else {
        useRawADC = false;
    }

//    numberADCSamples = 64;
    uint8_t sys_type;
    sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
    if (sys_type == 2) {
        initGevcu2PinTable();
    } else if (sys_type == 3) {
        initGevcu3PinTable();
    } else if (sys_type == 4) {
        initGevcu4PinTable();
    } else {
        initGevcuLegacyPinTable();
    }
}

void SystemIO::initGevcu2PinTable() {
    Logger::info("Running on GEVCU2/DUED hardware.");
    dig[0] = 9;
    dig[1] = 11;
    dig[2] = 12;
    dig[3] = 13;
    adc[0][0] = 1;
    adc[0][1] = 0;
    adc[1][0] = 3;
    adc[1][1] = 2;
    adc[2][0] = 5;
    adc[2][1] = 4;
    adc[3][0] = 7;
    adc[3][1] = 6;
    out[0] = 52;
    out[1] = 22;
    out[2] = 48;
    out[3] = 32;
    out[4] = 255;
    out[5] = 255;
    out[6] = 255;
    out[7] = 255;
    //numberADCSamples = 32;
}

void SystemIO::initGevcu3PinTable() {
    Logger::info("Running on GEVCU3 hardware");
    dig[0] = 48;
    dig[1] = 49;
    dig[2] = 50;
    dig[3] = 51;
    adc[0][0] = 3;
    adc[0][1] = 255;
    adc[1][0] = 2;
    adc[1][1] = 255;
    adc[2][0] = 1;
    adc[2][1] = 255;
    adc[3][0] = 0;
    adc[3][1] = 255;
    out[0] = 9;
    out[1] = 8;
    out[2] = 7;
    out[3] = 6;
    out[4] = 255;
    out[5] = 255;
    out[6] = 255;
    out[7] = 255;
    useRawADC = true;
}

void SystemIO::initGevcu4PinTable() {
    Logger::info("Running on GEVCU 4.x hardware");
    dig[0] = 48;
    dig[1] = 49;
    dig[2] = 50;
    dig[3] = 51;
    adc[0][0] = 3;
    adc[0][1] = 255;
    adc[1][0] = 2;
    adc[1][1] = 255;
    adc[2][0] = 1;
    adc[2][1] = 255;
    adc[3][0] = 0;
    adc[3][1] = 255;
    out[0] = 4;
    out[1] = 5;
    out[2] = 6;
    out[3] = 7;
    out[4] = 2;
    out[5] = 3;
    out[6] = 8;
    out[7] = 9;
    useRawADC = true;
}

void SystemIO::initGevcuLegacyPinTable() {
    Logger::info("Running on legacy hardware?");
    dig[0] = 11;
    dig[1] = 9;
    dig[2] = 13;
    dig[3] = 12;
    adc[0][0] = 1;
    adc[0][1] = 0;
    adc[1][0] = 2;
    adc[1][1] = 3;
    adc[2][0] = 4;
    adc[2][1] = 5;
    adc[3][0] = 7;
    adc[3][1] = 6;
    out[0] = 52;
    out[1] = 22;
    out[2] = 48;
    out[3] = 32;
    out[4] = 255;
    out[5] = 255;
    out[6] = 255;
    out[7] = 255;
//    numberADCSamples = 32;
}

/*
 * Forces the digital I/O ports to a safe state.
 */
void SystemIO::initializeDigitalIO() {
    int i;

    for (i = 0; i < CFG_NUMBER_DIGITAL_INPUTS; i++) {
        pinMode(dig[i], INPUT);
    }

    for (i = 0; i < CFG_NUMBER_DIGITAL_OUTPUTS; i++) {
        if (out[i] != 255) {
            pinMode(out[i], OUTPUT);
            digitalWrite(out[i], LOW);
        }
    }
}

/*
 * Initialize DMA driven ADC and read in gain/offset for each channel.
 */
void SystemIO::initializeAnalogIO() {

    setupFastADC();

    //requires the value to be contiguous in memory
    for (int i = 0; i < CFG_NUMBER_ANALOG_INPUTS; i++) {
        sysPrefs->read(EESYS_ADC0_GAIN + 4 * i, &adcComp[i].gain);
        sysPrefs->read(EESYS_ADC0_OFFSET + 4 * i, &adcComp[i].offset);

        //Logger::debug("ADC:%d GAIN: %d Offset: %d", i, adc_comp[i].gain, adc_comp[i].offset);
//        for (int j = 0; j < numberADCSamples; j++) {
//            adcAverageBuffer[i][j] = 0;
//        }
//
//        adcPointer[i] = 0;
        adcValues[i] = 0;
        adcOutValues[i] = 0;
    }
}

/*
 * Setup the system to continuously read the proper ADC channels and use DMA to place the results into RAM
 * Testing to find a good batch of settings for how fast to do ADC readings. The relevant areas:
 * 1. In the adc_init call it is possible to use something other than ADC_FREQ_MAX to slow down the ADC clock
 * 2. ADC_MR has a clock divisor, start up time, settling time, tracking time, and transfer time. These can be adjusted
 */
void SystemIO::setupFastADC() {
    pmc_enable_periph_clk(ID_ADC);
    adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST); //just about to change a bunch of these parameters with the next command

    /*
     The MCLK is 12MHz on our boards. The ADC can only run 1MHz so the prescaler must be at least 12x.
     The ADC should take Tracking+Transfer for each read when it is set to switch channels with each read

     Example:
     5+7 = 12 clocks per read 1M / 12 = 83333 reads per second. For newer boards there are 4 channels interleaved
     so, for each channel, the readings are 48uS apart. 64 of these readings are averaged together for a total of 3ms
     worth of ADC in each average. This is then averaged with the current value in the ADC buffer that is used for output.

     If, for instance, someone wanted to average over 6ms instead then the prescaler could be set to 24x instead.
     */
    ADC->ADC_MR = (1 << 7) //free running
    + (5 << 8) //12x MCLK divider ((This value + 1) * 2) = divisor
            + (1 << 16) //8 periods start up time (0=0clks, 1=8clks, 2=16clks, 3=24, 4=64, 5=80, 6=96, etc)
            + (1 << 20) //settling time (0=3clks, 1=5clks, 2=9clks, 3=17clks)
            + (4 << 24) //tracking time (Value + 1) clocks
            + (2 << 28); //transfer time ((Value * 2) + 3) clocks

    if (useRawADC) {
        ADC->ADC_CHER = 0xF0;    //enable A0-A3
    } else {
        ADC->ADC_CHER = 0xFF;    //enable A0-A7
    }

    NVIC_EnableIRQ(ADC_IRQn);
    ADC->ADC_IDR = ~(1 << 27);  //dont disable the ADC interrupt for rx end
    ADC->ADC_IER = 1 << 27; //do enable it
    ADC->ADC_RPR = (uint32_t) adcBuffer[0]; // DMA buffer
    ADC->ADC_RCR = 256; //# of samples to take
    ADC->ADC_RNPR = (uint32_t) adcBuffer[1]; // next DMA buffer
    ADC->ADC_RNCR = 256; //# of samples to take
    bufn = obufn = 0;
    ADC->ADC_PTCR = 1; //enable dma mode
    ADC->ADC_CR = 2; //start conversions

    Logger::debug("Fast ADC Mode Enabled");
}

void SystemIO::printIOStatus() {
    if (Logger::isDebug()) {
        Logger::debug("AIN0: %d, AIN1: %d, AIN2: %d, AIN3: %d", getAnalogIn(0), getAnalogIn(1), getAnalogIn(2), getAnalogIn(3));
        Logger::debug("DIN0: %t, DIN1: %t, DIN2: %t, DIN3: %t", getDigitalIn(0), getDigitalIn(1), getDigitalIn(2), getDigitalIn(3));
        Logger::debug("DOUT0: %t, DOUT1: %t, DOUT2: %t, DOUT3: %t,DOUT4: %t, DOUT5: %t, DOUT6: %t, DOUT7: %t", getDigitalOut(0), getDigitalOut(1),
                getDigitalOut(2), getDigitalOut(3), getDigitalOut(4), getDigitalOut(5), getDigitalOut(6), getDigitalOut(7));
    }
}

/*
 * Move DMA pointers to next buffer.
 */
uint32_t SystemIO::getNextADCBuffer() {
    bufn = (bufn + 1) & 3;
    return (uint32_t) adcBuffer[bufn];
}

/*
 * When the ADC reads in the programmed # of readings it will do two things:
 * 1. It loads the next buffer and buffer size into current buffer and size
 * 2. It sends this interrupt
 * This interrupt then loads the "next" fields with the proper values. This is
 * done with a four position buffer. In this way the ADC is constantly sampling
 * and this happens virtually for free. It all happens in the background with
 * minimal CPU overhead.
 */
void ADC_Handler() {
    int f = ADC->ADC_ISR;

    if (f & (1 << 27)) { //receive counter end of buffer
        ADC->ADC_RNPR = SystemIO::getInstance()->getNextADCBuffer();
        ADC->ADC_RNCR = 256;
    }
}

/*
 Adds a new ADC reading to the buffer for a channel. The buffer is NumADCSamples large (either 32 or 64) and rolling
 */
//void addNewADCVal(uint8_t which, uint16_t val)
//{
//    adcAverageBuffer[which][adcPointer[which]] = val;
//    adcPointer[which] = (adcPointer[which] + 1) % numberADCSamples;
//}
/*
 Take the arithmetic average of the readings in the buffer for each channel. This smooths out the ADC readings
 */
//uint16_t getADCAvg(uint8_t which)
//{
//    uint32_t sum;
//    sum = 0;
//
//    for (int j = 0; j < numberADCSamples; j++) {
//        sum += adcAverageBuffer[which][j];
//    }
//
//    sum = sum / numberADCSamples;
//    return ((uint16_t) sum);
//}
SystemIOConfiguration *SystemIO::getConfiguration() {
    return configuration;
}

void SystemIO::loadConfiguration() {
    Logger::info("System I/O configuration:");

#ifdef USE_HARD_CODED
    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        Logger::debug((char *) Constants::validChecksum);
        prefsHandler->read(EESYS_ENABLE_INPUT, &configuration->enableInput);
        prefsHandler->read(EESYS_CHARGE_POWER_AVAILABLE_INPUT, &configuration->chargePowerAvailableInput);
        prefsHandler->read(EESYS_INTERLOCK_INPUT, &configuration->interlockInput);

        prefsHandler->read(EESYS_PRECHARGE_MILLIS, &configuration->prechargeMillis);
        prefsHandler->read(EESYS_PRECHARGE_RELAY_OUTPUT, &configuration->prechargeRelayOutput);
        prefsHandler->read(EESYS_MAIN_CONTACTOR_OUTPUT, &configuration->mainContactorOutput);
        prefsHandler->read(EESYS_SECONDARY_CONTACTOR_OUTPUT, &configuration->secondaryContactorOutput);
        prefsHandler->read(EESYS_FAST_CHARGE_CONTACTOR_OUTPUT, &configuration->fastChargeContactorOutput);

        prefsHandler->read(EESYS_ENABLE_MOTOR_OUTPUT, &configuration->enableMotorOutput);
        prefsHandler->read(EESYS_ENABLE_CHARGER_OUTPUT, &configuration->enableChargerOutput);
        prefsHandler->read(EESYS_ENABLE_DCDC_OUTPUT, &configuration->enableDcDcOutput);
        prefsHandler->read(EESYS_ENABLE_HEATER_OUTPUT, &configuration->enableHeaterOutput);

        prefsHandler->read(EESYS_HEATER_VALVE_OUTPUT, &configuration->heaterValveOutput);
        prefsHandler->read(EESYS_HEATER_PUMP_OUTPUT, &configuration->heaterPumpOutput);
        prefsHandler->read(EESYS_COOLING_PUMP_OUTPUT, &configuration->coolingPumpOutput);
        prefsHandler->read(EESYS_COOLING_FAN_OUTPUT, &configuration->coolingFanOutput);
        prefsHandler->read(EESYS_COOLING_TEMP_ON, &configuration->coolingTempOn);
        prefsHandler->read(EESYS_COOLING_TEMP_OFF, &configuration->coolingTempOff);

        prefsHandler->read(EESYS_BRAKE_LIGHT_OUTPUT, &configuration->brakeLightOutput);
        prefsHandler->read(EESYS_REVERSE_LIGHT_OUTPUT, &configuration->reverseLightOutput);
        prefsHandler->read(EESYS_WARNING_OUTPUT, &configuration->warningOutput);
        prefsHandler->read(EESYS_POWER_LIMITATION_OUTPUT, &configuration->powerLimitationOutput);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn((char *) Constants::invalidChecksum);
        configuration->enableInput = EnableInput;
        configuration->chargePowerAvailableInput = CFG_OUTPUT_NONE;
        configuration->interlockInput = InterlockInput;

        configuration->prechargeMillis = PrechargeMillis;
        configuration->prechargeRelayOutput = PrechargeRelayOutput;
        configuration->mainContactorOutput = MainContactorRelayOutput;
        configuration->secondaryContactorOutput = SecondaryContactorRelayOutput;
        configuration->fastChargeContactorOutput = CFG_OUTPUT_NONE;

        configuration->enableMotorOutput = EnableRelayOutput;
        configuration->enableChargerOutput = CFG_OUTPUT_NONE;
        configuration->enableDcDcOutput = CFG_OUTPUT_NONE;
        configuration->enableHeaterOutput = CFG_OUTPUT_NONE;

        configuration->heaterValveOutput = CFG_OUTPUT_NONE;
        configuration->heaterPumpOutput = CFG_OUTPUT_NONE;
        configuration->coolingPumpOutput = CFG_OUTPUT_NONE;
        configuration->coolingFanOutput = CoolingFanRelayOutput;
        configuration->coolingTempOn = CoolingTemperatureOn;
        configuration->coolingTempOff = CoolingTemperatureOff;

        configuration->brakeLightOutput = BrakeLightOutput;
        configuration->reverseLightOutput = ReverseLightOutput;
        configuration->warningOutput = CFG_OUTPUT_NONE;
        configuration->powerLimitationOutput = CFG_OUTPUT_NONE;
        saveConfiguration();
    }
    Logger::info("GEVCU enable input: %d, charge power avail input: %d, interlock input: %d", configuration->enableInput, configuration->chargePowerAvailableInput, configuration->interlockInput);
    Logger::info("pre-charge milliseconds: %d, pre-charge relay: %d, main contactor: %d", configuration->prechargeMillis, configuration->prechargeRelayOutput, configuration->mainContactorOutput);
    Logger::info("secondary contactor: %d, fast charge contactor: %d", configuration->secondaryContactorOutput, configuration->fastChargeContactorOutput);
    Logger::info("enable motor: %d, enable charger: %d, enable DCDC: %d, enable heater: %d", configuration->enableMotorOutput, configuration->enableChargerOutput, configuration->enableDcDcOutput, configuration->enableHeaterOutput);
    Logger::info("heater valve: %d, heater pump: %d", configuration->heaterValveOutput, configuration->heaterPumpOutput);
    Logger::info("cooling pump: %d, cooling fan: %d, cooling temperature ON: %d, cooling tempreature Off: %d", configuration->coolingPumpOutput, configuration->coolingFanOutput, configuration->coolingTempOn, configuration->coolingTempOff);
    Logger::info("brake light: %d, reverse light: %d, warning: %d, power limitation: %d", configuration->brakeLightOutput, configuration->reverseLightOutput, configuration->warningOutput, configuration->powerLimitationOutput);
}

void SystemIO::saveConfiguration() {
    prefsHandler->write(EESYS_ENABLE_INPUT, configuration->enableInput);
    prefsHandler->write(EESYS_CHARGE_POWER_AVAILABLE_INPUT, configuration->chargePowerAvailableInput);
    prefsHandler->write(EESYS_INTERLOCK_INPUT, configuration->interlockInput);

    prefsHandler->write(EESYS_PRECHARGE_MILLIS, configuration->prechargeMillis);
    prefsHandler->write(EESYS_PRECHARGE_RELAY_OUTPUT, configuration->prechargeRelayOutput);
    prefsHandler->write(EESYS_MAIN_CONTACTOR_OUTPUT, configuration->mainContactorOutput);
    prefsHandler->write(EESYS_SECONDARY_CONTACTOR_OUTPUT, configuration->secondaryContactorOutput);
    prefsHandler->write(EESYS_FAST_CHARGE_CONTACTOR_OUTPUT, configuration->fastChargeContactorOutput);

    prefsHandler->write(EESYS_ENABLE_MOTOR_OUTPUT, configuration->enableMotorOutput);
    prefsHandler->write(EESYS_ENABLE_CHARGER_OUTPUT, configuration->enableChargerOutput);
    prefsHandler->write(EESYS_ENABLE_DCDC_OUTPUT, configuration->enableDcDcOutput);
    prefsHandler->write(EESYS_ENABLE_HEATER_OUTPUT, configuration->enableHeaterOutput);

    prefsHandler->write(EESYS_HEATER_VALVE_OUTPUT, configuration->heaterValveOutput);
    prefsHandler->write(EESYS_HEATER_PUMP_OUTPUT, configuration->heaterPumpOutput);
    prefsHandler->write(EESYS_COOLING_PUMP_OUTPUT, configuration->coolingPumpOutput);
    prefsHandler->write(EESYS_COOLING_FAN_OUTPUT, configuration->coolingFanOutput);
    prefsHandler->write(EESYS_COOLING_TEMP_ON, configuration->coolingTempOn);
    prefsHandler->write(EESYS_COOLING_TEMP_OFF, configuration->coolingTempOff);

    prefsHandler->write(EESYS_BRAKE_LIGHT_OUTPUT, configuration->brakeLightOutput);
    prefsHandler->write(EESYS_REVERSE_LIGHT_OUTPUT, configuration->reverseLightOutput);
    prefsHandler->write(EESYS_WARNING_OUTPUT, configuration->warningOutput);
    prefsHandler->write(EESYS_POWER_LIMITATION_OUTPUT, configuration->powerLimitationOutput);

    prefsHandler->saveChecksum();
}
