/*
 * StatusIndicator.cpp
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

#include "StatusIndicator.h"

StatusIndicator::StatusIndicator() :
        Device()
{
    prefsHandler = new PrefHandler(STATUSINDICATOR);
    commonName = "Status Display";
    step = 0;
    increment = false;
    cfg.cyclesDown = cfg.cyclesUp = 1;
}

void StatusIndicator::setup()
{
    Device::setup();

    systemIO.setStatusLight(0);
    ready = true;
    running = true;
}

void StatusIndicator::handleTick()
{
    step += (increment ? cfg.increment : -cfg.decrement);
    if (step > 250) {
        increment = false;
        step = 250;
        if (cfg.cyclesUp > 0) {
            cfg.cyclesUp--;
        }
    }
    if (step < 1) {
        increment = true;
        step = 0;
        if (cfg.cyclesDown > 0) {
            cfg.cyclesDown--;
        }
    }
    systemIO.setStatusLight(1 / (1 + exp(((step / 20.0f) - 8) * -0.75f)) * cfg.maxLevel + cfg.minLevel);

    if ((increment && cfg.cyclesUp == 0) || (!increment && cfg.cyclesDown == 0)) {
        tickHandler.detach(this);
        systemIO.setStatusLight((increment ? cfg.minLevel : cfg.maxLevel));
    }
}

/**
 * \brief Act on system state changes
 *
 * \param oldState the previous system state
 * \param newState the new system state
 */
void StatusIndicator::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);

    switch (newState) {
    case Status::init:
    case Status::charged:
        handleMessage(MSG_UPDATE, (void *) "pulse");
        break;
    case Status::charging:
        handleMessage(MSG_UPDATE, (void *) "blink");
        break;
    case Status::running:
        handleMessage(MSG_UPDATE, (void *) "on");
        break;
    }
}

void StatusIndicator::handleMessage(uint32_t messageType, void* message)
{
    Device::handleMessage(messageType, message);

    switch (messageType) {
    case MSG_UPDATE: {
        char *param = (char *) message;

        if (!strcmp("on", param)) {
            setMode(220, 0, 5, 5, 1, 0);
            increment = true;
        } else if (!strcmp("off", param)) {
            setMode(220, 0, 3, 3, 0, 1);
            increment = false;
        } else if (!strcmp("blink", param)) {
            setMode(220, 6, 4, 3, -1, -1);
        } else if (!strcmp("pulse", param)) {
            setMode(45, 4, 1, 1, -1, -1);
        }
        break;
    }
    }
}

void StatusIndicator::setMode(uint8_t maxLevel, uint8_t minLevel, uint8_t increment, uint8_t decrement, int16_t cyclesUp, int16_t cyclesDown)
{
    cfg.maxLevel = maxLevel;
    cfg.minLevel = minLevel;
    cfg.increment = increment;
    cfg.decrement = decrement;
    cfg.cyclesUp = cyclesUp;
    cfg.cyclesDown = cyclesDown;

    if (!tickHandler.isAttached(this, CFG_TICK_INTERVAL_STATUS)) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_STATUS);
    }
}

DeviceType StatusIndicator::getType()
{
    return DEVICE_DISPLAY;
}

DeviceId StatusIndicator::getId()
{
    return STATUSINDICATOR;
}

void StatusIndicator::loadConfiguration()
{
//    StatusDisplayConfiguration *config = (StatusDisplayConfiguration *) getConfiguration();

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
//      prefsHandler->read(EESYS_, &config->);
    } else {
        saveConfiguration();
    }
}

void StatusIndicator::saveConfiguration()
{
//    StatusDisplayConfiguration *config = (StatusDisplayConfiguration *) getConfiguration();

//  prefsHandler->write(EESYS_, config->);
    prefsHandler->saveChecksum();
}
