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
BrusaNLG5::BrusaNLG5() : Device()
{
    canHandlerEv = CanHandler::getInstanceEV();
    prefsHandler = new PrefHandler(BRUSA_NLG5);
    commonName = "Brusa NLG5 Charger";

    errorPresent = false;
    clearErrorLatch = false;
    mainsCurrent = 0;
    mainsVoltage = 0;
    batteryVoltage = 0;
    batteryCurrent = 0;
    currentLimitControlPilot = 0;
    currentLimitPowerIndicator = 0;
    auxBatteryVoltage = 0;
    extChargeBalance = 0;
    boosterOutputCurrent = 0;
    temperaturePowerStage = 0;
    temperatureExtSensor1 = 0;
    temperatureExtSensor2 = 0;
    temperatureExtSensor3 = 0;
}

/*
 * Setup the device if it is enabled in configuration.
 */
void BrusaNLG5::setup()
{
    tickHandler->detach(this);

    Logger::info("add device: %s (id: %X, %X)", commonName, BRUSA_NLG5, this);

    loadConfiguration();
    Device::setup(); // call parent

    // register ourselves as observer of 0x26a-0x26f can frames
    canHandlerEv->attach(this, CAN_MASKED_ID, CAN_MASK, false);

    tickHandler->attach(this, CFG_TICK_INTERVAL_CHARGE_NLG5);
}

/*
 * Process event from the tick handler.
 */
void BrusaNLG5::handleTick()
{
    Device::handleTick(); // call parent
    sendControl();
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
    prepareOutputFrame(CAN_ID_COMMAND);

    if ((status->getSystemState() == Status::running) && systemIO->getEnableInput()) {
        outputFrame.data.bytes[0] |= enable;
    }
    if (errorPresent && clearErrorLatch) {
        outputFrame.data.bytes[0] |= errorLatch;
        clearErrorLatch = false;
    }
    outputFrame.data.bytes[1] = (config->maxMainsCurrent & 0xFF00) >> 8;
    outputFrame.data.bytes[2] = (config->maxMainsCurrent & 0x00FF);

    uint16_t batteryOutputVoltage = 0; // TODO: calculate desired output voltage to battery
    outputFrame.data.bytes[3] = (batteryOutputVoltage & 0xFF00) >> 8;
    outputFrame.data.bytes[4] = (batteryOutputVoltage & 0x00FF);

    uint16_t batteryOutputCurrent = 0; // TODO: calculate desired output current to battery
    outputFrame.data.bytes[5] = (batteryOutputCurrent & 0xFF00) >> 8;
    outputFrame.data.bytes[6] = (batteryOutputCurrent & 0x00FF);

    //TODO: enable sending the can frames once it's save
//    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Prepare the CAN transmit frame.
 * Re-sets all parameters in the re-used frame.
 */
void BrusaNLG5::prepareOutputFrame(uint32_t id)
{
    outputFrame.length = 8;
    outputFrame.id = id;
    outputFrame.extended = 0;
    outputFrame.rtr = 0;

    outputFrame.data.bytes[0] = 0;
    outputFrame.data.bytes[1] = 0;
    outputFrame.data.bytes[2] = 0;
    outputFrame.data.bytes[3] = 0;
    outputFrame.data.bytes[4] = 0;
    outputFrame.data.bytes[5] = 0;
    outputFrame.data.bytes[6] = 0;
    outputFrame.data.bytes[7] = 0;
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

        default:
            Logger::warn(BRUSA_NLG5, "received unknown frame id %X", frame->id);
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

    //TODO: handle bit field values
//    limitPowerMaximum
//    limitPowerControlPilot
//    limitPowerIndicatorInput
//    limitMainsCurrent
//    limitBatteryCurrent
//    limitBatteryVoltage
//    bypassDetection1
//    bypassDetection2
//    controlPilotSignal
//    usMainsLevel2
//    usMainsLevel1
//    euMains
//    coolingFan
//    warning
//    error
//    hardwareEnabled
//    automaticChargingActive
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
    mainsCurrent = (uint16_t)(data[1] | (data[0] << 8));
    mainsVoltage = (uint16_t)(data[3] | (data[2] << 8));
    batteryVoltage = (uint16_t)(data[5] | (data[4] << 8));
    batteryCurrent = (uint16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "mains: %fV, %fA, battery: %fV, %fA", (float) mainsVoltage / 10.0F, (float) mainsCurrent / 100.0F, (float) batteryVoltage / 10.0F, (float) batteryCurrent / 100.0F);
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
    temperaturePowerStage = (uint16_t)(data[1] | (data[0] << 8));
    temperatureExtSensor1 = (uint16_t)(data[3] | (data[2] << 8));
    temperatureExtSensor2 = (uint16_t)(data[5] | (data[4] << 8));
    temperatureExtSensor3 = (uint16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "Temp power stage: %f°C, Temp ext sensor 1: %f°C", (float) temperaturePowerStage / 10.0F, (float) temperatureExtSensor1 / 10.0F);
        Logger::debug(BRUSA_NLG5, "Temp ext sensor 2: %f°C, Temp ext sensor 3: %f°C", (float) temperatureExtSensor2 / 10.0F, (float) temperatureExtSensor3 / 10.0F);
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

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "error bitfield: %X", bitfield);
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

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_NLG5, "warning bitfield: %X", bitfield);
    }

}

/*
 * Return the device type
 */
DeviceType BrusaNLG5::getType()
{
    return (DEVICE_CHARGER);
}

/*
 * Return the device id of this device
 */
DeviceId BrusaNLG5::getId()
{
    return BRUSA_NLG5;
}

/*
 * Expose the tick interval of this controller
 */
uint32_t BrusaNLG5::getTickInterval()
{
    return CFG_TICK_INTERVAL_CHARGE_NLG5;
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

    Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        Logger::debug(BRUSA_NLG5, (char *) Constants::validChecksum);

        prefsHandler->read(CHRG_MAX_MAINS_CURRENT, &config->maxMainsCurrent);
        prefsHandler->read(CHRG_CONSTANT_CURRENT, &config->constantCurrent);
        prefsHandler->read(CHRG_CONSTANT_VOLTAGE, &config->constantVoltage);
        prefsHandler->read(CHRG_TERMINATE_CURRENT, &config->terminateCurrent);
        prefsHandler->read(CHRG_MIN_BATTERY_VOLTAGE, &config->minimumBatteryVoltage);
        prefsHandler->read(CHRG_MAX_BATTERY_VOLTAGE, &config->maximumBatteryVoltage);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn(BRUSA_NLG5, (char *) Constants::invalidChecksum);
        config->maxMainsCurrent = 160;
        config->constantCurrent = 100;
        config->constantVoltage = 1000;
        config->terminateCurrent = 30;
        config->minimumBatteryVoltage = 2000;
        config->maximumBatteryVoltage = 3000;
        saveConfiguration();
    }

    Logger::debug(BRUSA_NLG5, "max mains current: %fA, constant current: %fA, constant voltage: %fV", (float) config->maxMainsCurrent / 10.0F, (float) config->constantCurrent / 10.0F, (float) config->constantVoltage / 10.0F);
    Logger::debug(BRUSA_NLG5, "terminate current: %fA, battery min: %fV max: %fV", (float) config->terminateCurrent / 10.0F, (float) config->minimumBatteryVoltage / 10.0F, (float) config->maximumBatteryVoltage / 10.0F);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaNLG5::saveConfiguration()
{
    BrusaNLG5Configuration *config = (BrusaNLG5Configuration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(CHRG_MAX_MAINS_CURRENT, config->maxMainsCurrent);
    prefsHandler->write(CHRG_CONSTANT_CURRENT, config->constantCurrent);
    prefsHandler->write(CHRG_CONSTANT_VOLTAGE, config->constantVoltage);
    prefsHandler->write(CHRG_TERMINATE_CURRENT, config->terminateCurrent);
    prefsHandler->write(CHRG_MIN_BATTERY_VOLTAGE, config->minimumBatteryVoltage);
    prefsHandler->write(CHRG_MAX_BATTERY_VOLTAGE, config->maximumBatteryVoltage);

    prefsHandler->saveChecksum();
}
