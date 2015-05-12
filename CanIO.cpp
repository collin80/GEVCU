/*
 * CanIO.c
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

#include "CanIO.h"

CanIO::CanIO() : Device()
{
    prefsHandler = new PrefHandler(CANIO);
    canHandlerEv = CanHandler::getInstanceEV();
    commonName = "CanIO";
}

void CanIO::setup()
{
    tickHandler->detach(this);

    loadConfiguration();

    canHandlerEv->attach(this, CAN_MASKED_ID, CAN_MASK, false);

    deviceReady = true;
    deviceRunning = true;

    tickHandler->attach(this, CFG_TICK_INTERVAL_CAN_IO);
}

void CanIO::handleTick()
{
    sendIOStatus();
}

/*
 * Processes an event from the CanHandler.
 */
void CanIO::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
    case CAN_ID_GEVCU_EXT_TEMPERATURE:
        processExternalTemperature(frame->data.byte);
        break;
    }
}

/**
 * react on additional messages from device manager
 * in case of quick output changes in SystemIO, it is vital to send additional
 * IO status messages
 *
 */
void CanIO::handleMessage(uint32_t msgType, void* message)
{
    Device::handleMessage(msgType, message);

    switch (msgType) {
    case MSG_UPDATE:
        sendIOStatus();
        break;
    }
}


/*
 * Send the status of the IO over CAN so it can be used by other devices.
 */
void CanIO::sendIOStatus()
{
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
    logicIO |= status->heatingPump ? heatingPump : 0;
    logicIO |= status->batteryHeater ? batteryHeater : 0;
    logicIO |= status->chargePowerAvailable ? chargePowerAvailable : 0;
    logicIO |= status->activateCharger ? activateCharger : 0;
    logicIO |= status->reverseLight ? reverseLight : 0;
    logicIO |= status->brakeLight ? brakeLight : 0;
    logicIO |= status->coolingPump ? coolingPump : 0;
    logicIO |= status->coolingFan ? coolingFan : 0;
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

void CanIO::processExternalTemperature(byte bytes[])
{
    for (int i = 0; i < CFG_NUMBER_TEMPERATURE_SENSORS; i++) {
        if (bytes[i] != 0) {
            status->externalTemperature[i] = bytes[i] - 50;
Logger::info(CANIO, "external temperature %d: %d", i, status->externalTemperature[i]);
        }
    }
}

DeviceType CanIO::getType()
{
    return DEVICE_IO;
}

DeviceId CanIO::getId()
{
    return CANIO;
}

void CanIO::loadConfiguration()
{
#ifdef USE_HARD_CODED
    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
//        prefsHandler->read(EESYS_ENABLE_INPUT, &configuration->enableInput);
//        prefsHandler->read(EESYS_PRECHARGE_MILLIS, &configuration->prechargeMillis);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
//        configuration->enableInput = EnableInput;
//        configuration->prechargeMillis = PrechargeMillis;
    }
}

void CanIO::saveConfiguration()
{
//    prefsHandler->write(EESYS_ENABLE_INPUT, configuration->enableInput);
//    prefsHandler->write(EESYS_PRECHARGE_MILLIS, configuration->prechargeMillis);
//    prefsHandler->saveChecksum();
}
