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

/**
 * Constructor - initialize class variables
 */
Device::Device()
{
    prefsHandler = NULL;

    commonName = "Generic Device";
    deviceConfiguration = NULL;

    ready = false;
    running = false;
    powerOn = false;
}

/**
 * Destructor
 */
Device::~Device()
{
}

/**
 * Called during initialization of the device.
 * May be called multiple times e.g. when recovering from an error
 * or disabling and re-enabling the device.
 */
void Device::setup()
{
    tickHandler.detach(this);

    ready = false;
    running = false;
    powerOn = false;

    loadConfiguration();

    logger.info(this, "device started");
}

/**
 * Called during tear-down of the device.
 * May be called multiple times e.g. when disabling the device.
 */
void Device::tearDown()
{
    tickHandler.detach(this);
    ready = false;
    running = false;
    powerOn = false;

    logger.info(this, "device stopped");
}

/**
 * Retrieve the common name of the device.
 */
String Device::getCommonName()
{
    return commonName;
}

/**
 * Handle a timer event - called by the TickHandler
 */
void Device::handleTick()
{
}

/**
 * Enable the device in the preferences and activate it
 */
void Device::enable()
{
    if (isEnabled()) {
        return;
    }
    if (prefsHandler != NULL && prefsHandler->setEnabled(true)) {
        prefsHandler->suggestCacheWrite(); //just in case someone power cycles quickly
        logger.info(this, "Successfully enabled device %s.(%#x)", commonName.c_str(), getId());
    }
    setup();
}

/**
 * Deactivate the device and disable it in the preferences
 */
void Device::disable()
{
    if (!isEnabled()) {
        return;
    }
    if (prefsHandler != NULL && prefsHandler->setEnabled(false)) {
        prefsHandler->suggestCacheWrite(); //just in case someone power cycles quickly
        logger.info(this, "Successfully disabled device %s.(%#x)", commonName.c_str(), getId());
    }
    tearDown();
}

/**
 * Returns if the device is enabled via preferences
 */
bool Device::isEnabled()
{
    if (prefsHandler == NULL) {
        return true;
    }
    return prefsHandler->isEnabled();
}

/**
 * Is the device itself ready for operation ?
 */
bool Device::isReady()
{
    return ready;
}

/**
 * Is the device reporting that it's running ?
 */
bool Device::isRunning()
{
    return running;
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
        disable();
        break;
    case MSG_ENABLE:
        enable();
        break;
    case MSG_KILL:
        tearDown();
        break;
    case MSG_STATE_CHANGE:
        Status::SystemState *state = (Status::SystemState *) message;
        handleStateChange(state[0], state[1]);
        break;
    }
}

/**
 * React on state changes.
 * Subclasses may overwrite the method but should call the parent.
 */
void Device::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    switch (newState) {
    case Status::init:
        this->setup();
        break;
    case Status::error: // stop all devices in case of an error
        this->tearDown();
        break;
    case Status::shutdown: // stop all devices
        this->tearDown();
        break;
    default:
        break;
    }
}

/**
 * If a flag is set in a bitfield, add a message part to a message
 */
void Device::appendMessage(String &message, uint32_t bitfield, uint32_t flag, String part) {
    if (bitfield & flag) {
        if (message.length() > 0) {
            message.concat(", ");
        }
        message.concat(part);
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

