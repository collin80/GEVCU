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
    gear = NEUTRAL;

    throttleLevel = 0;
    speedRequested = 0;
    speedActual = 0;
    torqueRequested = 0;
    torqueActual = 0;
    torqueAvailable = 0;
    mechanicalPower = 0;

    dcVoltage = 0;
    dcCurrent = 0;
    acCurrent = 0;
    powerConsumption = 0;
    milliStamp = 0;
    savePowerConsumption = false;
    ticksNoMessage = 0;
}

DeviceType MotorController::getType()
{
    return (DEVICE_MOTORCTRL);
}

/*
 * Calculate mechanical power and add power consumption to kWh counter.
 */
void MotorController::updatePowerConsumption()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
    uint32_t currentMillis = millis();

    if (running) {
        mechanicalPower = dcVoltage * dcCurrent / 10000; // in 0.1 kilowatts.
        if (dcVoltage > config->nominalVolt && torqueActual > 0) {
            powerConsumption = 0; //If our voltage is higher than fully charged with no regen, zero our meter
            savePowerConsumption = true;
        }
        // we're actually calculating kilowatt-milliseconds which is the same as watt seconds
        powerConsumption += (currentMillis - milliStamp) * mechanicalPower / 10;
        milliStamp = currentMillis; //reset our timer for next check

        if (speedActual == 0) { // save at stand-still
            if (savePowerConsumption) {
                prefsHandler->write(EEMC_ENEGRY_CONSUMPTION, powerConsumption);
                prefsHandler->saveChecksum();
            }
            savePowerConsumption = false;
        } else {
            savePowerConsumption = true;
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
    boolean disableSlew = false;

    throttleLevel = 0; //force to zero in case not in operational condition
    if (powerOn && ready) {
        if (accelerator) {
            throttleLevel = accelerator->getLevel();
        }
        // if the brake has been pressed it may override the accelerator
        if (brake && brake->getLevel() < 0 && brake->getLevel() < accelerator->getLevel()) {
            throttleLevel = brake->getLevel();
            disableSlew = true;
        }
        if (config->powerMode == modeSpeed) {
            int16_t speedTarget = throttleLevel * config->speedMax / 1000;
            torqueRequested = config->torqueMax;
        } else {
            // torque mode
            speedRequested = config->speedMax;
            int16_t torqueTarget = throttleLevel * config->torqueMax / 1000;

            if (config->slewRate == 0 || disableSlew) {
                torqueRequested = torqueTarget;
            } else {
                uint32_t currentTimestamp = millis();

                // no torque or requested and target have different sign -> request 0 power
                if (torqueTarget == 0 || (torqueTarget < 0 && torqueRequested > 0) || (torqueTarget > 0 && torqueRequested < 0)) {
                    torqueRequested = 0;
                } else if (abs(torqueTarget) <= abs(torqueRequested)) { // target closer to 0 then last time -> no slew, reduce power immediately
                    torqueRequested = torqueTarget;
                } else { // increase power -> apply slew
                    uint16_t slewPart = 0;
                    slewPart = config->torqueMax * config->slewRate / 1000 * (currentTimestamp - slewTimestamp) / 1000;
                    if (torqueTarget < 0) {
                        torqueRequested -= slewPart;
                        if (torqueRequested < torqueTarget) {
                            torqueRequested = torqueTarget;
                        }
                    } else {
                        torqueRequested += slewPart;
                        if (torqueRequested > torqueTarget) {
                            torqueRequested = torqueTarget;
                        }
                    }
                }
//Logger::info("torque target: %l, requested: %l", torqueTarget, torqueRequested);
                slewTimestamp = currentTimestamp;
            }
        }
    } else {
        torqueRequested = 0;
        speedRequested = 0;
    }
}

void MotorController::updateGear()
{
    if (powerOn && running) {
        gear = (systemIO.isReverseSignalPresent() ? REVERSE : DRIVE);
    } else {
        gear = NEUTRAL; // stay in neutral until the controller reports that it's running
    }
}

void MotorController::handleTick()
{
    checkActivity();

    processThrottleLevel();
    updateGear();

    updatePowerConsumption();

    if (Logger::isDebug()) {
        Logger::debug(getId(), "throttle: %f%%, requested Speed: %l rpm, requested Torque: %f Nm, gear: %d", throttleLevel / 10.0f, speedRequested,
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
        gear = NEUTRAL;
    }
    systemIO.setEnableMotor(newState == Status::ready || newState == Status::running);
}

void MotorController::setup()
{
    Device::setup();

    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
    prefsHandler->read(EEMC_ENEGRY_CONSUMPTION, &powerConsumption);  //retrieve power consumption hours from EEPROM
    savePowerConsumption = false;

    slewTimestamp = millis();
}

/**
 * Tear down the controller in a safe way.
 */
void MotorController::tearDown()
{
    Device::tearDown();

    throttleLevel = 0;
    gear = NEUTRAL;
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

uint32_t MotorController::getEnergyConsumption()
{
    return powerConsumption / 360000;
}

int16_t MotorController::getMechanicalPower()
{
    return mechanicalPower;
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
    Logger::info(getId(), "Motor controller configuration:");

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
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->invertDirection = false;
        config->speedMax = MaxRPMValue;
        config->torqueMax = MaxTorqueValue;
        config->slewRate = SlewRateValue;
        config->maxMechanicalPowerMotor = 2000;
        config->maxMechanicalPowerRegen = 400;
        config->reversePercent = ReversePercent;
        config->nominalVolt = NominalVolt;
        config->powerMode = modeTorque;
    }

    Logger::info(getId(), "Power mode: %s, Max torque: %i", (config->powerMode == modeTorque ? "torque" : "speed"), config->torqueMax);
    Logger::info(getId(), "Max RPM: %i, Slew rate: %i", config->speedMax, config->slewRate);
    Logger::info(getId(), "Max mech power motor: %fkW, Max mech power regen: %fkW", config->maxMechanicalPowerMotor / 10.0f,
            config->maxMechanicalPowerRegen / 10.0f);
}

void MotorController::saveConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_INVERT_DIRECTION, (uint8_t) (config->invertDirection ? 1 : 0));
    prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
    prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
    prefsHandler->write(EEMC_SLEW_RATE, config->slewRate);
    prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
    prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
    prefsHandler->write(EEMC_POWER_MODE, (uint8_t) config->powerMode);
    prefsHandler->saveChecksum();
}
