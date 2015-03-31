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

#undef HID_ENABLED

extern PrefHandler *sysPrefs;

/*
 * Constructor
 */
SystemIO::SystemIO() {
    configuration = new SystemIOConfiguration();
    prefsHandler = new PrefHandler(SYSTEM);
    canHandlerEv = CanHandler::getInstanceEV();
    status = Status::getInstance();
    preChargeStart = 0;
    coolflag = false;
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

    canHandlerEv->attach(this, CAN_MASKED_ID, CAN_MASK, false);

    TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_SYSTEM_IO);
}

/*
 * Processes an event from the CanHandler.
 */
void SystemIO::handleCanFrame(CAN_FRAME *frame) {
    switch (frame->id) {
    case CAN_ID_GEVCU_EXT_TEMPERATURE:
        processExternalTemperature(frame->data.byte);
        break;
    }
}


void SystemIO::handleTick() {
    if(status->getSystemState() == Status::preCharge) {
        handlePreCharge();
    }
    if (configuration->coolingFanOutput != CFG_OUTPUT_NONE) {
        handleCooling();
    }
    updateDigitalInputStatus();

    sendIOStatus();
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

/*
 * Send the status of the IO over CAN so it can be used by other devices.
 */
void SystemIO::sendIOStatus() {
    canHandlerEv->prepareOutputFrame(&outputFrame, CAN_ID_GEVCU_STATUS);

    uint16_t rawIO = 0;
    rawIO |= status->digitalInput[0] ? digitalIn1 : 0;
    rawIO |= status->digitalInput[1] ? digitalIn2 : 0;
    rawIO |= status->digitalInput[2] ? digitalIn3 : 0;
    rawIO |= status->digitalInput[3] ? digitalIn4 : 0;
    rawIO |= status->digitalOutput[0] ? digitalOut1 : 0;
    rawIO |= status->digitalOutput[1] ? digitalOut2 : 0;
    rawIO |= status->digitalOutput[2] ? digitalOut3 : 0;
    rawIO |= status->digitalOutput[3] ? digitalOut4 : 0;
    rawIO |= status->digitalOutput[4] ? digitalOut5 : 0;
    rawIO |= status->digitalOutput[5] ? digitalOut6 : 0;
    rawIO |= status->digitalOutput[6] ? digitalOut7 : 0;
    rawIO |= status->digitalOutput[7] ? digitalOut8 : 0;

    outputFrame.data.byte[0] = (rawIO & 0xFF00) >> 8;
    outputFrame.data.byte[1] = (rawIO & 0x00FF);

    uint16_t logicIO = 0;
//    logicIO |= .. ? heatingPump : 0;
//    logicIO |= .. ? batteryHeater : 0;
//    logicIO |= .. ? chargePowerAvailable : 0;
//    logicIO |= .. ? activateCharger : 0;
    logicIO |= status->reverseLight ? reverseLight : 0;
    logicIO |= status->brakeLight ? brakeLight : 0;
//    logicIO |= .. ? coolingPump : 0;
    logicIO |= status->coolingFanRelay ? coolingFan : 0;
    logicIO |= status->secondaryContactorRelay ? secondayContactor : 0;
    logicIO |= status->mainContactorRelay ? mainContactor : 0;
    logicIO |= status->preChargeRelay ? preChargeRelay : 0;
    logicIO |= status->enableOut ? enableSignalOut : 0;
    logicIO |= status->enableIn ? enableSignalIn : 0;

    outputFrame.data.byte[2] = (logicIO & 0xFF00) >> 8;
    outputFrame.data.byte[3] = (logicIO & 0x00FF);

    outputFrame.data.byte[4] = status->getSystemState();

    uint8_t stat = 0;
    stat |= status->warning ? warning : 0;
    stat |= status->limitationTorque ? powerLimitation : 0;
    outputFrame.data.byte[5] = stat;

    canHandlerEv->sendFrame(outputFrame);
}


/*
 * Handle the pre-charge sequence.
 */
void SystemIO::handlePreCharge() {
    if (configuration->prechargeMillis == 0) { // we don't want to pre-charge
        Logger::info("Pre-charging not enabled");
        status->setSystemState(Status::preCharged);
        setEnableRelayOutput(true);
        return;
    }

    if (preChargeStart == 0) {
        Logger::info("Starting pre-charge sequence");
        printIOStatus();

        preChargeStart = millis();
        setPrechargeRelayOutput(true);

#ifdef CFG_THREE_CONTACTOR_PRECHARGE
        if (configuration->secondaryContactorOutput != CFG_OUTPUT_NONE) {
            delay(CFG_PRE_CHARGE_RELAY_DELAY);
            setSecondaryContactorRelayOutput(true);
        }
#endif
    } else {
        if ((millis() - preChargeStart) > configuration->prechargeMillis) {
            setMainContactorRelayOutput(true);
            delay(CFG_PRE_CHARGE_RELAY_DELAY);
            setPrechargeRelayOutput(false);

            status->setSystemState(Status::preCharged);
            Logger::info("Pre-charge sequence complete after %i milliseconds", millis() - preChargeStart);

            setEnableRelayOutput(true);
        }
    }
}

/*
 * Control an optional cooling fan output depending on the temperature of
 * the motor controller
 */
void SystemIO::handleCooling() {
    if (status->temperatureController / 10 > configuration->coolingTempOn) {
        if (!coolflag) {
            coolflag = true;
            setCoolingFanRelayOutput(true);
            // enabling cooling does not necessarily mean overtemp, the motor controller object should set the overtemp status
            // Status::getInstance()->overtempController = true;
        }
    }

    if (status->temperatureController / 10 < configuration->coolingTempOff) {
        if (coolflag) {
            coolflag = false;
            setCoolingFanRelayOutput(false);
            // enabling cooling does not necessarily mean overtemp, the motor controller object should set the overtemp status
            // Status::getInstance()->overtempController = false;
        }
    }
}

void SystemIO::processExternalTemperature(byte bytes[]) {
	for (int i = 0; i < 8; i++) {
		status->externalTemperature[i] = bytes[i] - 50;
Logger::info("external temperature %d: %d", i, status->externalTemperature[i]);
	}
}

/*
 * Get the the input signal for the car's enable signal.
 */
bool SystemIO::getEnableInput() {
    if (configuration->enableInput != CFG_OUTPUT_NONE) {
        bool flag = getDigitalIn(configuration->enableInput);
        status->enableIn = flag;
        return flag;
    }
    return false;
}

/*
 * Enable / disable the pre-charge relay output and set the status flag.
 */
void SystemIO::setPrechargeRelayOutput(bool enabled) {
    if (configuration->prechargeOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->prechargeOutput, enabled);
    }
    status->preChargeRelay = enabled;
    sendIOStatus();
}

/*
 * Enable / disable the main contactor relay output and set the status flag.
 */
void SystemIO::setMainContactorRelayOutput(bool enabled) {
    if (configuration->mainContactorOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->mainContactorOutput, enabled);
    }
    status->mainContactorRelay = enabled;
    sendIOStatus();
}

/*
 * Enable / disable the secondary contactor relay output and set the status flag.
 */
void SystemIO::setSecondaryContactorRelayOutput(bool enabled) {
    if (configuration->secondaryContactorOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->secondaryContactorOutput, enabled);
    }
    status->secondaryContactorRelay = enabled;
    sendIOStatus();
}

/*
 * Enable / disable the 'enable' relay output and set the status flag.
 */
void SystemIO::setEnableRelayOutput(bool enabled) {
    if (configuration->enableOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->enableOutput, enabled);
    }
    status->enableOut = enabled;
    sendIOStatus();
}

/*
 * Enable / disable the brake light output and set the status flag.
 */
void SystemIO::setBrakeLightOutput(bool enabled) {
    if (configuration->brakeLightOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->brakeLightOutput, enabled);
    }
    status->brakeLight = enabled;
    sendIOStatus();
}

/*
 * Enable / disable the brake light output and set the status flag.
 */
void SystemIO::setReverseLightOutput(bool enabled) {
    if (configuration->reverseLightOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->reverseLightOutput, enabled);
    }
    status->reverseLight = enabled;
    sendIOStatus();
}


/*
 * Enable / disable the cooling realy output and set the status flag.
 */
void SystemIO::setCoolingFanRelayOutput(bool enabled) {
    if (configuration->coolingFanOutput != CFG_OUTPUT_NONE) {
        setDigitalOut(configuration->coolingFanOutput, enabled);
    }
    status->coolingFanRelay = enabled;
    sendIOStatus();
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
 */
boolean SystemIO::getDigitalIn(uint8_t which) {
    if (which >= CFG_NUMBER_DIGITAL_INPUTS) {
        which = 0;
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
    if (out[which] == 255) {
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
        Logger::debug("AIN0: %d, AIN1: %d, AIN2: %d, AIN3: %d", getAnalogIn(0),
                getAnalogIn(1), getAnalogIn(2), getAnalogIn(3));
        Logger::debug("DIN0: %d, DIN1: %d, DIN2: %d, DIN3: %d", getDigitalIn(0),
                getDigitalIn(1), getDigitalIn(2), getDigitalIn(3));
        Logger::debug(
                "DOUT0: %d, DOUT1: %d, DOUT2: %d, DOUT3: %d,DOUT4: %d, DOUT5: %d, DOUT6: %d, DOUT7: %d",
                getDigitalOut(0), getDigitalOut(1), getDigitalOut(2), getDigitalOut(3),
                getDigitalOut(4), getDigitalOut(5), getDigitalOut(6), getDigitalOut(7));
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
#ifdef USE_HARD_CODED
    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        prefsHandler->read(EESYS_ENABLE_INPUT, &configuration->enableInput);
        prefsHandler->read(EESYS_PRECHARGE_MILLIS, &configuration->prechargeMillis);
        prefsHandler->read(EESYS_PRECHARGE_OUTPUT, &configuration->prechargeOutput);
        prefsHandler->read(EESYS_MAIN_CONTACTOR_OUTPUT, &configuration->mainContactorOutput);
        prefsHandler->read(EESYS_SECONDARY_CONTACTOR_OUTPUT, &configuration->secondaryContactorOutput);
        prefsHandler->read(EESYS_ENABLE_OUTPUT, &configuration->enableOutput);
        prefsHandler->read(EESYS_COOLING_FAN_RELAY, &configuration->coolingFanOutput);
        prefsHandler->read(EESYS_COOLING_TEMP_ON, &configuration->coolingTempOn);
        prefsHandler->read(EESYS_COOLING_TEMP_OFF, &configuration->coolingTempOff);
        prefsHandler->read(EESYS_BRAKE_LIGHT, &configuration->brakeLightOutput);
        prefsHandler->read(EESYS_REVERSE_LIGHT, &configuration->reverseLightOutput);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        configuration->enableInput = EnableInput;
        configuration->prechargeMillis = PrechargeMillis;
        configuration->prechargeOutput = PrechargeRelayOutput;
        configuration->mainContactorOutput = MainContactorRelayOutput;
        configuration->secondaryContactorOutput = SecondaryContactorRelayOutput;
        configuration->enableOutput = EnableRelayOutput;
        configuration->coolingFanOutput = CoolingFanRelayOutput;
        configuration->coolingTempOn = CoolingTemperatureOn;
        configuration->coolingTempOff = CoolingTemperatureOff;
        configuration->brakeLightOutput = BrakeLightOutput;
        configuration->reverseLightOutput = ReverseLightOutput;
    }
}

void SystemIO::saveConfiguration() {
    prefsHandler->write(EESYS_ENABLE_INPUT, configuration->enableInput);
    prefsHandler->write(EESYS_PRECHARGE_MILLIS, configuration->prechargeMillis);
    prefsHandler->write(EESYS_PRECHARGE_OUTPUT, configuration->prechargeOutput);
    prefsHandler->write(EESYS_MAIN_CONTACTOR_OUTPUT, configuration->mainContactorOutput);
    prefsHandler->write(EESYS_SECONDARY_CONTACTOR_OUTPUT, configuration->secondaryContactorOutput);
    prefsHandler->write(EESYS_ENABLE_OUTPUT, configuration->enableOutput);
    prefsHandler->write(EESYS_COOLING_FAN_RELAY, configuration->coolingFanOutput);
    prefsHandler->write(EESYS_COOLING_TEMP_ON, configuration->coolingTempOn);
    prefsHandler->write(EESYS_COOLING_TEMP_OFF, configuration->coolingTempOff);
    prefsHandler->write(EESYS_BRAKE_LIGHT, configuration->brakeLightOutput);
    prefsHandler->write(EESYS_REVERSE_LIGHT, configuration->reverseLightOutput);
    prefsHandler->saveChecksum();
}
