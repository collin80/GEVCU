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

    powerMode = modeTorque;
    throttleRequested = 0;
    speedRequested = 0;
    speedActual = 0;
    torqueRequested = 0;
    torqueActual = 0;
    torqueAvailable = 0;
    mechanicalPower = 0;

    selectedGear = NEUTRAL;

    dcVoltage = 0;
    dcCurrent = 0;
    acCurrent = 0;
    kiloWattHours = 0;
    nominalVolts = 0;
    milliStamp = 0;
    skipcounter = 0;
}

DeviceType MotorController::getType()
{
    return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick()
{
    uint8_t forwardSwitch, reverseSwitch;
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    //killowatts and kilowatt hours
    mechanicalPower = dcVoltage * dcCurrent / 10000; //In kilowatts. DC voltage is x10
    //Logger::console("POWER: %d", mechanicalPower);
    //Logger::console("dcVoltage: %d", dcVoltage);
    //Logger::console("nominalVolts: %d", nominalVolts);
    //Logger::console("kilowatthours: %d", kiloWattHours);


    if (dcVoltage > nominalVolts && torqueActual > 0) {
        kiloWattHours = 1;   //If our voltage is higher than fully charged with no regen, zero our kwh meter
    }

    if (milliStamp > millis()) {
        milliStamp = 0;   //In case millis rolls over to zero while running
    }

    uint32_t currentMillis = millis();
    kiloWattHours += (currentMillis - milliStamp) * mechanicalPower; //We assume here that power is at current level since last tick and accrue in kilowattmilliseconds.
    milliStamp = currentMillis; //reset our kwms timer for next check

    DeviceManager *deviceManager = DeviceManager::getInstance();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();

    if (status->getSystemState() == Status::running && accelerator) {
        throttleRequested = accelerator->getLevel();
        if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) { //if the brake has been pressed it overrides the accelerator.
            throttleRequested = brake->getLevel();
        }
    } else {
        throttleRequested = 0; //force to zero in case not in operational condition
    }

    // transfer temperature to status object (so cooling fan can use it and we have no circular references)
    status->temperatureController = temperatureController;
    status->temperatureMotor = temperatureMotor;

    //Logger::debug("Throttle: %d", throttleRequested);
    if (skipcounter++ > 30) { //As how fast we turn on cooling is very low priority, we only check cooling every 24th lap or about once per second
        prefsHandler->write(EEMC_KILOWATTHRS, kiloWattHours);
        prefsHandler->saveChecksum();
        skipcounter = 0;
    }
}


void MotorController::handleCanFrame(CAN_FRAME *frame)
{
}


void MotorController::setup()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();
    prefsHandler->read(EEMC_KILOWATTHRS, &kiloWattHours);  //retrieve kilowatt hours from EEPROM
    nominalVolts = config->nominalVolt;

    Device::setup();
}

MotorController::PowerMode MotorController::getPowerMode()
{
    return powerMode;
}

void MotorController::setPowerMode(PowerMode mode)
{
    powerMode = mode;
}

int16_t MotorController::getThrottle()
{
    return throttleRequested;
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

MotorController::Gears MotorController::getSelectedGear()
{
    return selectedGear;
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

int16_t MotorController::getnominalVolt()
{
    return nominalVolts;
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

uint32_t MotorController::getTickInterval()
{
    return CFG_TICK_INTERVAL_MOTOR_CONTROLLER;
}

void MotorController::loadConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        prefsHandler->read(EEMC_MAX_RPM, &config->speedMax);
        prefsHandler->read(EEMC_MAX_TORQUE, &config->torqueMax);
        prefsHandler->read(EEMC_RPM_SLEW_RATE, &config->speedSlewRate);
        prefsHandler->read(EEMC_TORQUE_SLEW_RATE, &config->torqueSlewRate);
        prefsHandler->read(EEMC_REVERSE_LIMIT, &config->reversePercent);
        prefsHandler->read(EEMC_KILOWATTHRS, &config->kilowattHrs);
        prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->speedMax = MaxRPMValue;
        config->torqueMax = MaxTorqueValue;
        config->speedSlewRate = RPMSlewRateValue;
        config->torqueSlewRate = TorqueSlewRateValue;
        config->reversePercent = ReversePercent;
        config->kilowattHrs = KilowattHrs;
        config->nominalVolt = NominalVolt;
    }

    Logger::info("MaxTorque: %i MaxRPM: %i", config->torqueMax, config->speedMax);
}

void MotorController::saveConfiguration()
{
    MotorControllerConfiguration *config = (MotorControllerConfiguration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
    prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
    prefsHandler->write(EEMC_RPM_SLEW_RATE, config->speedSlewRate);
    prefsHandler->write(EEMC_TORQUE_SLEW_RATE, config->torqueSlewRate);
    prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
    prefsHandler->write(EEMC_KILOWATTHRS, config->kilowattHrs);
    prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
    prefsHandler->saveChecksum();
}
