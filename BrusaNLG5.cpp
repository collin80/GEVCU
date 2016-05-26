/*
 * BrusaNLG5.cpp
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

#include "BrusaNLG5.h"

/*
 * Constructor
 */
BrusaNLG5::BrusaNLG5() : Charger()
{
    prefsHandler = new PrefHandler(BRUSA_NLG5);
    commonName = "Brusa NLG5 Charger";

    errorPresent = false;
    clearErrorLatch = false;
    currentLimitControlPilot = 0;
    currentLimitPowerIndicator = 0;
    auxBatteryVoltage = 0;
    extChargeBalance = 0;
    boosterOutputCurrent = 0;
    temperatureExtSensor1 = 0;
    temperatureExtSensor2 = 0;
    temperatureExtSensor3 = 0;
    bitfield = 0;
    canTickCounter = 0;
}

/**
 * Tear down the controller in a safe way.
 */
void BrusaNLG5::tearDown()
{
    Charger::tearDown();

    canHandlerEv.detach(this, CAN_MASKED_ID, CAN_MASK);
    sendControl();
}

/*
 * Process event from the tick handler.
 */
void BrusaNLG5::handleTick()
{
    Charger::handleTick(); // call parent
    sendControl();

    // check if we get a status message, if not received for 1 sec, the charger is not ready
    if (canTickCounter < 1000) {
        canTickCounter++;
    } else {
        ready = false;
        running = false;
    }
}

/*
 * Send NLG5_CTL message to the charger.
 *
 * The message is used to set the operation mode, enable the charger
 * and set the current/voltage.
 */
void BrusaNLG5::sendControl()
{
    BrusaNLG5Configuration *config = (BrusaNLG5Configuration *) getConfiguration();
    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_COMMAND);

    if (powerOn && (ready || running)) {
        outputFrame.data.bytes[0] |= enable;
    }
//    if (errorPresent && clearErrorLatch) {
//        outputFrame.data.bytes[0] |= errorLatch;
//        clearErrorLatch = false;
//    }
    outputFrame.data.bytes[1] = (constrain(config->maximumInputCurrent, 0, 500) & 0xFF00) >> 8;
    outputFrame.data.bytes[2] = (constrain(config->maximumInputCurrent, 0, 500) & 0x00FF);

    uint16_t voltage = calculateOutputVoltage();
    outputFrame.data.bytes[3] = (constrain(voltage, 0, 10000) & 0xFF00) >> 8;
    outputFrame.data.bytes[4] = (constrain(voltage, 0, 10000) & 0x00FF);

    uint16_t current = calculateOutputCurrent();
    outputFrame.data.bytes[5] = (constrain(current, 0, 1500) & 0xFF00) >> 8;
    outputFrame.data.bytes[6] = (constrain(current, 0, 1500) & 0x00FF);
    outputFrame.length = 7;

    canHandlerEv.sendFrame(outputFrame);
}

/**
 * act on state changes to register ourselves at the tick handler.
 * This is special for chargers as they should not run while driving in
 * order not to consume CPU cycles unnecessarily.
 */
void BrusaNLG5::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Charger::handleStateChange(oldState, newState);

    if (newState == Status::charging) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_CHARGE_NLG5);
        canHandlerEv.attach(this, CAN_MASKED_ID, CAN_MASK, false);
        canTickCounter = 0;
    } else {
        tearDown();
    }
}

/*
 * Processes an event from the CanHandler.
 *
 * In case a CAN message was received which pass the masking and id filtering,
 * this method is called. Depending on the ID of the CAN message, the data of
 * the incoming message is processed.
 */
void BrusaNLG5::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
        case CAN_ID_STATUS:
            processStatus(frame->data.bytes);
            break;

        case CAN_ID_VALUES_1:
            processValues1(frame->data.bytes);
            break;

        case CAN_ID_VALUES_2:
            processValues2(frame->data.bytes);
            break;

        case CAN_ID_TEMPERATURE:
            processTemperature(frame->data.bytes);
            break;

        case CAN_ID_ERROR:
            processError(frame->data.bytes);
            break;
    }
}

/*
 * Process a NLG5_ST message which was received from the charger.
 *
 * This message provides the general status of the charger as well as
 * active limitations.
 */
void BrusaNLG5::processStatus(uint8_t data[])
{
    bitfield = (uint32_t)((data[1] << 0) | (data[0] << 8) | (data[3] << 16) | (data[2] << 24));

    canTickCounter = 0;
    ready = true;
    running = bitfield & hardwareEnabled;

    if (bitfield & error) {
        Logger::error(BRUSA_NLG5, "Charger reported an error, terminating charge.");
        //TODO check if this needs to be re-enabled !
//        status->setSystemState(Status::error);
    }

//    bypassDetection1
//    bypassDetection2
//    controlPilotSignal
//    usMainsLevel2
//    usMainsLevel1
//    euMains
//    coolingFan
//    warning
//    automaticChargingActive

//    limitPowerMaximum
//    limitPowerControlPilot
//    limitPowerIndicatorInput
//    limitMainsCurrent
//    limitBatteryCurrent
//    limitBatteryVoltage
//    limitBatteryTemperature
//    limitTransformerTemperature
//    limitDiodesTemperature
//    limitPowerStageTemperature
//    limitCapacitorTemperature
//    limitMaximumOutputVoltage
//    limitMaximumOutputCurrent
//    limitMaximumMainsCurrent

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "status bitfield: %X", bitfield);
    }
}

/*
 * Process a NLG5_ACT_I message which was received from the charger.
 *
 * This message provides actual values.
 */
void BrusaNLG5::processValues1(uint8_t data[])
{
    inputCurrent = (uint16_t)(data[1] | (data[0] << 8));
    inputVoltage = (uint16_t)(data[3] | (data[2] << 8));
    batteryVoltage = (uint16_t)(data[5] | (data[4] << 8));
    batteryCurrent = (uint16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "mains: %fV, %fA, battery: %fV, %fA", (float) inputVoltage / 10.0F, (float) inputCurrent / 100.0F, (float) batteryVoltage / 10.0F, (float) batteryCurrent / 100.0F);
    }
}

/*
 * Process a NLG5_ACT_II message which was received from the charger.
 *
 * This message provides actual values.
 */
void BrusaNLG5::processValues2(uint8_t data[])
{
    currentLimitControlPilot = (uint16_t)(data[1] | (data[0] << 8));
    currentLimitPowerIndicator = (uint8_t)data[2];
    auxBatteryVoltage = (uint8_t)data[3];
    extChargeBalance = (uint16_t)(data[5] | (data[4] << 8));
    boosterOutputCurrent = (uint16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "current limit by control pilot: %fA, by power indicator: %fA", (float) currentLimitControlPilot / 10.0F, (float) currentLimitPowerIndicator / 10.0F);
        Logger::debug(BRUSA_NLG5, "aux battery: %fV, external charge balance: %fAh, booster output; %fA", (float) auxBatteryVoltage / 10.0F, (float) extChargeBalance / 100.0F, (float) boosterOutputCurrent / 100.0F);
    }
}

/*
 * Process a NLG5_TEMP message which was received from the charger.
 *
 * This message provides various temperature readings.
 */
void BrusaNLG5::processTemperature(uint8_t data[])
{
    temperature = (uint16_t)(data[1] | (data[0] << 8));
    temperatureExtSensor1 = (uint16_t)(data[3] | (data[2] << 8));
    temperatureExtSensor2 = (uint16_t)(data[5] | (data[4] << 8));
    temperatureExtSensor3 = (uint16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "Temp power stage: %fC, Temp ext sensor 1: %C", (float) temperature / 10.0F, (float) temperatureExtSensor1 / 10.0F);
        Logger::debug(BRUSA_NLG5, "Temp ext sensor 2: %fC, Temp ext sensor 3: %fC", (float) temperatureExtSensor2 / 10.0F, (float) temperatureExtSensor3 / 10.0F);
    }
}

/*
 * Process a NLG5_ERR message which was received from the charger.
 *
 * This message provides errors and warnings.
 */
void BrusaNLG5::processError(uint8_t data[])
{
    bitfield = (uint32_t)((data[1] << 0) | (data[0] << 8) | (data[3] << 16) | (data[2] << 24));

    //TODO: handle bit field values
//    extTemperatureSensor3Defect
//    extTemperatureSensor2Defect
//    extTemperatureSensor1Defect
//    temperatureSensorTransformer
//    temperatureSensorDiodes
//    temperatureSensorPowerStage
//    temperatureSensorCapacitor
//    batteryPolarity
//    mainFuseDefective
//    outputFuseDefective
//    mainsVoltagePlausibility
//    batteryVoltagePlausibility
//    shortCircuit
//    mainsOvervoltage1
//    mainsOvervoltage2
//    batteryOvervoltage
//    emergencyChargeTime
//    emergencyAmpHours
//    emergencyBatteryVoltage
//    emergencyBatteryTemperature
//    canReceiveOverflow
//    canTransmitOverflow
//    canOff
//    canTimeout
//    initializationError
//    watchDogTimeout
//    crcPowerEeprom
//    crcSystemEeprom
//    crcNVSRAM
//    crcFlashMemory

    if (bitfield != 0) {
        Logger::error(BRUSA_NLG5, "%X (%B)", bitfield, bitfield);
    }

    bitfield = (uint32_t)data[4];

    //TODO: handle bit field values
//    saveCharging
//    ledDriver
//    controlMessageNotActive
//    valueOutOfRange
//    limitOvertemperature
//    limitLowBatteryVoltage
//    limitLowMainsVoltage

    if (bitfield != 0) {
        Logger::warn(BRUSA_NLG5, "%X (%B)", bitfield, bitfield);
    }
}

/*
 * Return the device id of this device
 */
DeviceId BrusaNLG5::getId()
{
    return BRUSA_NLG5;
}

/*
 * Load configuration data from EEPROM.
 *
 * If not available or the checksum is invalid, default values are chosen.
 */
void BrusaNLG5::loadConfiguration()
{
    BrusaNLG5Configuration *config = (BrusaNLG5Configuration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new BrusaNLG5Configuration();
        setConfiguration(config);
    }

    Charger::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
    }
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaNLG5::saveConfiguration()
{
    BrusaNLG5Configuration *config = (BrusaNLG5Configuration *) getConfiguration();

    Charger::saveConfiguration(); // call parent

    prefsHandler->saveChecksum();
}
