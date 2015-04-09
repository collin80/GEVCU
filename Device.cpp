/*
 * Device.cpp
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

#include "Device.h"
#include "DeviceManager.h"

Device::Device()
{
    status = Status::getInstance();
    tickHandler = TickHandler::getInstance();
    systemIO = SystemIO::getInstance();

    deviceReady = false;
    deviceRunning = false;

    deviceConfiguration = NULL;
    prefsHandler = NULL;
    commonName = "Generic Device";
}


//Empty functions to handle these callbacks if the derived classes don't

void Device::setup()
{
}

char* Device::getCommonName()
{
    return commonName;
}

void Device::handleTick()
{
}

uint32_t Device::getTickInterval()
{
    return 0;
}

//just bubbles up the value from the preference handler.
bool Device::isEnabled()
{
    return prefsHandler->isEnabled();
}

/**
 * Is the device itself ready for operation ?
 */
bool Device::isReady()
{
    return deviceReady;
}

/**
 * Is the device reporting that it's running ?
 */
bool Device::isRunning()
{
    return deviceRunning;
}

/**
 * Handle incoming messages from the DeviceManager. A message might
 * indicate a change in the system state, a command to reload the configuration
 * or other actions.
 */
void Device::handleMessage(uint32_t msgType, void* message)
{
    switch (msgType) {
    case MSG_SOFT_FAULT:
        //TODO: implement action/method for soft fault
        break;
    case MSG_HARD_FAULT:
        //TODO: implement action/method for hard fault
        break;
    case MSG_DISABLE:
        //TODO: implement action/method to disable device (if possible without restart)
        break;
    case MSG_ENABLE:
        //TODO: implement action/method to enable device (if possible without restart)
        break;
    case MSG_STATE_CHANGE:
        Status::SystemState state = *(Status::SystemState *) message;
        switch (state) {
        case Status::init:
            this->setup();
            break;
        }
        break;
    }
}

DeviceType Device::getType()
{
    return DEVICE_NONE;
}

DeviceId Device::getId()
{
    return INVALID;
}

void Device::loadConfiguration()
{
}

void Device::saveConfiguration()
{
}

DeviceConfiguration *Device::getConfiguration()
{
    return this->deviceConfiguration;
}

void Device::setConfiguration(DeviceConfiguration *configuration)
{
    this->deviceConfiguration = configuration;
}

