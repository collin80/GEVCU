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
    lightLevel = 0;
    mode = none;
    up = false;
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
    switch (mode) {
    case on:
        lightLevel += 5;
        if (lightLevel >= 250) {
            lightLevel = 255;
            mode = none;
        }
        break;
    case off:
        lightLevel -= 5;
        if (lightLevel <= 5) {
            lightLevel = 0;
            mode = none;
        }
        break;
    case blink:
        lightLevel += (up ? 5 : -5);
        if (lightLevel >= 250) {
            lightLevel = 255;
            up = false;
        }
        if (lightLevel <= 40) {
            lightLevel = 40;
            up = true;
        }
        break;
    case pulse:
        lightLevel += (up ? 2 : -2);
        if (lightLevel >= 125) {
            lightLevel = 125;
            up = false;
        }
        if (lightLevel <= 0) {
            lightLevel = 0;
            up = true;
        }
        break;
    default:
        tickHandler.detach(this);
    }
    systemIO.setStatusLight(lightLevel);
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

    if (mode == none) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_STATUS);
    }

    switch (newState) {
    case Status::init:
    case Status::charged:
        mode = pulse;
        break;
    case Status::charging:
        mode = blink;
        break;
    case Status::running:
        mode = on;
        break;
    }
    up = (lightLevel == 0);
}

void StatusIndicator::handleMessage(uint32_t messageType, void* message)
{
    Device::handleMessage(messageType, message);

    if (mode == none) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_STATUS);
    }

    switch (messageType) {
    case MSG_UPDATE: {
        char *param = (char *) message;

        if(!strcmp("on", param)) {
            mode = on;
        } else if(!strcmp("off", param)) {
            mode = off;
        } else if(!strcmp("blink", param)) {
            mode = blink;
        } else if(!strcmp("pulse", param)) {
            mode = pulse;
        }

        up = (lightLevel == 0);
        break;
    }
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
