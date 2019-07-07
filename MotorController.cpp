/*
 * MotorController.cpp
 *
 * Parent class for all motor controllers.
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

#include "MotorController.h"

MotorController::MotorController() :
        Device()
{
    temperatureMotor = 0;
    temperatureController = 0;

    powerOn = false;
    gear = GEAR_NEUTRAL;

    throttleLevel = 0;
    speedRequested = 0;
    speedActual = 0;
    torqueRequested = 0;
    torqueActual = 0;
    torqueAvailable = 0;

    dcVoltage = 0;
    dcCurrent = 0;
    acCurrent = 0;
    lastTick = 0;
    slewTimestamp = millis();
    gearChangeTimestamp = 0;
    rolling = false;
    ticksNoMessage = 0;
    brakeHoldActive = false;
    brakeHoldStart = 0;
    brakeHoldLevel = 0;
    minimumBatteryTemperature = 50; // 5 deg C

    cruisePid = NULL;
    cruiseSpeedActual = 0;
    cruiseSpeedTarget = 0;
    cruiseThrottle = 1000;
}

DeviceType MotorController::getType()
{
    return (DEVICE_MOTORCTRL);
}

/*
 * Activate/Deactivate status indicator at standstill/rolling
 */
void MotorController::updateStatusIndicator()
{
    if (running) {
        if (speedActual == 0) {
            if (rolling) {
                rolling = false;
                if (status.enableCreep) // a little hack to temporarely disable light
                    deviceManager.sendMessage(DEVICE_DISPLAY, STATUSINDICATOR, MSG_UPDATE, (void *) "on");
            }
        } else {
            if (speedActual > 1000 && !rolling) {
                rolling = true;
                deviceManager.sendMessage(DEVICE_DISPLAY, STATUSINDICATOR, MSG_UPDATE, (void *) "off");
            }
        }
    }
}

/*
 * The sub-classes must use this function to report activity/messages from the controller.
 * In subsequent calls to checkActivity() it is verified if we ran into a timeout.
 */
void MotorController::reportActivity()
{
    if (!running) //if we're newly running then cancel faults if necessary.
    {
//        faultHandler.cancelOngoingFault(getId(), FAULT_MOTORCTRL_COMM);
    }
    ticksNoMessage = 0;
}

/*
 * Verify if the controller sends messages, if it is online
 * Otherwise the controller's status flags "ready" and "running"
 * are set to false and a fault is raised
 */
void MotorController::checkActivity()
{
    if (ticksNoMessage < 255) { // make sure it doesn't overflow
        ticksNoMessage++;
    }
    // We haven't received frames from the controller for a defined number of ticks
    // But we're in system state "running", so we've lost communications.
    if (ticksNoMessage > CFG_MOTORCTRL_MAX_NUM_LOST_MSG && status.getSystemState() == Status::running) {
        running = false;
        ready = false;
//        faultHandler.raiseFault(getId(), FAULT_MOTORCTRL_COMM, true);
    }
}

int16_t MotorController::processBrakeHold(MotorControllerConfiguration *config, int16_t throttleLvl, int16_t brakeLvl)
{
    if (brakeHoldActive) {
        if (brakeHoldStart == 0) {
            if (brakeLvl == 0) { // engage brake hold once the brake is released
                brakeHoldStart = millis();
                brakeHoldLevel = 0;
                Logger::debug("brake hold engaged for %dms", CFG_BRAKE_HOLD_MAX_TIME);
            }
        } else {
            // deactivate after 5sec or when accelerator gives more torque or we're rolling forward without motor power
            if (brakeHoldStart + CFG_BRAKE_HOLD_MAX_TIME < millis() || throttleLvl > brakeHoldLevel || (speedActual > 0 && brakeHoldLevel == 0)) {
                brakeHoldActive = false;
                brakeHoldLevel = 0;
                brakeHoldStart = 0;
                throttleLvl = 0;
                slewTimestamp = millis(); // this should re-activate slew --> slowly reduce to 0 torque
                Logger::debug("brake hold deactivated");
            } else {
                uint16_t delta = abs(speedActual) * 2 / config->brakeHoldForceCoefficient + 1; // make sure it's always bigger than 0
                if (speedActual < 0 && brakeHoldLevel < config->brakeHold * 10) {
                    brakeHoldLevel += delta;
                }
                if (speedActual >= 0 && brakeHoldLevel > 0) {
                    brakeHoldLevel -= (delta * 2); // decrease faster to limit oscillation
                }

                brakeHoldLevel = constrain(brakeHoldLevel, 0, config->brakeHold * 10); // it might have overshot above
                throttleLvl = brakeHoldLevel;
            }
        }
    } else {
        if (brakeLvl < 0 && speedActual == 0) { // init brake hold at stand-still when brake is pressed
            brakeHoldActive = true;
            brakeHoldStart = 0;
            Logger::debug("brake hold activated");
        }
    }
    return throttleLvl;
}

/**
 * /brief In case ABS is active, apply no power to wheels to prevent loss of control through regen forces. If gear shift support is enabled, additionally
 * the motor will be spun up/down to the next
 */
void MotorController::processAbsOrGearChange(bool gearChangeSupport)
{
    if (systemIO.isABSActive()) {
        torqueRequested = 0;
        speedRequested = 0;
        Logger::info(this, "ABS active");

        if (gearChangeSupport) {
            if (gearChangeTimestamp == 0) {
                Logger::info(this, "Start gear change cycle");
                gearChangeTimestamp = millis();
                return;
            }

            uint32_t duration = millis() - gearChangeTimestamp;
            // break/accel  with about 20Nm after 500ms for 500ms
            if (duration > 500 && duration < 1000) {
                Logger::info(this, "Adjusting motor speed");
                speedRequested = 2500;
                torqueRequested = 200; // TODO positive and negative?
                return;
            }
        }
    } else {
        if (gearChangeTimestamp) {
            Logger::info(this, "Gear change cycle finished");
            gearChangeTimestamp = 0;
        }
    }

}

/**
 * /brief Check if the battery temperatures are below the minimum temperature (configured in charger) in which case no regen should be applied.
 *
 * @return true if it's ok to apply regen, false if no regen must be applied
 */
bool MotorController::checkBatteryTemperatureForRegen()
{
    int16_t lowestBatteryTemperature = status.getLowestBatteryTemperature();

    if (lowestBatteryTemperature != CFG_NO_TEMPERATURE_DATA && (lowestBatteryTemperature * 10) < minimumBatteryTemperature) {
        status.enableRegen = false;
        Logger::info(this, "No regenerative braking due to low battery temperature! (%f < %f)", lowestBatteryTemperature / 10.0f,
                minimumBatteryTemperature / 10.0f);
        return false;
    }

    return true;
}

/*
 * From the throttle and brake level, calculate the requested torque and speed.
 * Depending on power mode, the throttle dependent value is torque or speed. The other
 * value is locked to the configured maximum value.
 */
void MotorController::processThrottleLevel()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
    Throttle *accelerator = deviceManager.getAccelerator();
    Throttle *brake = deviceManager.getBrake();

    throttleLevel = 0; //force to zero in case not in operational condition or no throttle is enabled
    speedRequested = 0;
    if (powerOn && ready) {
        if (accelerator && !accelerator->isFaulted()) {
            throttleLevel = accelerator->getLevel();
        }

        if (cruisePid != NULL/* && (cruiseThrottle - 1000) > throttleLevel*/) {
            throttleLevel = round(cruiseThrottle) - 1000;
        }

        if (brake && !brake->isFaulted() && brake->getLevel() < 0) { // if the brake has been pressed it overrides the accelerator
            throttleLevel = brake->getLevel();
            disableCruiseControl();
        }
        if (brake && config->brakeHold > 0) { // check if brake hold should be applied
            throttleLevel = processBrakeHold(config, throttleLevel, brake->getLevel());
        }

        if (throttleLevel < 0 && (!status.enableRegen || !checkBatteryTemperatureForRegen())) { // do not apply regen if the batteries are too cold
            throttleLevel = 0;
        }

        if (config->powerMode == modeSpeed) {
            int32_t speedTarget = throttleLevel * config->speedMax / 1000;
            torqueRequested = config->torqueMax;
        } else {  // torque mode
            speedRequested = config->speedMax;
            int32_t torqueTarget = throttleLevel * config->torqueMax / 1000;

            if (config->slewRate == 0 || brakeHoldActive) {
                torqueRequested = torqueTarget;
            } else { // calc slew part and add/subtract from torqueRequested
                uint32_t currentTimestamp = millis();
                uint16_t slewPart = abs(torqueTarget - torqueRequested) * config->slewRate / 1000 * (currentTimestamp - slewTimestamp) / 1000;

                // if we're we're reversing torque, reduce the slew part in the 0 area to make transitions between positive and negative torque smoother
                if ((torqueActual * torqueRequested < 0) && abs(torqueRequested) < 150) {
                    slewPart /= 10; //TODO make configurable
                }

                if (slewPart == 0 && torqueRequested != torqueTarget) {
                    slewPart = 1;
                }

                if (torqueTarget < torqueRequested) {
                    torqueRequested = max(torqueRequested - slewPart, torqueTarget);
                } else {
                    torqueRequested = min(torqueRequested + slewPart, torqueTarget);
                }
                slewTimestamp = currentTimestamp;
            }
        }
    } else {
        torqueRequested = 0;
    }

    processAbsOrGearChange(config->gearChangeSupport);
}

void MotorController::updateGear()
{
    if (powerOn && running) {
        gear = (systemIO.isReverseSignalPresent() ? GEAR_REVERSE : GEAR_DRIVE);
    } else {
        gear = GEAR_NEUTRAL; // stay in neutral until the controller reports that it's running
    }
}

void MotorController::cruiseControlToggle() {
    if (cruisePid == NULL) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

        cruiseSpeedTarget = speedActual;
        cruiseSpeedActual = speedActual;
        cruiseThrottle = throttleLevel + 1000.0f; // because PID can't handle negative numbers, cruiseThrottle is offset by +1000 (0-2000)
        status.cruiseSpeed = speedActual;

        Logger::info(this, "Enabling cruise control with speed %f", cruiseSpeedTarget);

        cruisePid = new PID(&cruiseSpeedActual, &cruiseThrottle, &cruiseSpeedTarget, config->cruiseKp, config->cruiseKi, config->cruiseKd, DIRECT);
        cruisePid->SetOutputLimits(0, 2000);

        uint32_t interval = tickHandler.getInterval(this);
        if (interval != 0) {
            cruisePid->SetSampleTime(interval / 1000);
        }
        cruisePid->SetMode(AUTOMATIC);
    } else {
        disableCruiseControl();
    }
}

void MotorController::cruiseControlAdjust(int16_t speedDelta) {
    if (cruisePid == NULL) {
        cruiseControlToggle();
    }
    cruiseSpeedTarget += speedDelta;
    Logger::info(this, "Changing cruise control speed to %d", cruiseSpeedTarget);
}

void MotorController::cruiseControlSetSpeed(int16_t speedTarget) {
    if (cruisePid == NULL) {
        cruiseControlToggle();
    }
    cruiseSpeedTarget = speedTarget;
    Logger::info(this, "Setting cruise control speed to %f", cruiseSpeedTarget);
}

void MotorController::disableCruiseControl() {
    if (cruisePid == NULL)
        return;

    Logger::info(this, "Cruise control OFF.");
    cruisePid = NULL;
    cruiseSpeedActual = 0;
    cruiseSpeedTarget = 0;
    cruiseThrottle = 1000;
}

void MotorController::handleTick()
{
    checkActivity();

    if (cruisePid != NULL) {
        cruiseSpeedActual = speedActual;
        cruisePid->Compute(); // updates torque
    }

    processThrottleLevel();
    updateGear();

    updateStatusIndicator();

    if (Logger::isDebug()) {
        Logger::debug(this, "throttle: %f%%, requested Speed: %ld rpm, requested Torque: %f Nm, gear: %d", throttleLevel / 10.0f, speedRequested,
                torqueRequested / 10.0F, gear);
    }
}

void MotorController::handleCanFrame(CAN_FRAME *frame)
{
}

/**
 * act on messages the super-class does not react upon, like state change
 * to ready or running which should enable/disable the power-stage of the controller
 */
void MotorController::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);

    powerOn = (newState == Status::running ? true : false);

    if (!powerOn) {
        throttleLevel = 0;
        gear = GEAR_NEUTRAL;
    }

    systemIO.setEnableMotor(newState == Status::ready || newState == Status::running);

    if (newState == Status::ready) { // at this time also the charger config should be loaded
        // get the minimum temperature from the charger
        Charger *charger = deviceManager.getCharger();
        if (charger) {
            ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();
            if (config) {
                minimumBatteryTemperature = config->minimumTemperature;
            }
        }
    }
}

void MotorController::setup()
{
    Device::setup();

    rolling = false;
    slewTimestamp = millis();


//TODO move to saveable config
MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
config->cruiseKp = 1.0f;
config->cruiseKi = .20f;
config->cruiseKd = .10f;
}

/**
 * Tear down the controller in a safe way.
 */
void MotorController::tearDown()
{
    Device::tearDown();

    throttleLevel = 0;
    gear = GEAR_NEUTRAL;
}

int16_t MotorController::getThrottleLevel()
{
    return throttleLevel;
}

int16_t MotorController::getSpeedRequested()
{
    return speedRequested;
}

int16_t MotorController::getSpeedActual()
{
    return speedActual;
}

int16_t MotorController::getTorqueRequested()
{
    return torqueRequested;
}

int16_t MotorController::getTorqueActual()
{
    return torqueActual;
}

MotorController::Gears MotorController::getGear()
{
    return gear;
}

int16_t MotorController::getTorqueAvailable()
{
    return torqueAvailable;
}

uint16_t MotorController::getDcVoltage()
{
    return dcVoltage;
}

int16_t MotorController::getDcCurrent()
{
    return dcCurrent;
}

uint16_t MotorController::getAcCurrent()
{
    return acCurrent;
}

/**
 * Return mechanical power in Watts
 */
int32_t MotorController::getMechanicalPower()
{
    return dcVoltage * dcCurrent / 100;
}

int16_t MotorController::getTemperatureMotor()
{
    return temperatureMotor;
}

int16_t MotorController::getTemperatureController()
{
    return temperatureController;
}

void MotorController::loadConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::loadConfiguration(); // call parent
    Logger::info(this, "Motor controller configuration:");

#ifdef USE_HARD_CODED

    if (false) {
#else

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        prefsHandler->read(EEMC_INVERT_DIRECTION, &temp);
        config->invertDirection = temp;
        prefsHandler->read(EEMC_MAX_RPM, &config->speedMax);
        prefsHandler->read(EEMC_MAX_TORQUE, &config->torqueMax);
        prefsHandler->read(EEMC_SLEW_RATE, &config->slewRate);
        prefsHandler->read(EEMC_MAX_MECH_POWER_MOTOR, &config->maxMechanicalPowerMotor);
        prefsHandler->read(EEMC_MAX_MECH_POWER_REGEN, &config->maxMechanicalPowerRegen);
        prefsHandler->read(EEMC_REVERSE_LIMIT, &config->reversePercent);
        prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
        prefsHandler->read(EEMC_POWER_MODE, (uint8_t *) &config->powerMode);
        prefsHandler->read(EEMC_CREEP_LEVEL, &config->creepLevel);
        prefsHandler->read(EEMC_CREEP_SPEED, &config->creepSpeed);
        prefsHandler->read(EEMC_BRAKE_HOLD, &config->brakeHold);
        prefsHandler->read(EEMC_BRAKE_HOLD_COEFF, &config->brakeHoldForceCoefficient);
        prefsHandler->read(EEMC_GEAR_CHANGE_SUPPORT, &temp);
        config->gearChangeSupport = temp;
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->invertDirection = false;
        config->speedMax = 6000;
        config->torqueMax = 3000;
        config->slewRate = 0;
        config->maxMechanicalPowerMotor = 2000;
        config->maxMechanicalPowerRegen = 400;
        config->reversePercent = 50;
        config->nominalVolt = 3300;
        config->powerMode = modeTorque;
        config->creepLevel = 0;
        config->creepSpeed = 0;
        config->brakeHold = 0;
        config->gearChangeSupport = false;
    }

    Logger::info(this, "Power mode: %s, Max torque: %i", (config->powerMode == modeTorque ? "torque" : "speed"), config->torqueMax);
    Logger::info(this, "Max RPM: %i, Slew rate: %i", config->speedMax, config->slewRate);
    Logger::info(this, "Max mech power motor: %fkW, Max mech power regen: %fkW", config->maxMechanicalPowerMotor / 10.0f,
            config->maxMechanicalPowerRegen / 10.0f);
    Logger::info(this, "Creep level: %i, Creep speed: %i, Brake Hold: %d, Gear Change Support: %d", config->creepLevel, config->creepSpeed,
            config->brakeHold, config->gearChangeSupport);
}

void MotorController::saveConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_INVERT_DIRECTION, (uint8_t) (config->invertDirection ? 1 : 0));
    prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
    prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
    prefsHandler->write(EEMC_SLEW_RATE, config->slewRate);
    prefsHandler->write(EEMC_MAX_MECH_POWER_MOTOR, config->maxMechanicalPowerMotor);
    prefsHandler->write(EEMC_MAX_MECH_POWER_REGEN, config->maxMechanicalPowerRegen);
    prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
    prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
    prefsHandler->write(EEMC_POWER_MODE, (uint8_t) config->powerMode);
    prefsHandler->write(EEMC_CREEP_LEVEL, config->creepLevel);
    prefsHandler->write(EEMC_CREEP_SPEED, config->creepSpeed);
    prefsHandler->write(EEMC_BRAKE_HOLD, config->brakeHold);
    prefsHandler->write(EEMC_BRAKE_HOLD_COEFF, config->brakeHoldForceCoefficient);
    prefsHandler->write(EEMC_GEAR_CHANGE_SUPPORT, (uint8_t) (config->gearChangeSupport ? 1 : 0));
    prefsHandler->saveChecksum();
}
