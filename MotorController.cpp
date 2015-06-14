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

MotorController::MotorController() : Device()
{
    canHandlerEv = CanHandler::getInstanceEV();

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
    kiloWattHours = 0;
    milliStamp = 0;
    skipcounter = 0;
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
        mechanicalPower = dcVoltage * dcCurrent / 10000; //In kilowatts. DC voltage is x10
        if (dcVoltage > config->nominalVolt && torqueActual > 0) {
            kiloWattHours = 1; //If our voltage is higher than fully charged with no regen, zero our kwh meter
        }
        if (milliStamp > currentMillis) {
            milliStamp = 0; //In case millis rolls over to zero while running
        }
        kiloWattHours += (currentMillis - milliStamp) * mechanicalPower; //We assume here that power is at current level since last tick and accrue in kilowattmilliseconds.
        milliStamp = currentMillis; //reset our kwms timer for next check

        if (skipcounter++ > 30) {
            prefsHandler->write(EEMC_KILOWATTHRS, kiloWattHours);
            prefsHandler->saveChecksum();
            skipcounter = 0;
        }
    }
}

/*
 * The sub-classes must use this function to report activity/messages from the controller.
 * In subsequent calls to checkActivity() it is verified if we ran into a timeout.
 */
void MotorController::reportActivity() {
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
void MotorController::checkActivity() {
    if (ticksNoMessage < 255) { // make sure it doesn't overflow
        ticksNoMessage++;
    }
    // We haven't received frames from the controller for a defined number of ticks
    // But we're in system state "running", so we've lost communications.
    if (ticksNoMessage > CFG_MOTORCTRL_MAX_NUM_LOST_MSG && status->getSystemState() == Status::running) {
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
    DeviceManager *deviceManager = DeviceManager::getInstance();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();

    throttleLevel = 0; //force to zero in case not in operational condition
    torqueRequested = 0;
    speedRequested = 0;

    if (powerOn && ready) {
        if (accelerator) {
            throttleLevel = accelerator->getLevel();
        }
        // if the brake has been pressed it may override the accelerator
        if (brake && brake->getLevel() < 0 && brake->getLevel() < accelerator->getLevel()) {
            throttleLevel = brake->getLevel();
        }
        if (config->powerMode == modeSpeed) {
            speedRequested = throttleLevel * config->speedMax / 1000;
            torqueRequested = config->torqueMax;
        } else {
            // torque mode
            speedRequested = config->speedMax;
            torqueRequested = throttleLevel * config->torqueMax / 1000;
        }
    }
}

void MotorController::updateGear()
{
    if (powerOn && running) {
        gear = (systemIO->isReverseSignalPresent() ? REVERSE : DRIVE);
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
    systemIO->setEnableMotor(powerOn);
}

void MotorController::setup()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
    prefsHandler->read(EEMC_KILOWATTHRS, &kiloWattHours);  //retrieve kilowatt hours from EEPROM

    Device::setup();
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

uint32_t MotorController::getKiloWattHours()
{
    return kiloWattHours;
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
        prefsHandler->read(EEMC_RPM_SLEW_RATE, &config->speedSlewRate);
        prefsHandler->read(EEMC_TORQUE_SLEW_RATE, &config->torqueSlewRate);
        prefsHandler->read(EEMC_MAX_MECH_POWER_MOTOR, &config->maxMechanicalPowerMotor);
        prefsHandler->read(EEMC_MAX_MECH_POWER_REGEN, &config->maxMechanicalPowerRegen);
        prefsHandler->read(EEMC_REVERSE_LIMIT, &config->reversePercent);
        prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
        prefsHandler->read(EEMC_POWER_MODE, (uint8_t *) &config->powerMode);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->invertDirection = false;
        config->speedMax = MaxRPMValue;
        config->torqueMax = MaxTorqueValue;
        config->speedSlewRate = RPMSlewRateValue;
        config->torqueSlewRate = TorqueSlewRateValue;
        config->maxMechanicalPowerMotor = 2000;
        config->maxMechanicalPowerRegen = 400;
        config->reversePercent = ReversePercent;
        config->nominalVolt = NominalVolt;
        config->powerMode = modeTorque;
    }

    Logger::info(getId(), "Power mode: %s, Max torque: %i, Max RPM: %i", (config->powerMode == modeTorque ? "torque" : "speed"), config->torqueMax, config->speedMax);
    Logger::info(getId(), "Torque slew rate: %i, Speed slew rate: %i", config->torqueSlewRate, config->speedSlewRate);
    Logger::info(getId(), "Max mech power motor: %fkW, Max mech power regen: %fkW", config->maxMechanicalPowerMotor / 10.0f,
            config->maxMechanicalPowerRegen / 10.0f);
}

void MotorController::saveConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_INVERT_DIRECTION, (uint8_t)(config->invertDirection ? 1 : 0));
    prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
    prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
    prefsHandler->write(EEMC_RPM_SLEW_RATE, config->speedSlewRate);
    prefsHandler->write(EEMC_TORQUE_SLEW_RATE, config->torqueSlewRate);
    prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
    prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
    prefsHandler->write(EEMC_POWER_MODE, (uint8_t) config->powerMode);
    prefsHandler->saveChecksum();
}
