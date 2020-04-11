/*
 * BrusaBSC6.cpp
 *
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

#include "BrusaBSC6.h"

/*
 * Constructor
 */
BrusaBSC6::BrusaBSC6() : DcDcConverter()
{
    prefsHandler = new PrefHandler(BRUSA_BSC6);
    commonName = "Brusa BSC6 DC-DC Converter";

    mode = 0;
    lvCurrentAvailable = 0;
    temperatureBuckBoostSwitch1 = 0;
    temperatureBuckBoostSwitch2 = 0;
    temperatureHvTrafostageSwitch1 = 0;
    temperatureHvTrafostageSwitch2 = 0;
    temperatureLvTrafostageSwitch1 = 0;
    temperatureLvTrafostageSwitch2 = 0;
    temperatureTransformerCoil1 = 0;
    temperatureTransformerCoil2 = 0;
    internal12VSupplyVoltage = 0;
    lsActualVoltage = 0;
    lsActualCurrent = 0;
    lsCommandedCurrent = 0;
    internalOperationState = 0;
    bitfield = 0;
}

/**
 * Tear down the controller in a safe way.
 */
void BrusaBSC6::tearDown()
{
    DcDcConverter::tearDown();
    canHandlerEv.detach(this, BSC6_CAN_MASKED_ID, BSC6_CAN_MASK);
    sendControl(); // as powerOn is false now, send last command to deactivate controller
}

void BrusaBSC6::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    bool powerOnBefore = powerOn;

    DcDcConverter::handleStateChange(oldState, newState);

    if (powerOnBefore != powerOn) {
        if (powerOn) {
            // register ourselves as observer of 0x26a-0x26f can frames
            canHandlerEv.attach(this, BSC6_CAN_MASKED_ID, BSC6_CAN_MASK, false);
            tickHandler.attach(this, CFG_TICK_INTERVAL_DCDC_BSC6);
        } else {
            tearDown();
        }
    }
}

/*
 * Process event from the tick handler.
 */
void BrusaBSC6::handleTick()
{
    DcDcConverter::handleTick(); // call parent

    sendControl();
    sendLimits();
}

/*
 * Send BSC6COM message to the DCDC converter.
 *
 * The message is used to set the operation mode, enable the converter
 * and set the voltage limits.
 */
void BrusaBSC6::sendControl()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    outputFrameControl.data.bytes[0] =
            ((ready || running) && powerOn ? enable : 0) |
            (config->mode == 1 ? boostMode : 0) |
            (config->debugMode ? debugMode : 0);

    canHandlerEv.sendFrame(outputFrameControl);
}

/*
 * Send BSC6LIM message to DCDC converter.
 *
 * This message controls the electrical limits in the converter.
 */
void BrusaBSC6::sendLimits()
{
    canHandlerEv.sendFrame(outputFrameLimits);
}

/*
 * Processes an event from the CanHandler.
 *
 * In case a CAN message was received which pass the masking and id filtering,
 * this method is called. Depending on the ID of the CAN message, the data of
 * the incoming message is processed.
 */
void BrusaBSC6::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
        case BSC6_CAN_ID_VALUES_1:
            processValues1(frame->data.bytes);
            break;
        case BSC6_CAN_ID_VALUES_2:
            processValues2(frame->data.bytes);
            break;
        case BSC6_CAN_ID_DEBUG_1:
            processDebug1(frame->data.bytes);
            break;
        case BSC6_CAN_ID_DEBUG_2:
            processDebug2(frame->data.bytes);
            break;
    }
}

/*
 * Process a BSC6VAL1 message which was received from the converter.
 *
 * This message provides the general status of the converter as well as
 * available high/low voltage current and voltage.
 */
void BrusaBSC6::processValues1(uint8_t data[])
{
    bitfield = (uint32_t)data[7] & 0x0F;

    running = (bitfield & bsc6Running) ? true : false;
    status.dcdcRunning = running;
    ready = (bitfield & bsc6Ready) ? true : false;
    if (bitfield & automatic) {
        //TODO implement handling of automatic flag
    }

    hvVoltage = (uint16_t)(data[1] | (data[0] << 8));
    lvVoltage = (uint8_t)data[2];
    hvCurrent = (int16_t)(data[4] | (data[3] << 8)) - 250;
    lvCurrent = (int16_t)(data[6] | (data[5] << 8)) - 280;
    mode = (data[7] & 0xF0) >> 4;

    if (logger.isDebug()) {
        logger.debug(this, "status bitfield: %#08x, ready: %d, running: %d, HV: %fV %fA, LV: %fV %dA, mode %d", bitfield, ready, running, (float) hvVoltage / 10.0F, (float) hvCurrent / 10.0F, (float) lvVoltage / 10.0F, lvCurrent, mode);
    }
}

/*
 * Process a BSC6VAL2 message which was received from the converter.
 *
 * This message provides (critical) errors.
 */
void BrusaBSC6::processValues2(uint8_t data[])
{
    lvCurrentAvailable = (uint8_t)data[0];
    temperature = (uint8_t)data[1] * 10;

    bitfield = (uint32_t)((data[3] << 0) | (data[2] << 8) | (data[4] << 16));

    if (bitfield != 0) {
        String error;

        appendMessage(error, bitfield, lowVoltageUndervoltage, "LV under-voltage");
        appendMessage(error, bitfield, lowVoltageOvervoltage, "LV over-voltage");
        appendMessage(error, bitfield, highVoltageUndervoltage, "HV under-voltage");
        appendMessage(error, bitfield, highVoltageOvervoltage, "HV over-voltage");
        appendMessage(error, bitfield, internalSupply, "internal supply");
        appendMessage(error, bitfield, temperatureSensor, "temperature sensor");
        appendMessage(error, bitfield, trafoStartup, "trafo startup");
        appendMessage(error, bitfield, overTemperature, "over temperature");
        appendMessage(error, bitfield, highVoltageFuse, "HV fuse");
        appendMessage(error, bitfield, lowVoltageFuse, "LV fuse");
        appendMessage(error, bitfield, currentSensorLowSide, "current sensor low side");
        appendMessage(error, bitfield, currentDeviation, "current deviation");
        appendMessage(error, bitfield, interLock, "interlock");
        appendMessage(error, bitfield, internalSupply12V, "internal supply 12V");
        appendMessage(error, bitfield, internalSupply6V, "internal supply 6V");
        appendMessage(error, bitfield, voltageDeviation, "voltage deviation");
        appendMessage(error, bitfield, invalidValue, "invalid value");
        appendMessage(error, bitfield, commandMessageLost, "cmd msg lost");
        appendMessage(error, bitfield, limitMessageLost, "limit msg lost");
        appendMessage(error, bitfield, crcErrorNVSRAM, "CRC error NVSRAM");
        appendMessage(error, bitfield, brokenTemperatureSensor, "broken temp sensor");

        logger.error(this, "error (%#08x): %s", bitfield, error.c_str());
    }
    if (logger.isDebug()) {
        logger.debug(this, "LV current avail: %dA, maximum Temperature: %.1fC", lvCurrentAvailable, (float) temperature / 10.0F);
    }
}

/*
 * Process a BSC6DBG1 message which was received from the converter.
 *
 * This message provides various temperature readings.
 */
void BrusaBSC6::processDebug1(uint8_t data[])
{
    temperatureBuckBoostSwitch1 = (uint8_t)data[0];
    temperatureBuckBoostSwitch2 = (uint8_t)data[1];
    temperatureHvTrafostageSwitch1 = (uint8_t)data[2];
    temperatureHvTrafostageSwitch2 = (uint8_t)data[3];
    temperatureLvTrafostageSwitch1 = (uint8_t)data[4];
    temperatureLvTrafostageSwitch2 = (uint8_t)data[5];
    temperatureTransformerCoil1 = (uint8_t)data[6];
    temperatureTransformerCoil2 = (uint8_t)data[7];

    if (logger.isDebug()) {
        logger.debug(this, "Temp buck/boost switch 1: %d�C, switch 2: %d�C", temperatureBuckBoostSwitch1, temperatureBuckBoostSwitch2);
        logger.debug(this, "Temp HV trafostage switch 1: %d�C, switch 2: %d�C", temperatureHvTrafostageSwitch1, temperatureHvTrafostageSwitch2);
        logger.debug(this, "Temp LV trafostage switch 1: %d�C, switch 2: %d�C", temperatureLvTrafostageSwitch1, temperatureLvTrafostageSwitch2);
        logger.debug(this, "Temp transformer coil 1: %d�C, coil 2: %d�C", temperatureTransformerCoil1, temperatureTransformerCoil2);
    }
}

/*
 * Process a BSC6DBG2 message which was received from the converter.
 *
 * This message provides various temperature readings.
 */
void BrusaBSC6::processDebug2(uint8_t data[])
{
    internal12VSupplyVoltage = (uint8_t)data[0];
    lsActualVoltage = (uint8_t)data[1];
    lsActualCurrent = (int16_t)data[2] - 6000;
    lsCommandedCurrent = (int16_t)data[3] - 6000;
    internalOperationState = (uint8_t)data[4];

    if (logger.isDebug()) {
        logger.debug(this, "internal 12V supply: %.1fV, LS actual voltage: %.1fV", (float) internal12VSupplyVoltage / 10.0F, (float) lsActualVoltage / 10.0F);
        logger.debug(this, "LS actual current: %.2fA, LS commanded current: %.2fA", (float) internal12VSupplyVoltage / 200.0F, (float) lsActualVoltage / 200.0F);
        logger.debug(this, "internal power state: %d", internalOperationState);
    }
}

/**
 * Prepare the content of the CAN frames sent to the BSC6 so they don't have to be filled in every tick cycle
 */
void BrusaBSC6::prepareCanMessages()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    canHandlerEv.prepareOutputFrame(&outputFrameControl, BSC6_CAN_ID_COMMAND);
    outputFrameControl.data.bytes[1] = constrain(config->lowVoltageCommand, 80, 160); // 8-16V in 0.1V, offset = 0V
    outputFrameControl.data.bytes[2] = constrain(config->highVoltageCommand - 170, 20, 255); // 190-425V in 1V, offset = 170V
    outputFrameControl.length = 3;

    canHandlerEv.prepareOutputFrame(&outputFrameLimits, BSC6_CAN_ID_LIMIT);
    outputFrameLimits.data.bytes[0] = constrain(config->hvUndervoltageLimit - 170, 0, 255);
    outputFrameLimits.data.bytes[1] = constrain(config->lvBuckModeCurrentLimit, 0, 250);
    outputFrameLimits.data.bytes[2] = constrain(config->hvBuckModeCurrentLimit, 0, 250);
    outputFrameLimits.data.bytes[3] = constrain(config->lvUndervoltageLimit, 0, 160);
    outputFrameLimits.data.bytes[4] = constrain(config->lvBoostModeCurrentLinit, 0, 250);
    outputFrameLimits.data.bytes[5] = constrain(config->hvBoostModeCurrentLimit, 0, 250);
    outputFrameLimits.length = 6;
}

/*
 * Return the device id of this device
 */
DeviceId BrusaBSC6::getId()
{
    return BRUSA_BSC6;
}

/*
 * Load configuration data from EEPROM.
 *
 * If not available or the checksum is invalid, default values are chosen.
 */
void BrusaBSC6::loadConfiguration()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new BrusaBSC6Configuration();
        setConfiguration(config);
    }

    DcDcConverter::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        prefsHandler->read(EEDC_DEBUG_MODE, &temp);
        config->debugMode = (temp != 0);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->debugMode = false; // no debug messages
        saveConfiguration();
    }
    logger.info(this, "debug: %d", config->debugMode);

    prepareCanMessages();
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaBSC6::saveConfiguration()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    DcDcConverter::saveConfiguration(); // call parent
    prefsHandler->write(EEDC_DEBUG_MODE, (uint8_t) (config->debugMode ? 1 : 0));
    prefsHandler->saveChecksum();

    prepareCanMessages();
}
