/*
 * CanThrottle.cpp
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

#include "CanThrottle.h"

CanThrottle::CanThrottle() : Throttle()
{
    prefsHandler = new PrefHandler(CANACCELPEDAL);
    rawSignal.input1 = 0;
    rawSignal.input2 = 0;
    rawSignal.input3 = 0;
    ticksNoResponse = 255; // invalidate input signal until response is received
    responseId = 0;
    responseMask = 0x7ff;
    responseExtended = false;

    commonName = "CANBus accelerator";
}

void CanThrottle::setup()
{
    Throttle::setup();

    requestFrame.length = 0x08;
    requestFrame.rtr = 0x00;
    requestFrame.extended = 0x00;

    SystemIOConfiguration *config = (SystemIOConfiguration *) getConfiguration();

    switch (config->carType) {
    case SystemIOConfiguration::OBD2:
        requestFrame.id = 0x7df; // OBD2 broadcast (or specific address from 0x7e0 to 0x7e7)
        memcpy(requestFrame.data.bytes, (const uint8_t[] ) { 0x02, 0x01, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00 }, 8); // 2=data bytes, 1=current data, 4c=throttle commanded
        responseId = 0x7e8; // usually ECU responds on 0x7e8 (possible range: 0x7e8 to 0x7ef)
        ready = true;
        break;
    case SystemIOConfiguration::Volvo_S80_Gas:
        // Request: dlc=0x08 fid=0x7e0 id=0x7e0 ide=0x00 rtr=0x00 data=0x03,0x22,0xEE,0xCB,0x00,0x00,0x00,0x00 (vida: [0x00, 0x00, 0x07, 0xe0, 0x22, 0xee, 0xcb])
        // Raw response: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00 (vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14])
        requestFrame.id = 0x7e0;
        memcpy(requestFrame.data.bytes, (const uint8_t[] ) { 0x03, 0x22, 0xee, 0xcb, 0x00, 0x00, 0x00, 0x00 }, 8);
        responseId = 0x7e8;
        ready = true;
        break;

    case SystemIOConfiguration::Volvo_V50_Diesel:
        // Request: dlc=0x08 fid=0xFFFFE id=0x3FFFE ide=0x01 rtr=0x00 data=0xCD,0x11,0xA6,0x00,0x24,0x01,0x00,0x00 (vida: [0x00, 0xf, 0xff, 0xfe, 0xcd, 0x11, 0xa6, 0x00, 0x24, 0x01, 0x00, 0x00])
        // Response: dlc=0x08 fid=0x400021 id=0x21 ide=0x01 rtr=0x00 data=0xCE,0x11,0xE6,0x00,0x24,0x03,0xFD,0x00 (vida: [0x00, 0x40, 0x00, 0x21, 0xce, 0x11, 0xe6, 0x00, 0x24, 0x03, 0xfd, 0x00])
        requestFrame.id = 0x3FFFE;
        requestFrame.extended = 0x01;
        memcpy(requestFrame.data.bytes, (const uint8_t[] ) { 0xce, 0x11, 0xe6, 0x00, 0x24, 0x03, 0xfd, 0x00 }, 8);
        responseId = 0x21;
        responseExtended = true;
        ready = true;
        break;

    default:
        logger.error(this, "no valid car type defined.");
    }
}

/**
 * Tear down the device in a safe way.
 */
void CanThrottle::tearDown()
{
    Throttle::tearDown();

    canHandlerCar.detach(this, responseId, responseMask);
}

/**
 * act on messages the super-class does not react upon, like state change
 * to ready or running which should enable/disable the controller
 */
void CanThrottle::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Throttle::handleStateChange(oldState, newState);

    if (newState == Status::ready || newState == Status::running) {
        if (oldState != Status::ready && oldState != Status::running) {
            canHandlerCar.attach(this, responseId, responseMask, responseExtended);
            tickHandler.attach(this, CFG_TICK_INTERVAL_CAN_THROTTLE);
        }
    } else {
        if (oldState == Status::ready || oldState == Status::running) {
            tearDown();
        }
    }
}

/*
 * Send a request to the ECU.
 *
 */
void CanThrottle::handleTick()
{
    Throttle::handleTick(); // Call parent handleTick

    canHandlerCar.sendFrame(requestFrame);

    if (ticksNoResponse < 255) { // make sure it doesn't overflow
        ticksNoResponse++;
    }
}

/*
 * Handle the response of the ECU and calculate the throttle value
 *
 */
void CanThrottle::handleCanFrame(CAN_FRAME *frame)
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) getConfiguration();

    if (frame->id == responseId) {
        switch (config->carType) {
        case SystemIOConfiguration::OBD2:
            if (frame->data.bytes[0] == 0x3 && frame->data.bytes[1] == 0x41 && frame->data.bytes[2] == 0x4c) { // [0]=num data bytes, [1]=mode + 0x40, [2]=PID
                rawSignal.input1 = frame->data.bytes[3];
                ticksNoResponse = 0;
            }
            break;
        case SystemIOConfiguration::Volvo_S80_Gas:
            // only evaluate messages with payload 0x04,0x62,0xEE,0xCB as other ECU data is also sent by with 0x738
            if (frame->data.bytes[0] == 0x04 && frame->data.bytes[1] == 0x62 && frame->data.bytes[2] == 0xee && frame->data.bytes[3] == 0xcb) {
                rawSignal.input1 = frame->data.bytes[4];
                ticksNoResponse = 0;
            }
            break;

        case SystemIOConfiguration::Volvo_V50_Diesel:
            // only evaluate messages with payload 0xCE,0x11,0xE6,0x00,0x2
            if (frame->data.bytes[0] == 0xce && frame->data.bytes[1] == 0x11 && frame->data.bytes[2] == 0x6E && frame->data.bytes[3] == 0x00
                    && frame->data.bytes[4] == 0x02) {
                rawSignal.input1 = (frame->data.bytes[5] + 1) * frame->data.bytes[6];
                ticksNoResponse = 0;
            }
            break;
        }
    }
}

RawSignalData* CanThrottle::acquireRawSignal()
{
    return &rawSignal; // should have already happened in the background
}

bool CanThrottle::validateSignal(RawSignalData* rawSignal)
{
    CanThrottleConfiguration *config = (CanThrottleConfiguration *) getConfiguration();

    if (ticksNoResponse >= CFG_CANTHROTTLE_MAX_NUM_LOST_MSG) {
        if (throttleStatus == OK) {
            logger.error(this, "no response on position request received: %d ", ticksNoResponse);
        }

        throttleStatus = ERR_MISC;
        return false;
    }

    if (rawSignal->input1 > (config->maximumLevel + CFG_THROTTLE_TOLERANCE)) {
        if (throttleStatus == OK) {
            logger.error(this, VALUE_OUT_OF_RANGE, rawSignal->input1);
        }

        throttleStatus = ERR_HIGH_T1;
        return false;
    }

    if (rawSignal->input1 < (config->minimumLevel - CFG_THROTTLE_TOLERANCE)) {
        if (throttleStatus == OK) {
            logger.error(this, VALUE_OUT_OF_RANGE, rawSignal->input1);
        }

        throttleStatus = ERR_LOW_T1;
        return false;
    }

    // all checks passed -> throttle seems to be ok
    if (throttleStatus != OK) {
        logger.info(this, NORMAL_OPERATION);
    }

    throttleStatus = OK;
    return true;
}

uint16_t CanThrottle::calculatePedalPosition(RawSignalData* rawSignal)
{
    CanThrottleConfiguration *config = (CanThrottleConfiguration *) getConfiguration();

    return normalizeAndConstrainInput(rawSignal->input1, config->minimumLevel, config->maximumLevel);
}

DeviceId CanThrottle::getId()
{
    return CANACCELPEDAL;
}

void CanThrottle::loadConfiguration()
{
    CanThrottleConfiguration *config = (CanThrottleConfiguration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new CanThrottleConfiguration();
        setConfiguration(config);
    }

    Throttle::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
    } else {
        saveConfiguration();
    }

    logger.info(this, "MIN: %ld MAX: %ld", config->minimumLevel, config->maximumLevel);
}

/*
 * Store the current configuration to EEPROM
 */
void CanThrottle::saveConfiguration()
{
    CanThrottleConfiguration *config = (CanThrottleConfiguration *) getConfiguration();

    Throttle::saveConfiguration(); // call parent

    prefsHandler->saveChecksum();
}
