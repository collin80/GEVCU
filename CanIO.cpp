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

CanIO::CanIO() :
        Device()
{
    prefsHandler = new PrefHandler(CANIO);
    commonName = "CAN I/O";
    motorController = NULL;
    dcdcConverter = NULL;
}

void CanIO::setup()
{
    Device::setup();
    motorController = deviceManager.getMotorController();
    dcdcConverter = deviceManager.getDcDcConverter();

    canHandlerEv.attach(this, IO_CAN_MASKED_ID, IO_CAN_MASK, false);
    canHandlerCar.attach(this, IO_CAN_MASKED_ID_CAR, IO_CAN_MASK_CAR, false);

    ready = true;
    running = true;

    tickHandler.attach(this, CFG_TICK_INTERVAL_CAN_IO);
}

/**
 * Tear down the device in a safe way.
 */
void CanIO::tearDown()
{
    Device::tearDown();
    sendIOStatus(); // so the error state is transmitted

    canHandlerEv.detach(this, IO_CAN_MASKED_ID, IO_CAN_MASK);
}

void CanIO::handleTick()
{
    sendIOStatus();
    sendAnalogData();
    sendMotorData();
}

/*
 * Processes an event from the CanHandler.
 */
void CanIO::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
    case IO_CAN_ID_GEVCU_EXT_TEMPERATURE:
        processTemperature(frame->data.byte);
        break;
    case IO_CAN_ID_GEVCU_EXT_FLOW_COOL:
        status.flowCoolant = frame->data.high;
        break;
    case IO_CAN_ID_GEVCU_EXT_FLOW_HEAT:
        status.flowHeater = frame->data.high;
        break;
    case IO_CAN_ID_GEVCU_EXT_HEATER:
        status.heaterPower = frame->data.bytes[0] << 8 | frame->data.bytes[1];
        status.heaterTemperature = frame->data.bytes[2];
        break;
    case IO_CAN_ID_CRUISE_CONTROL: // buttons on the steering wheel
    	motorController->handleCruiseControlButton(getCruiseControlButton(frame->data.byte));
    	break;
    case IO_CAN_ID_VEHICLE_SPEED:
    	//TODO read vehicle speed
    	break;
    }
}


/**
 * Identify pressed cruise control button
 *
 * NOTE: This is currently specific to a Volvo S80 Y08, implementation would need to be adjusted to support other car types
 *
 *                   not pressed       pressed
 * Cruise enable:    byte5,bit2 (42)   byte7,bit2(58)
 * +:                byte5,bit7 (47)   byte7,bit7(63)
 * -:                byte4,bit0 (32)   byte6,bit0(48)
 * recall:           byte5,bit5 (45)   byte7,bit5(61)
 * 0:                byte5,bit4 (44)   byte7,bit4(60)
 *
 */
MotorController::CruiseControlButton CanIO::getCruiseControlButton(uint8_t data[]) {
	if ((data[7] & 1<<2) && !(data[5] & 1<<2))
		return MotorController::TOGGLE;
	if ((data[7] & 1<<7) && !(data[5] & 1<<7))
		return MotorController::PLUS;
	if ((data[6] & 1<<0) && !(data[4] & 1<<0))
		return MotorController::MINUS;
	if ((data[7] & 1<<5) && !(data[5] & 1<<5))
		return MotorController::RECALL;
	if ((data[7] & 1<<4) && !(data[5] & 1<<4))
		return MotorController::DISENGAGE;
	return MotorController::NONE;
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
    canHandlerEv.prepareOutputFrame(&outputFrame, IO_CAN_ID_GEVCU_STATUS);

    uint16_t rawIO = 0;
    rawIO |= status.digitalInput[0] ? digitalIn1 : 0;
    rawIO |= status.digitalInput[1] ? digitalIn2 : 0;
    rawIO |= status.digitalInput[2] ? digitalIn3 : 0;
    rawIO |= status.digitalInput[3] ? digitalIn4 : 0;
    rawIO |= status.digitalOutput[0] ? digitalOut1 : 0;
    rawIO |= status.digitalOutput[1] ? digitalOut2 : 0;
    rawIO |= status.digitalOutput[2] ? digitalOut3 : 0;
    rawIO |= status.digitalOutput[3] ? digitalOut4 : 0;
    rawIO |= status.digitalOutput[4] ? digitalOut5 : 0;
    rawIO |= status.digitalOutput[5] ? digitalOut6 : 0;
    rawIO |= status.digitalOutput[6] ? digitalOut7 : 0;
    rawIO |= status.digitalOutput[7] ? digitalOut8 : 0;

    outputFrame.data.s0 = rawIO;

    uint16_t logicIO = 0;
    logicIO |= status.preChargeRelay ? preChargeRelay : 0;
    logicIO |= status.mainContactor ? mainContactor : 0;
    logicIO |= status.secondaryContactor ? secondaryContactor : 0;
    logicIO |= status.fastChargeContactor ? fastChargeContactor : 0;

    logicIO |= status.enableMotor ? enableMotor : 0;
    logicIO |= status.enableCharger ? enableCharger : 0;
    logicIO |= status.enableDcDc ? enableDcDc : 0;
    logicIO |= status.enableHeater ? enableHeater : 0;

    logicIO |= status.heaterValve ? heaterValve : 0;
    logicIO |= status.heaterPump ? heaterPump : 0;
    logicIO |= status.coolingPump ? coolingPump : 0;
    logicIO |= status.coolingFan ? coolingFan : 0;

    logicIO |= status.brakeLight ? brakeLight : 0;
    logicIO |= status.reverseLight ? reverseLight : 0;
    logicIO |= status.powerSteering ? powerSteering : 0;
    logicIO |= status.unused ? unused : 0;

    outputFrame.data.s1 = logicIO;

    outputFrame.data.byte[4] = status.getSystemState();

    canHandlerEv.sendFrame(outputFrame);
}

/*
 * Send the values of the analog inputs over CAN so it can be used by other devices.
 */
void CanIO::sendAnalogData()
{
    canHandlerEv.prepareOutputFrame(&outputFrame, IO_CAN_ID_GEVCU_ANALOG_IO);
    outputFrame.data.s0 = systemIO.getAnalogIn(0);
    outputFrame.data.s1 = systemIO.getAnalogIn(1);
    outputFrame.data.s2 = systemIO.getAnalogIn(2);
    outputFrame.data.s3 = systemIO.getAnalogIn(3);
    canHandlerEv.sendFrame(outputFrame);
}

/*
 * Send the values of the motor controller over the car's CAN so it can be used by a CAN filter.
 */
void CanIO::sendMotorData()
{
    canHandlerCar.prepareOutputFrame(&outputFrame, IO_CAN_ID_GEVCU_MOTOR_DATA);
    if (motorController != NULL) {
        outputFrame.data.s0 = max(motorController->getSpeedActual(), 0);
    }
    if (dcdcConverter != NULL) {
        outputFrame.data.s1 = (dcdcConverter->isRunning() ? 0x01 : 0x00);
    }
    canHandlerCar.sendFrame(outputFrame);
}

void CanIO::processTemperature(byte bytes[])
{
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        status.temperatureBattery[i] = (bytes[i] == 0 ? CFG_NO_TEMPERATURE_DATA : (bytes[i] - CFG_CAN_TEMPERATURE_OFFSET) * 10);
        if (logger.isDebug()) {
            logger.debug(this, "battery temperature %d: %.1f", i, status.temperatureBattery[i] / 10.0f);
        }
    }
    status.temperatureCoolant = (bytes[6] == 0 ? CFG_NO_TEMPERATURE_DATA : (bytes[6] - CFG_CAN_TEMPERATURE_OFFSET) * 10);
    status.temperatureExterior = (bytes[7] == 0 ? CFG_NO_TEMPERATURE_DATA : (bytes[7] - CFG_CAN_TEMPERATURE_OFFSET) * 10);
    if (logger.isDebug()) {
        logger.debug(this, "coolant temperature: %.1f, exterior temperature %.1f", status.temperatureCoolant / 10.0f,
                status.temperatureExterior / 10.0f);
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
    CanIOConfiguration *config = (CanIOConfiguration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new CanIOConfiguration();
        setConfiguration(config);
    }

    Device::loadConfiguration(); // call parent
    logger.info(this, "CAN I/O configuration:");

#ifdef USE_HARD_CODED
    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
//        prefsHandler->read(EESYS_, &config->);
    } else {
//        config-> = 0;
        saveConfiguration();
    }
//    logger.info(getId(), "xyz: %d", config->);
}

void CanIO::saveConfiguration()
{
//    prefsHandler->write(EESYS_, config->);
    prefsHandler->saveChecksum();
}

