/*
 * BrusaDMC5.cpp
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

#include "BrusaDMC5.h"

/*
 Warning:
 At high speed disable the DMC_EnableRq can be dangerous because a field weakening current is
 needed to achieve zero torque. Switching off the DMC in such a situation will make heavy regenerat-
 ing torque that can't be controlled.
 */

/*
 * Constructor
 */
BrusaDMC5::BrusaDMC5() : MotorController()
{
    prefsHandler = new PrefHandler(BRUSA_DMC5);
    torqueAvailable = 0;
    maxPositiveTorque = 0;
    minNegativeTorque = 0;
    limiterStateNumber = 0;

    tickCounter = 0;

    commonName = "Brusa DMC5 Inverter";
}

/*
 * Setup the device if it is enabled in configuration.
 */
void BrusaDMC5::setup()
{
    tickHandler->detach(this);

    Logger::info("add device: Brusa DMC5 (id: %X, %X)", BRUSA_DMC5, this);

    loadConfiguration();
    MotorController::setup(); // run the parent class version of this function

    // register ourselves as observer of 0x258-0x268 and 0x458 can frames
    canHandlerEv->attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
    canHandlerEv->attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);

    tickHandler->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA);
}

/*
 * Process event from the tick handler.
 *
 * The super-class requests desired levels from the throttle and
 * brake and decides which one to apply.
 */
void BrusaDMC5::handleTick()
{
    MotorController::handleTick(); // call parent
    tickCounter++;

    sendControl();  // send CTRL every 20ms

    if (tickCounter > 4) {
        sendControl2(); // send CTRL_2 every 100ms
        sendLimits();   // send LIMIT every 100ms
        tickCounter = 0;
    }
}

/*
 * Send DMC_CTRL message to the motor controller.
 *
 * This message controls the power-stage in the controller, clears the error latch
 * in case errors were detected and requests the desired torque / speed.
 */
void BrusaDMC5::sendControl()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();
    prepareOutputFrame(CAN_ID_CONTROL);

    speedRequested = 0;
    torqueRequested = 0;

    outputFrame.data.bytes[0] = enablePositiveTorqueSpeed; // | enableNegativeTorqueSpeed;

//TODO: clarify how to handle the "clear error latch" function
//    if (faulted) {
//        outputFrame.data.bytes[0] |= clearErrorLatch;
//    } else {
        if ((status->getSystemState() == Status::running || speedActual > 1000) && systemIO->getEnableInput()) {   // see warning about field weakening current to prevent uncontrollable regen
            outputFrame.data.bytes[0] |= enablePowerStage;
        }

        if (status->getSystemState() == Status::running) {
            if (config->enableOscillationLimiter) {
                outputFrame.data.bytes[0] |= enableOscillationLimiter;
            }

            if (powerMode == modeSpeed) {
                outputFrame.data.bytes[0] |= enableSpeedMode;
                speedRequested = throttleRequested * config->speedMax / 1000;
                torqueRequested = config->torqueMax; // positive number used for both speed directions
            } else { // torque mode
                speedRequested = config->speedMax; // positive number used for both torque directions
                torqueRequested = throttleRequested * config->torqueMax / 1000;
            }

            // set the speed in rpm
            outputFrame.data.bytes[2] = (speedRequested & 0xFF00) >> 8;
            outputFrame.data.bytes[3] = (speedRequested & 0x00FF);

            // set the torque in 0.01Nm (GEVCU uses 0.1Nm -> multiply by 10)
            outputFrame.data.bytes[4] = ((torqueRequested * 10) & 0xFF00) >> 8;
            outputFrame.data.bytes[5] = ((torqueRequested * 10) & 0x00FF);
        }
//    }

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "requested Speed: %l rpm, requested Torque: %f Nm", speedRequested, (float) torqueRequested / 10.0F);
    }

    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Send DMC_CTRL2 message to the motor controller.
 *
 * This message controls the mechanical power limits for motor- and regen-mode.
 */
void BrusaDMC5::sendControl2()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    prepareOutputFrame(CAN_ID_CONTROL_2);
    outputFrame.data.bytes[0] = ((config->torqueSlewRate * 10) & 0xFF00) >> 8;
    outputFrame.data.bytes[1] = ((config->torqueSlewRate * 10) & 0x00FF);
    outputFrame.data.bytes[2] = (config->speedSlewRate & 0xFF00) >> 8;
    outputFrame.data.bytes[3] = (config->speedSlewRate & 0x00FF);
    outputFrame.data.bytes[4] = (config->maxMechanicalPowerMotor & 0xFF00) >> 8;
    outputFrame.data.bytes[5] = (config->maxMechanicalPowerMotor & 0x00FF);
    outputFrame.data.bytes[6] = (config->maxMechanicalPowerRegen & 0xFF00) >> 8;
    outputFrame.data.bytes[7] = (config->maxMechanicalPowerRegen & 0x00FF);

    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Send DMC_LIM message to the motor controller.
 *
 * This message controls the electrical limits in the controller.
 */
void BrusaDMC5::sendLimits()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    prepareOutputFrame(CAN_ID_LIMIT);
    outputFrame.data.bytes[0] = (config->dcVoltLimitMotor & 0xFF00) >> 8;
    outputFrame.data.bytes[1] = (config->dcVoltLimitMotor & 0x00FF);
    outputFrame.data.bytes[2] = (config->dcVoltLimitRegen & 0xFF00) >> 8;
    outputFrame.data.bytes[3] = (config->dcVoltLimitRegen & 0x00FF);
    outputFrame.data.bytes[4] = (config->dcCurrentLimitMotor & 0xFF00) >> 8;
    outputFrame.data.bytes[5] = (config->dcCurrentLimitMotor & 0x00FF);
    outputFrame.data.bytes[6] = (config->dcCurrentLimitRegen & 0xFF00) >> 8;
    outputFrame.data.bytes[7] = (config->dcCurrentLimitRegen & 0x00FF);

    canHandlerEv->sendFrame(outputFrame);
}

/*
 * Prepare the CAN transmit frame.
 * Re-sets all parameters in the re-used frame.
 */
void BrusaDMC5::prepareOutputFrame(uint32_t id)
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
void BrusaDMC5::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
        case CAN_ID_STATUS:
            processStatus(frame->data.bytes);
            break;

        case CAN_ID_ACTUAL_VALUES:
            processActualValues(frame->data.bytes);
            break;

        case CAN_ID_ERRORS:
            processErrors(frame->data.bytes);
            break;

        case CAN_ID_TORQUE_LIMIT:
            processTorqueLimit(frame->data.bytes);
            break;

        case CAN_ID_TEMP:
            processTemperature(frame->data.bytes);
            break;

//        default:
//            Logger::warn(BRUSA_DMC5, "received unknown frame id %X", frame->id);
    }
}

/*
 * Process a DMC_TRQS message which was received from the motor controller.
 *
 * This message provides the general status of the controller as well as
 * available and current torque and speed.
 */
void BrusaDMC5::processStatus(uint8_t data[])
{
    bitfield = (uint32_t)(data[1] | (data[0] << 8));
    torqueAvailable = (int16_t)(data[3] | (data[2] << 8)) / 10;
    torqueActual = (int16_t)(data[5] | (data[4] << 8)) / 10;
    speedActual = (int16_t)(data[7] | (data[6] << 8));

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "status: %X, torque avail: %fNm, actual torque: %fNm, speed actual: %drpm", bitfield, (float) torqueAvailable / 100.0F, (float) torqueActual / 100.0F, speedActual);
    }

    if ((bitfield & stateReady) && status->getSystemState() == Status::preCharged) {
        status->setSystemState(Status::ready);
    }
    if ((bitfield & stateRunning) && status->getSystemState() == Status::ready) {
        status->setSystemState(Status::running);
    }
    if ((bitfield & errorFlag) && status->getSystemState() != Status::error) {
        status->setSystemState(Status::error);
    }
    status->warning = (bitfield & warningFlag) ? true : false;
    status->limitationTorque = (bitfield & torqueLimitation) ? true : false;
    status->limitationMotorModel = (bitfield & motorModelLimitation) ? true : false;
    status->limitationMechanicalPower = (bitfield & mechanicalPowerLimitation) ? true : false;
    status->limitationMaxTorque = (bitfield & maxTorqueLimitation) ? true : false;
    status->limitationAcCurrent = (bitfield & acCurrentLimitation) ? true : false;
    status->limitationControllerTemperature = (bitfield & temperatureLimitation) ? true : false;
    status->limitationSpeed = (bitfield & speedLimitation) ? true : false;
    status->limitationDcVoltage = (bitfield & voltageLimitation) ? true : false;
    status->limitationDcCurrent = (bitfield & currentLimitation) ? true : false;
    status->limitationSlewRate = (bitfield & slewRateLimitation) ? true : false;
    status->limitationMotorTemperature = (bitfield & motorTemperatureLimitation) ? true : false;
}

/*
 * Process a DMC_ACTV message which was received from the motor controller.
 *
 * This message provides information about current electrical conditions and
 * applied mechanical power.
 */
void BrusaDMC5::processActualValues(uint8_t data[])
{
    dcVoltage = (uint16_t)(data[1] | (data[0] << 8));
    dcCurrent = (int16_t)(data[3] | (data[2] << 8));
    acCurrent = (uint16_t)(data[5] | (data[4] << 8)) / 2.5;
    mechanicalPower = (int16_t)(data[7] | (data[6] << 8)) / 6.25;

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "actual values: DC Volts: %fV, DC current: %fA, AC current: %fA, mechPower: %fkW", (float) dcVoltage / 10.0F, (float) dcCurrent / 10.0F, (float) acCurrent / 10.0F, (float) mechanicalPower / 10.0F);
    }
}

/*
 * Process a DMC_ERRS message which was received from the motor controller.
 *
 * This message provides various error and warning status flags in a bitfield.
 * The bitfield is not processed here but it is made available for other components
 * (e.g. the webserver to display the various status flags)
 */
void BrusaDMC5::processErrors(uint8_t data[])
{
    bitfield = (uint32_t)(data[1] | (data[0] << 8) | (data[5] << 16) | (data[4] << 24));

    status->speedSensorSupply = (bitfield & speedSensorSupply) ? true : false;
    status->speedSensor = (bitfield & speedSensor) ? true : false;
    status->canLimitMessageInvalid = (bitfield & canLimitMessageInvalid) ? true : false;
    status->canControlMessageInvalid = (bitfield & canControlMessageInvalid) ? true : false;
    status->canLimitMessageLost = (bitfield & canLimitMessageLost) ? true : false;
    status->overvoltageInternalSupply = (bitfield & overvoltageSkyConverter) ? true : false;
    status->voltageMeasurement = (bitfield & voltageMeasurement) ? true : false;
    status->shortCircuit = (bitfield & shortCircuit) ? true : false;
    status->canControlMessageLost = (bitfield & canControlMessageLost) ? true : false;
    status->overtempController = (bitfield & overtemp) ? true : false;
    status->overtempMotor = (bitfield & overtempMotor) ? true : false;
    status->overspeed = (bitfield & overspeed) ? true : false;
    status->hvUndervoltage = (bitfield & undervoltage) ? true : false;
    status->hvOvervoltage = (bitfield & overvoltage) ? true : false;
    status->hvOvercurrent = (bitfield & overcurrent) ? true : false;
    status->initalisation = (bitfield & initalisation) ? true : false;
    status->analogInput = (bitfield & analogInput) ? true : false;
    status->unexpectedShutdown = (bitfield & driverShutdown) ? true : false;
    status->powerMismatch = (bitfield & powerMismatch) ? true : false;
    status->canControl2MessageLost = (bitfield & canControl2MessageLost) ? true : false;
    status->motorEeprom = (bitfield & motorEeprom) ? true : false;
    status->storage = (bitfield & storage) ? true : false;
    status->enableSignalLost = (bitfield & enablePinSignalLost) ? true : false;
    status->canCommunicationStartup = (bitfield & canCommunicationStartup) ? true : false;
    status->internalSupply = (bitfield & internalSupply) ? true : false;
    status->acOvercurrent = (bitfield & acOvercurrent) ? true : false;
    status->osTrap = (bitfield & osTrap) ? true : false;

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "errors: %X", bitfield);
    }

    bitfield = (uint32_t)(data[7] | (data[6] << 8));

//    status-> systemCheckActive = (bitfield & systemCheckActive) ? true : false;
    status-> externalShutdownPath2Off = (bitfield & externalShutdownPathAw2Off) ? true : false;
    status-> externalShutdownPath1Off = (bitfield & externalShutdownPathAw1Off) ? true : false;
    status-> oscillationLimitControllerActive = (bitfield & oscillationLimitControllerActive) ? true : false;
    status-> driverShutdownPathActive = (bitfield & driverShutdownPathActive) ? true : false;
    status-> powerMismatch = (bitfield & powerMismatchDetected) ? true : false;
    status-> speedSensorSignal = (bitfield & speedSensorSignal) ? true : false;
    status-> hvUndervoltage = (bitfield & hvUndervoltage) ? true : false;
    status-> maximumModulationLimiter = (bitfield & maximumModulationLimiter) ? true : false;
    status-> temperatureSensor = (bitfield & temperatureSensor) ? true : false;

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "warning: %X", bitfield);
    }
}

/*
 * Process a DMC_TRQS2 message which was received from the motor controller.
 *
 * This message provides information about available torque.
 */
void BrusaDMC5::processTorqueLimit(uint8_t data[])
{
    maxPositiveTorque = (int16_t)(data[1] | (data[0] << 8)) / 10;
    minNegativeTorque = (int16_t)(data[3] | (data[2] << 8)) / 10;
    limiterStateNumber = (uint8_t) data[4];

    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "torque limit: max positive: %fNm, min negative: %fNm", (float) maxPositiveTorque / 10.0F, (float) minNegativeTorque / 10.0F, limiterStateNumber);
    }
}

/*
 * Process a DMC_TEMP message which was received from the motor controller.
 *
 * This message provides information about motor and inverter temperatures.
 */
void BrusaDMC5::processTemperature(uint8_t data[])
{
    int16_t temperaturePowerStage;
    temperaturePowerStage = (int16_t)(data[1] | (data[0] << 8)) * 5;
    temperatureMotor = (int16_t)(data[3] | (data[2] << 8)) * 5;
    temperatureController = (int16_t)(data[4] - 50) * 10;
    if (Logger::isDebug()) {
        Logger::debug(BRUSA_DMC5, "temperature: powerStage: %fC, motor: %fC, system: %fC", (float) temperaturePowerStage / 10.0F, (float) temperatureMotor / 10.0F, (float) temperatureController / 10.0F);
    }
    if (temperaturePowerStage > temperatureController) {
        temperatureController = temperaturePowerStage;
    }
}

/*
 * Return the device id of this device
 */
DeviceId BrusaDMC5::getId()
{
    return BRUSA_DMC5;
}

/*
 * Expose the tick interval of this controller
 */
uint32_t BrusaDMC5::getTickInterval()
{
    return CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA;
}

/*
 * Load configuration data from EEPROM.
 *
 * If not available or the checksum is invalid, default values are chosen.
 */
void BrusaDMC5::loadConfiguration()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new BrusaDMC5Configuration();
        setConfiguration(config);
    }

    MotorController::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        Logger::debug(BRUSA_DMC5, (char *) Constants::validChecksum);

        prefsHandler->read(EEMC_MAX_MECH_POWER_MOTOR, &config->maxMechanicalPowerMotor);
        prefsHandler->read(EEMC_MAX_MECH_POWER_REGEN, &config->maxMechanicalPowerRegen);
        prefsHandler->read(EEMC_DC_VOLT_LIMIT_MOTOR, &config->dcVoltLimitMotor);
        prefsHandler->read(EEMC_DC_VOLT_LIMIT_REGEN, &config->dcVoltLimitRegen);
        prefsHandler->read(EEMC_DC_CURRENT_LIMIT_MOTOR, &config->dcCurrentLimitMotor);
        prefsHandler->read(EEMC_DC_CURRENT_LIMIT_REGEN, &config->dcCurrentLimitRegen);
        prefsHandler->read(EEMC_OSCILATION_LIMITER, &temp);
        config->enableOscillationLimiter = (temp != 0);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn(BRUSA_DMC5, (char *) Constants::invalidChecksum);
        config->maxMechanicalPowerMotor = 50000;
        config->maxMechanicalPowerRegen = 0; //TODO: 50000; don't want regen yet !
        config->dcVoltLimitMotor = 1000;
        config->dcVoltLimitRegen =  0;//TODO: 1000; don't want regen yet !;
        config->dcCurrentLimitMotor = 0;
        config->dcCurrentLimitRegen = 0;
        config->enableOscillationLimiter = false;
        saveConfiguration();
    }
    Logger::debug(BRUSA_DMC5, "Max mech power motor: %d kW, max mech power regen: %d ", config->maxMechanicalPowerMotor, config->maxMechanicalPowerRegen);
    Logger::debug(BRUSA_DMC5, "DC limit motor: %d Volt, DC limit regen: %d Volt", config->dcVoltLimitMotor, config->dcVoltLimitRegen);
    Logger::debug(BRUSA_DMC5, "DC limit motor: %d Amps, DC limit regen: %d Amps", config->dcCurrentLimitMotor, config->dcCurrentLimitRegen);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaDMC5::saveConfiguration()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    MotorController::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_MAX_MECH_POWER_MOTOR, config->maxMechanicalPowerMotor);
    prefsHandler->write(EEMC_MAX_MECH_POWER_REGEN, config->maxMechanicalPowerRegen);
    prefsHandler->write(EEMC_DC_VOLT_LIMIT_MOTOR, config->dcVoltLimitMotor);
    prefsHandler->write(EEMC_DC_VOLT_LIMIT_REGEN, config->dcVoltLimitRegen);
    prefsHandler->write(EEMC_DC_CURRENT_LIMIT_MOTOR, config->dcCurrentLimitMotor);
    prefsHandler->write(EEMC_DC_CURRENT_LIMIT_REGEN, config->dcCurrentLimitRegen);
    prefsHandler->write(EEMC_OSCILATION_LIMITER, (uint8_t)(config->enableOscillationLimiter ? 1 : 0));
    prefsHandler->saveChecksum();
}
