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
    canHandlerEv.detach(this, CAN_MASKED_ID, CAN_MASK);
    sendCommand(); // as powerOn is false now, send last command to deactivate controller
}

void BrusaBSC6::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    bool powerOnBefore = powerOn;

    DcDcConverter::handleStateChange(oldState, newState);

    if (powerOnBefore != powerOn) {
        if (powerOn) {
            // register ourselves as observer of 0x26a-0x26f can frames
            canHandlerEv.attach(this, CAN_MASKED_ID, CAN_MASK, false);
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

    sendCommand();
    sendLimits();
}

/*
 * Send BSC6COM message to the DCDC converter.
 *
 * The message is used to set the operation mode, enable the converter
 * and set the voltage limits.
 */
void BrusaBSC6::sendCommand()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();
    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_COMMAND);

    if ((ready || running) && powerOn) {
        outputFrame.data.bytes[0] |= enable;
    }
    if (config->mode == 1) {
        outputFrame.data.bytes[0] |= boostMode;
    }
    if (config->debugMode) {
        outputFrame.data.bytes[0] |= debugMode;
    }

    outputFrame.data.bytes[1] = constrain(config->lowVoltageCommand, 80, 160); // 8-16V in 0.1V, offset = 0V
    outputFrame.data.bytes[2] = constrain(config->highVoltageCommand - 170, 20, 255); // 190-425V in 1V, offset = 170V

    outputFrame.length = 3;
    canHandlerEv.sendFrame(outputFrame);
}

/*
 * Send BSC6LIM message to DCDC converter.
 *
 * This message controls the electrical limits in the converter.
 */
void BrusaBSC6::sendLimits()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();
    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_LIMIT);

    outputFrame.data.bytes[0] = constrain(config->hvUndervoltageLimit - 170, 0, 255);
    outputFrame.data.bytes[1] = constrain(config->lvBuckModeCurrentLimit, 0, 250);
    outputFrame.data.bytes[2] = constrain(config->hvBuckModeCurrentLimit, 0, 250);
    outputFrame.data.bytes[3] = constrain(config->lvUndervoltageLimit, 0, 160);
    outputFrame.data.bytes[4] = constrain(config->lvBoostModeCurrentLinit, 0, 250);
    outputFrame.data.bytes[5] = constrain(config->hvBoostModeCurrentLimit, 0, 250);
    outputFrame.length = 6;

    canHandlerEv.sendFrame(outputFrame);
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
        case CAN_ID_VALUES_1:
            processValues1(frame->data.bytes);
            break;

        case CAN_ID_VALUES_2:
            processValues2(frame->data.bytes);
            break;

        case CAN_ID_DEBUG_1:
            processDebug1(frame->data.bytes);
            break;

        case CAN_ID_DEBUG_2:
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
    ready = (bitfield & bsc6Ready) ? true : false;
    if (bitfield & automatic) {
        //TODO
    }

    hvVoltage = (uint16_t)(data[1] | (data[0] << 8));
    lvVoltage = (uint8_t)data[2];
    hvCurrent = (int16_t)(data[4] | (data[3] << 8)) - 250;
    lvCurrent = (int16_t)(data[6] | (data[5] << 8)) - 280;
    mode = (data[7] & 0xF0) >> 4;

    if (Logger::isDebug()) {
        Logger::debug(this, "status bitfield: %#08x, ready: %d, running: %d, HV: %fV %fA, LV: %fV %dA, mode %d", bitfield, ready, running, (float) hvVoltage / 10.0F, (float) hvCurrent / 10.0F, (float) lvVoltage / 10.0F, lvCurrent, mode);
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
    // TODO: react on various bitfields if set ?
//    lowVoltageUndervoltage
//    lowVoltageOvervoltage
//    highVoltageUndervoltage
//    highVoltageOvervoltage
//    internalSupply
//    temperatureSensor
//    trafoStartup
//    overTemperature
//    highVoltageFuse
//    lowVoltageFuse
//    currentSensorLowSide
//    currentDeviation
//    interLock
//    internalSupply12V
//    internalSupply6V
//    voltageDeviation
//    invalidValue
//    commandMessageLost
//    limitMessageLost
//    crcErrorNVSRAM
//    brokenTemperatureSensor

    if (bitfield != 0) {
        Logger::error(this, "%#08x", bitfield);
    }
    if (Logger::isDebug()) {
        Logger::debug(this, "LV current avail: %dA, maximum Temperature: %.1fC", lvCurrentAvailable, (float) temperature / 10.0F);
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

    if (Logger::isDebug()) {
        Logger::debug(this, "Temp buck/boost switch 1: %d�C, switch 2: %d�C", temperatureBuckBoostSwitch1, temperatureBuckBoostSwitch2);
        Logger::debug(this, "Temp HV trafostage switch 1: %d�C, switch 2: %d�C", temperatureHvTrafostageSwitch1, temperatureHvTrafostageSwitch2);
        Logger::debug(this, "Temp LV trafostage switch 1: %d�C, switch 2: %d�C", temperatureLvTrafostageSwitch1, temperatureLvTrafostageSwitch2);
        Logger::debug(this, "Temp transformer coil 1: %d�C, coil 2: %d�C", temperatureTransformerCoil1, temperatureTransformerCoil2);
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

    if (Logger::isDebug()) {
        Logger::debug(this, "internal 12V supply: %.1fV, LS actual voltage: %.1fV", (float) internal12VSupplyVoltage / 10.0F, (float) lsActualVoltage / 10.0F);
        Logger::debug(this, "LS actual current: %.2fA, LS commanded current: %.2fA", (float) internal12VSupplyVoltage / 200.0F, (float) lsActualVoltage / 200.0F);
        Logger::debug(this, "internal power state: %d", internalOperationState);
    }
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
        prefsHandler->read(DCDC_DEBUG_MODE, &temp);
        config->debugMode = (temp != 0);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->debugMode = false; // no debug messages
        saveConfiguration();
    }
    Logger::info(this, "debug: %d", config->debugMode);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaBSC6::saveConfiguration()
{
    BrusaBSC6Configuration *config = (BrusaBSC6Configuration *) getConfiguration();

    DcDcConverter::saveConfiguration(); // call parent

    prefsHandler->write(DCDC_DEBUG_MODE, (uint8_t) (config->debugMode ? 1 : 0));

    prefsHandler->saveChecksum();
}
