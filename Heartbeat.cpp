/*
 * Heartbeat.c
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

#include "Heartbeat.h"

Heartbeat::Heartbeat() : Device()
{
    prefsHandler = new PrefHandler(HEARTBEAT);
    led = false;
    throttleDebug = false;
    dotCount = 0;
    lastTickTime = 0;
    commonName = "Heartbeat";
}

void Heartbeat::setup()
{
    Device::setup();

    pinMode(CFG_IO_BLINK_LED, OUTPUT);
    digitalWrite(CFG_IO_BLINK_LED, LOW);
    ready = true;
    running = true;

    tickHandler.attach(this, CFG_TICK_INTERVAL_HEARTBEAT);
}

void Heartbeat::setThrottleDebug(bool debug)
{
    throttleDebug = debug;
}

bool Heartbeat::getThrottleDebug()
{
    return throttleDebug;
}

void Heartbeat::handleTick()
{
    SerialUSB.print('.');

    if ((++dotCount % 80) == 0) {
        SerialUSB.println();
    }

    lastTickTime = millis();

    if (led) {
        digitalWrite(CFG_IO_BLINK_LED, HIGH);
    } else {
        digitalWrite(CFG_IO_BLINK_LED, LOW);
    }

    led = !led;

    if (throttleDebug) {
        Throttle *accelerator = deviceManager.getAccelerator();
        Throttle *brake = deviceManager.getBrake();

        logger.console("");

        logger.console("System State: %s", status.systemStateToStr(status.getSystemState()).c_str());
        systemIO.printIOStatus();

        if (accelerator) {
            logger.console("Throttle Status: isFaulted: %d level: %i", accelerator->isFaulted(), accelerator->getLevel());
            RawSignalData *rawSignal = accelerator->acquireRawSignal();
            logger.console("Throttle rawSignal1: %d, rawSignal2: %d", rawSignal->input1, rawSignal->input2);
        }

        if (brake) {
            logger.console("Brake Output: %i", brake->getLevel());
            RawSignalData *rawSignal = brake->acquireRawSignal();
            logger.console("Brake rawSignal1: %d", rawSignal->input1);
        }
    }
}

DeviceType Heartbeat::getType()
{
    return DEVICE_MISC;
}

DeviceId Heartbeat::getId()
{
    return HEARTBEAT;
}

void Heartbeat::loadConfiguration()
{
//    HeartbeatConfiguration *config = (HeartbeatConfiguration *) getConfiguration();

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
//      prefsHandler->read(EESYS_, &config->);
    } else {
        saveConfiguration();
    }
}

void Heartbeat::saveConfiguration()
{
//    HeartbeatConfiguration *config = (HeartbeatConfiguration *) getConfiguration();

//  prefsHandler->write(EESYS_, config->);
    prefsHandler->saveChecksum();
}
