/*
 * CanOBD2.cpp
 *
 * Listens for PID requests over canbus and responds with the relevant information in the proper format
 *
 * Currently this is in a very rough state and shouldn't be trusted - Make configuration work soon
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

#include "CanOBD2.h"

CanOBD2::CanOBD2() :
        Device()
{
    prefsHandler = new PrefHandler(CANOBD2);
    commonName = "CAN OBD2 Interface";

    canHandlerRespond = NULL;
    canHandlerPoll = NULL;

    arrayPos = 0;
    lastRequestAnswered = true;
    timestamp = 0;
}

void CanOBD2::setup()
{
    Device::setup();

    if (canHandlerRespond != NULL) {
        canHandlerRespond->attach(this, CAN_MASKED_ID_REQUEST, CAN_MASK_REQUEST, false);
    }
    // if we use the same bus for both, no need to register again as below IDs are covered by those above
    if (canHandlerPoll != NULL && canHandlerPoll != canHandlerRespond) {
        canHandlerPoll->attach(this, CAN_MASKED_ID_POLL_RESPONSE, CAN_MASK_POLL_RESPONSE, false);
    }

    ready = true;
    running = true;

    if (canHandlerPoll != NULL) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_CAN_OBD2);
    }
}

void CanOBD2::tearDown()
{
    Device::tearDown();

    if (canHandlerRespond != NULL) {
        canHandlerRespond->detach(this, CAN_MASKED_ID_REQUEST, CAN_MASK_REQUEST);
    }
    if (canHandlerPoll != NULL && canHandlerPoll != canHandlerRespond) {
        canHandlerPoll->detach(this, CAN_MASKED_ID_POLL_RESPONSE, CAN_MASK_POLL_RESPONSE);
    }
}

/*
 * Send a request to the ECU.
 *
 */
uint8_t pids[] = { PID_VEHICLE_SPEED, PID_AMBIENT_TEMP, PID_BAROMETRIC_PRESSURE };

void CanOBD2::handleTick()
{
    Device::handleTick(); // Call parent handleTick
    if (lastRequestAnswered && canHandlerPoll != NULL) {
        CanOBD2Configuration *config = (CanOBD2Configuration *) getConfiguration();

        canHandlerPoll->prepareOutputFrame(&pollFrame, config->canIdOffsetPoll == 255 ? CAN_ID_BROADCAST : CAN_ID_REQUEST + config->canIdOffsetPoll);
        pollFrame.data.bytes[0] = 2; // 2 following bytes
        pollFrame.data.bytes[1] = 1; // show current data
        pollFrame.data.bytes[2] = pids[arrayPos++];
        canHandlerPoll->sendFrame(pollFrame);
        if ((arrayPos + 1) > (sizeof(pids) / sizeof(uint8_t))) {
            arrayPos = 0;
        }
        lastRequestAnswered = false;
        timestamp = millis();
    }
}

/**
 * /brief Process the incoming request for data and send the response back.
 *
 * @param frame can message with request details
 */
void CanOBD2::processRequest(CAN_FRAME *frame)
{
    CanOBD2Configuration *config = (CanOBD2Configuration *) getConfiguration();
    CAN_FRAME outputFrame;

    if (canHandlerRespond != NULL) {
        canHandlerRespond->prepareOutputFrame(&outputFrame, CAN_ID_RESPONSE + config->canIdOffsetRespond);
        if (OBD2Handler::getInstance()->processRequest(frame->data.bytes, outputFrame.data.bytes)) {
            canHandlerRespond->sendFrame(outputFrame);
        }
    }
}

/**
 * /brief Process the incoming response to our own poll request
 *
 * @param frame can message to process
 */
void CanOBD2::processResponse(CAN_FRAME *frame)
{
    if (canHandlerPoll != NULL) {
        uint8_t* b = frame->data.bytes;
        uint8_t pid = b[2];
//Logger::console("pid response: len=%d, mode=%X, pid=%X, data=%d %d %d %d, name=%s", b[0], b[1], b[2], b[3], b[4], b[5], b[6], getPidName(pid));
        switch(pid) {
        case PID_VEHICLE_SPEED:
            status.vehicleSpeed = b[3];
    Logger::console("v speed: %d kmh (%dms)", status.vehicleSpeed, millis() - timestamp);
            break;
        case PID_AMBIENT_TEMP:
            status.temperatureExterior = (b[3] - 40) * 10;
    Logger::console("ext temp: %.1f C (%dms)", status.temperatureExterior / 10.0f, millis() - timestamp);
            break;
        case PID_BAROMETRIC_PRESSURE:
            status.barometricPressure = b[3];
   Logger::console("baro: %d kPa (%dms)", status.barometricPressure, millis() - timestamp);
            break;
        }
        lastRequestAnswered = true;
    }
}

void CanOBD2::handleCanFrame(CAN_FRAME *frame)
{
    CanOBD2Configuration *config = (CanOBD2Configuration *) getConfiguration();

    // a request from CAN bus for OBD2 data (e.g. from a diagnostic tool)
    if ((frame->id == CAN_ID_REQUEST + config->canIdOffsetRespond) || (frame->id == CAN_ID_BROADCAST)) {
        processRequest(frame);
    }
    // a response to our poll for OBD2 data (e.g. containing the vehicle speed)
    if (frame->id >= CAN_ID_RESPONSE && frame->id < CAN_ID_RESPONSE + 8) {
        processResponse(frame);
    }
}

DeviceId CanOBD2::getId()
{
    return CANOBD2;
}

void CanOBD2::loadConfiguration()
{
    CanOBD2Configuration *config = (CanOBD2Configuration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new CanOBD2Configuration();
        setConfiguration(config);
    }

    Device::loadConfiguration(); // call parent
    Logger::info(this, "CanOBD2 configuration:");

#ifdef USE_HARD_CODED
    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        prefsHandler->read(EEOBD2_CAN_BUS_RESPOND, &config->canBusRespond);
        prefsHandler->read(EEOBD2_CAN_ID_OFFSET_RESPOND, &config->canIdOffsetRespond);
        prefsHandler->read(EEOBD2_CAN_BUS_POLL, &config->canBusPoll);
        prefsHandler->read(EEOBD2_CAN_ID_OFFSET_POLL, &config->canIdOffsetPoll);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->canBusRespond = 0; // ev can bus
        config->canIdOffsetRespond = 0;
        config->canBusPoll = 1; // car's can bus
        config->canIdOffsetPoll = 255; // broadcast
        saveConfiguration();
    }

    if (config->canBusPoll != CFG_OUTPUT_NONE) {
        canHandlerPoll = (config->canBusPoll == 1 ? &canHandlerCar : &canHandlerEv);
    }
    if (config->canBusRespond != CFG_OUTPUT_NONE) {
        canHandlerRespond = (config->canBusRespond == 1 ? &canHandlerCar : &canHandlerEv);
    }

    Logger::info(this, "bus respond: %d, respond id offset: %d, bus poll: %d, poll id offset: %d", config->canBusRespond, config->canIdOffsetRespond,
            config->canBusPoll, config->canIdOffsetPoll);
}

/*
 * Store the current configuration to EEPROM
 */
void CanOBD2::saveConfiguration()
{
    CanOBD2Configuration *config = (CanOBD2Configuration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(EEOBD2_CAN_BUS_RESPOND, config->canBusRespond);
    prefsHandler->write(EEOBD2_CAN_ID_OFFSET_RESPOND, config->canIdOffsetRespond);
    prefsHandler->write(EEOBD2_CAN_BUS_POLL, config->canBusPoll);
    prefsHandler->write(EEOBD2_CAN_ID_OFFSET_POLL, config->canIdOffsetPoll);
    prefsHandler->saveChecksum();
}

