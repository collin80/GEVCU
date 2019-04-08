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
    mechanicalPower = 0;
    torqueAvailable = 0;
    maxPositiveTorque = 0;
    minNegativeTorque = 0;
    limiterStateNumber = 0;
    tickCounter = 0;
    bitfield = 0;
    canMessageLost = false;

    commonName = "Brusa DMC5 Inverter";
}

/**
 * Tear down the controller in a safe way.
 */
void BrusaDMC5::tearDown()
{
    MotorController::tearDown();

    canHandlerEv.detach(this, CAN_MASKED_ID_1, CAN_MASK_1);
    canHandlerEv.detach(this, CAN_MASKED_ID_2, CAN_MASK_2);

    // for safety reasons at power off, first request 0 torque
    // this allows the controller to dissipate residual fields first
    sendControl();
}

/**
 * act on messages the super-class does not react upon, like state change
 * to ready or running which should enable/disable the controller
 */
void BrusaDMC5::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    MotorController::handleStateChange(oldState, newState);

    // as the DMC is not sending/receiving messages as long as the enable signal is low
    // (state != ready / running), attach to can/tick handler only when running.
    if (newState == Status::running) {
        // register ourselves as observer of 0x258-0x268 and 0x458 can frames
        canHandlerEv.attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
        canHandlerEv.attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);
        tickHandler.attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA);
    } else {
        if (oldState == Status::running) {
            tearDown();
        }
    }
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

    if (tickCounter == 2) {
        sendControl2(); // send CTRL_2 every 100ms
    }
    if (tickCounter == 4) {
        sendLimits();   // send LIMIT every 100ms
    }
    if (tickCounter > 4) {
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
    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_CONTROL);
    if (canMessageLost) {
        outputFrame.data.bytes[0] |= clearErrorLatch;
        Logger::warn(this, "clearing error latch");
    } else {
        // to safe energy only enable the power-stage when positive acceleration is requested or the motor is still spinning (to control regen)
        // see warning in Brusa docs about field weakening current to prevent uncontrollable regen
        if (ready && ((getThrottleLevel() > 0 && powerOn) || (speedActual != 0))) {
            outputFrame.data.bytes[0] |= enablePowerStage;
        }

        if (powerOn && ready) {
            int16_t speedCommand = getSpeedRequested();
            int16_t torqueCommand = getTorqueRequested();

            outputFrame.data.bytes[0] |= (config->invertDirection ^ (getGear() == REVERSE) ? enableNegativeTorqueSpeed : enablePositiveTorqueSpeed);

            if (config->powerMode == modeSpeed) {
                outputFrame.data.bytes[0] |= enableSpeedMode;
                if (config->invertDirection ^ (getGear() == REVERSE)) { // reverse the motor direction if specified
                    speedCommand *= -1;
                }
            } else {
                if (config->invertDirection ^ (getGear() == REVERSE)) { // reverse the motor direction if specified
                    torqueCommand *= -1;
                }
            }

            if (config->enableOscillationLimiter) {
                outputFrame.data.bytes[0] |= enableOscillationLimiter;
            }

            // set the speed in rpm, the values are constrained to prevent a fatal overflow
            outputFrame.data.bytes[2] = (constrain(speedCommand, -32760, 32760) & 0xFF00) >> 8;
            outputFrame.data.bytes[3] = (constrain(speedCommand, -32760, 32760) & 0x00FF);

            // set the torque in 0.01Nm (GEVCU uses 0.1Nm -> multiply by 10), the values are constrained to prevent a fatal overflow
            outputFrame.data.bytes[4] = ((constrain(torqueCommand, -3275, 3275) * 10) & 0xFF00) >> 8;
            outputFrame.data.bytes[5] = ((constrain(torqueCommand, -3275, 3275) * 10) & 0x00FF);
        }
    }
    canHandlerEv.sendFrame(outputFrame);
}

/*
 * Send DMC_CTRL2 message to the motor controller.
 *
 * This message controls the mechanical power limits for motor- and regen-mode.
 */
void BrusaDMC5::sendControl2()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_CONTROL_2);
// slew rate is not working in firmware --> use GEVCU's own implementation and leave value at zero
//    outputFrame.data.bytes[0] = ((constrain(config->slewRate, 0, 100) * 655) & 0xFF00) >> 8;
//    outputFrame.data.bytes[1] = ((constrain(config->slewRate, 0, 100) * 655) & 0x00FF);
//    outputFrame.data.bytes[2] = ((constrain(config->slewRate, 0, 100) * 655) & 0xFF00) >> 8;
//    outputFrame.data.bytes[3] = ((constrain(config->slewRate, 0, 100) * 655) & 0x00FF);
    outputFrame.data.bytes[4] = ((constrain(config->maxMechanicalPowerMotor, 0, 2621) * 25) & 0xFF00) >> 8;
    outputFrame.data.bytes[5] = ((constrain(config->maxMechanicalPowerMotor, 0, 2621) * 25) & 0x00FF);
    outputFrame.data.bytes[6] = ((constrain(config->maxMechanicalPowerRegen, 0, 2621) * 25) & 0xFF00) >> 8;
    outputFrame.data.bytes[7] = ((constrain(config->maxMechanicalPowerRegen, 0, 2621) * 25) & 0x00FF);

    canHandlerEv.sendFrame(outputFrame);
}

/*
 * Send DMC_LIM message to the motor controller.
 *
 * This message controls the electrical limits in the controller.
 */
void BrusaDMC5::sendLimits()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();
    BatteryManager *batteryManager = deviceManager.getBatteryManager();

    uint16_t currentLimitMotor = config->dcCurrentLimitMotor;
    uint16_t currentLimitRegen = config->dcCurrentLimitRegen;

    if (batteryManager) {
        if (batteryManager->hasDischargeLimit()) {
//            currentLimitMotor = batteryManager->getDischargeLimit() * 10;
//            Logger::info(this, "Motor power limited by BMS to %dA (instead %dA)", batteryManager->getDischargeLimit(), currentLimitMotor/10);
        } else if (batteryManager->hasAllowDischarging() && !batteryManager->isDischargeAllowed()) {
//            currentLimitMotor = 0;
//            Logger::info(this, "Motor power limited by BMS to 0");
        }

        if (batteryManager->hasChargeLimit()) {
            currentLimitRegen = batteryManager->getChargeLimit() * 10;
//            Logger::info(this, "Regen power limited by BMS to %dA (instead %dA)", batteryManager->getChargeLimit(), currentLimitRegen/10);
        } else if (batteryManager->hasAllowCharging() && !batteryManager->isChargeAllowed()) {
            currentLimitRegen = 0;
//            Logger::info(this, "Regen power limited by BMS to 0");
        }

        if (currentLimitMotor < config->dcCurrentLimitMotor) {
//            Logger::info(this, "Motor power limited by BMS");
        }
        if (currentLimitRegen < config->dcCurrentLimitRegen) {
//            Logger::info(this, "Regen power limited by BMS");
        }
    }

    canHandlerEv.prepareOutputFrame(&outputFrame, CAN_ID_LIMIT);
    outputFrame.data.bytes[0] = (constrain(config->dcVoltLimitMotor, 0, 65535) & 0xFF00) >> 8;
    outputFrame.data.bytes[1] = (constrain(config->dcVoltLimitMotor, 0, 65535) & 0x00FF);
    outputFrame.data.bytes[2] = (constrain(config->dcVoltLimitRegen, 0, 65535) & 0xFF00) >> 8;
    outputFrame.data.bytes[3] = (constrain(config->dcVoltLimitRegen, 0, 65535) & 0x00FF);
    outputFrame.data.bytes[4] = (constrain(currentLimitMotor, 0, 65535) & 0xFF00) >> 8;
    outputFrame.data.bytes[5] = (constrain(currentLimitMotor, 0, 65535) & 0x00FF);
    outputFrame.data.bytes[6] = (constrain(currentLimitRegen, 0, 65535) & 0xFF00) >> 8;
    outputFrame.data.bytes[7] = (constrain(currentLimitRegen, 0, 65535) & 0x00FF);

    canHandlerEv.sendFrame(outputFrame);
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
        reportActivity();
        break;

    case CAN_ID_ACTUAL_VALUES:
        processActualValues(frame->data.bytes);
        reportActivity();
        break;

    case CAN_ID_ERRORS:
        processErrors(frame->data.bytes);
        reportActivity();
        break;

    case CAN_ID_TORQUE_LIMIT:
        processTorqueLimit(frame->data.bytes);
        reportActivity();
        break;

    case CAN_ID_TEMP:
        processTemperature(frame->data.bytes);
        reportActivity();
        break;
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
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    bitfield = (uint32_t) (data[1] | (data[0] << 8));
    torqueAvailable = (int16_t) (data[3] | (data[2] << 8)) / 10;
    torqueActual = (int16_t) (data[5] | (data[4] << 8)) / 10;
    speedActual = (int16_t) (data[7] | (data[6] << 8));

    if (config->invertDirection ^ (getGear() == REVERSE)) {
        speedActual *= -1;
        torqueActual *= -1;
    }

    if (bitfield & errorFlag) {
//    	if (running || ready) {
            Logger::error(this, "Error reported from motor controller!");
//    	}
    	running = false;
    	ready = false;
    } else {
        ready = (bitfield & dmc5Ready) ? true : false;
        running = (bitfield & dmc5Running) ? true : false;
    }

    status.warning = (bitfield & warningFlag) ? true : false;
    status.limitationTorque = (bitfield & torqueLimitation) ? true : false;
    status.limitationMotorModel = (bitfield & motorModelLimitation) ? true : false;
    status.limitationMechanicalPower = (bitfield & mechanicalPowerLimitation) ? true : false;
    status.limitationMaxTorque = (bitfield & maxTorqueLimitation) ? true : false;
    status.limitationAcCurrent = (bitfield & acCurrentLimitation) ? true : false;
    status.limitationControllerTemperature = (bitfield & temperatureLimitation) ? true : false;
    status.limitationSpeed = (bitfield & speedLimitation) ? true : false;
    status.limitationDcVoltage = (bitfield & voltageLimitation) ? true : false;
    status.limitationDcCurrent = (bitfield & currentLimitation) ? true : false;
    status.limitationSlewRate = (bitfield & slewRateLimitation) ? true : false;
    status.limitationMotorTemperature = (bitfield & motorTemperatureLimitation) ? true : false;

    if (Logger::isDebug()) {
        Logger::debug(this, "status: %#08x, ready: %d, running: %d, torque avail: %.2fNm, actual : %.2fNm, speed actual: %drpm", bitfield,
                ready, running, torqueAvailable / 100.0F, torqueActual / 100.0F, speedActual);
    }
}

/*
 * Process a DMC_ACTV message which was received from the motor controller.
 *
 * This message provides information about current electrical conditions and
 * applied mechanical power.
 */
void BrusaDMC5::processActualValues(uint8_t data[])
{
    dcVoltage = (uint16_t) (data[1] | (data[0] << 8));
    dcCurrent = (int16_t) (data[3] | (data[2] << 8));
    acCurrent = (uint16_t) (data[5] | (data[4] << 8)) / 2.5;
    mechanicalPower = (int16_t) (data[7] | (data[6] << 8)) / 6.25;

    if (Logger::isDebug()) {
        Logger::debug(this, "DC Volts: %.1fV, DC current: %.1fA, AC current: %.1fA, mechPower: %.1fkW", dcVoltage / 10.0F,
                dcCurrent / 10.0F, acCurrent / 10.0F, mechanicalPower / 10.0F);
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
    bitfield = (uint32_t) (data[1] | (data[0] << 8) | (data[5] << 16) | (data[4] << 24));

    if (bitfield & speedSensorSupply)
        Logger::error(this, "power supply of speed sensor failed");
    if (bitfield & speedSensor)
        Logger::error(this, "encoder or position sensor delivers faulty signal");
    if (bitfield & canLimitMessageInvalid)
        Logger::error(this, "limit data CAN message is invalid");
    if (bitfield & canControlMessageInvalid)
        Logger::error(this, "control data CAN message is invalid");
    if (bitfield & canLimitMessageLost)
        Logger::error(this, "timeout of limit CAN message");
    if (bitfield & overvoltageSkyConverter)
        Logger::error(this, "over voltage of the internal power supply");
    if (bitfield & voltageMeasurement)
        Logger::error(this, "differences in the redundant voltage measurement");
    if (bitfield & shortCircuit)
        Logger::error(this, "short circuit in the power stage");
    if (bitfield & canControlMessageLost)
        Logger::error(this, "timeout of control message");
    if (bitfield & canControl2MessageLost)
        Logger::error(this, "timeout of control2 message");
    if (bitfield & canLimitMessageLost)
        Logger::error(this, "timeout of limit message");
    if (bitfield & initalisation)
        Logger::error(this, "error during initialisation");
    if (bitfield & analogInput)
        Logger::error(this, "an analog input signal is outside its boundaries");
    if (bitfield & driverShutdown)
        Logger::error(this, "power stage of the motor controller was shut-down in an uncontrolled fashion");
    if (bitfield & powerMismatch)
        Logger::error(this, "plausibility error between electrical and mechanical power");
    if (bitfield & motorEeprom)
        Logger::error(this, "error in motor/controller EEPROM module");
    if (bitfield & storage)
        Logger::error(this, "data consistency check failed in motor controller");
    if (bitfield & enablePinSignalLost)
        Logger::error(this, "enable signal lost, motor controller shut-down (is imminent)");
    if (bitfield & canCommunicationStartup)
        Logger::error(this, "motor controller received CAN messages which were not appropriate to its state");
    if (bitfield & internalSupply)
        Logger::error(this, "problem with the internal power supply");
    if (bitfield & osTrap)
        Logger::error(this, "severe problem in OS of motor controller");

    canMessageLost = ((bitfield & canControlMessageLost) | (bitfield & canControl2MessageLost) | (bitfield & canLimitMessageLost)) ? true : false;
    status.overtempController = (bitfield & overtemp) ? true : false;
    status.overtempMotor = (bitfield & overtempMotor) ? true : false;
    status.overspeed = (bitfield & overspeed) ? true : false;
    status.hvUndervoltage = (bitfield & undervoltage) ? true : false;
    status.hvOvervoltage = (bitfield & overvoltage) ? true : false;
    status.hvOvercurrent = (bitfield & overcurrent) ? true : false;
    status.acOvercurrent = (bitfield & acOvercurrent) ? true : false;

    bitfield = (uint32_t) (data[7] | (data[6] << 8));

    if (bitfield & externalShutdownPathAw2Off)
        Logger::warn(this, "external shut-down path AW1 off");
    if (bitfield & externalShutdownPathAw1Off)
        Logger::warn(this, "external shut-down path AW2 off");
    if (bitfield & driverShutdownPathActive)
        Logger::warn(this, "driver shut-down path active");
    if (bitfield & powerMismatchDetected)
        Logger::warn(this, "power mismatch detected");
    if (bitfield & speedSensorSignal)
            Logger::warn(this, "speed sensor signal is bad");
    if (bitfield & temperatureSensor)
        Logger::warn(this, "invalid data from temperature sensor(s)");

    status.systemCheckActive = (bitfield & systemCheckActive) ? true : false;
    status.oscillationLimiter = (bitfield & oscillationLimitControllerActive) ? true : false;
    status.hvUndervoltage = (bitfield & hvUndervoltage) ? true : false;
    status.maximumModulationLimiter = (bitfield & maximumModulationLimiter) ? true : false;
}

/*
 * Process a DMC_TRQS2 message which was received from the motor controller.
 *
 * This message provides information about available torque.
 */
void BrusaDMC5::processTorqueLimit(uint8_t data[])
{
    maxPositiveTorque = (int16_t) (data[1] | (data[0] << 8)) / 10;
    minNegativeTorque = (int16_t) (data[3] | (data[2] << 8)) / 10;
    limiterStateNumber = (uint8_t) data[4];

    if (Logger::isDebug()) {
        Logger::debug(this, "torque limit: max positive: %.1fNm, min negative: %.1fNm", maxPositiveTorque / 10.0F, minNegativeTorque / 10.0F,
                limiterStateNumber);
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
    temperaturePowerStage = (int16_t) (data[1] | (data[0] << 8)) * 5;
    temperatureMotor = (int16_t) (data[3] | (data[2] << 8)) * 5;
    temperatureController = (int16_t) (data[4] - 50) * 10;
    if (Logger::isDebug()) {
        Logger::debug(this, "temperature: powerStage: %.1fC, motor: %.1fC, system: %.1fC", temperaturePowerStage / 10.0F, temperatureMotor / 10.0F,
                temperatureController / 10.0F);
    }
    if (temperaturePowerStage > temperatureController) {
        temperatureController = temperaturePowerStage;
    }
}

int32_t BrusaDMC5::getMechanicalPower()
{
    return mechanicalPower * 100;
}


/*
 * Return the device id of this device
 */
DeviceId BrusaDMC5::getId()
{
    return BRUSA_DMC5;
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
        prefsHandler->read(EEMC_DC_VOLT_LIMIT_MOTOR, &config->dcVoltLimitMotor);
        prefsHandler->read(EEMC_DC_VOLT_LIMIT_REGEN, &config->dcVoltLimitRegen);
        prefsHandler->read(EEMC_DC_CURRENT_LIMIT_MOTOR, &config->dcCurrentLimitMotor);
        prefsHandler->read(EEMC_DC_CURRENT_LIMIT_REGEN, &config->dcCurrentLimitRegen);
        prefsHandler->read(EEMC_OSCILLATION_LIMITER, &temp);
        config->enableOscillationLimiter = (temp != 0);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->dcVoltLimitMotor = 1000;
        config->dcVoltLimitRegen = 4200;
        config->dcCurrentLimitMotor = 3000;
        config->dcCurrentLimitRegen = 3000;
        config->enableOscillationLimiter = false;
        saveConfiguration();
    }
    Logger::info(this, "DC limit motor: %.1f Volt, DC limit regen: %.1f Volt", config->dcVoltLimitMotor / 10.0f, config->dcVoltLimitRegen / 10.0f);
    Logger::info(this, "DC limit motor: %.1f Amps, DC limit regen: %.1f Amps", config->dcCurrentLimitMotor / 10.0f,
            config->dcCurrentLimitRegen / 10.0f);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void BrusaDMC5::saveConfiguration()
{
    BrusaDMC5Configuration *config = (BrusaDMC5Configuration *) getConfiguration();

    MotorController::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_DC_VOLT_LIMIT_MOTOR, config->dcVoltLimitMotor);
    prefsHandler->write(EEMC_DC_VOLT_LIMIT_REGEN, config->dcVoltLimitRegen);
    prefsHandler->write(EEMC_DC_CURRENT_LIMIT_MOTOR, config->dcCurrentLimitMotor);
    prefsHandler->write(EEMC_DC_CURRENT_LIMIT_REGEN, config->dcCurrentLimitRegen);
    prefsHandler->write(EEMC_OSCILLATION_LIMITER, (uint8_t) (config->enableOscillationLimiter ? 1 : 0));
    prefsHandler->saveChecksum();
}
