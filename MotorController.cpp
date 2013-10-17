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
 
MotorController::MotorController() : Device() {
	ready = false;
	running = false;
	faulted = false;
	warning = false;

	temperatureMotor = 0;
	temperatureInverter = 0;
	temperatureSystem = 0;

	statusBitfield1 = 0;
	statusBitfield2 = 0;
	statusBitfield3 = 0;
	statusBitfield4 = 0;

	powerMode = modeTorque;
	throttleRequested = 0;
	speedRequested = 0;
	speedActual = 0;
	torqueRequested = 0;
	torqueActual = 0;
	torqueAvailable = 0;
	mechanicalPower = 0;

	gearSwitch = GS_FAULT;

	dcVoltage = 0;
	dcCurrent = 0;
	acCurrent = 0;

	prechargeTime = 0;
	prechargeSoFar = 0;
	donePrecharge = false;
}

DeviceType MotorController::getType() {
	return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {
	uint8_t forwardSwitch, reverseSwitch;

	gearSwitch = GS_FORWARD;

	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	if (accelerator)
		throttleRequested = accelerator->getLevel();
	if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) //if the brake has been pressed it overrides the accelerator.
		throttleRequested = brake->getLevel();

	if (prechargeTime == 0) donePrecharge = true;

	if (!donePrecharge) 
	{
		if (prechargeSoFar < prechargeTime) 
		{
			prechargeSoFar += (getTickInterval() / 1000);
		}
		else {
			MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
			Logger::info("Done with precharge.");
			setOutput(config->mainContactorRelay, true);
			setOutput(config->prechargeRelay, false);
			donePrecharge = true;
		}
	}

	//Logger::debug("Throttle: %d", throttleRequested);

}

void MotorController::setup() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	//first set up the appropriate digital pins. All are active low currently
	/*
	 pinMode(MOTORCTL_INPUT_DRIVE_EN, INPUT_PULLUP); //Drive Enable
	 pinMode(MOTORCTL_INPUT_FORWARD, INPUT_PULLUP); //Forward gear
	 pinMode(MOTORCTL_INPUT_REVERSE, INPUT_PULLUP); //Reverse Gear
	 pinMode(MOTORCTL_INPUT_LIMP, INPUT_PULLUP); //Limp mode
	 */

	Device::setup();

	if (config->prechargeC> 0 && config->prechargeRelay < NUM_OUTPUT) {
		//precharge time is 5RC which is (R*C / 1000) ms * 5 = RC/200 but ohms is in tenths so divide by another 10 = RC/2000
		prechargeTime = ((int)config->prechargeC * config->prechargeR) / 2000;
		Logger::info("RC precharge mode. C: %i  R: %i   Precharge time: %i ms", config->prechargeC, config->prechargeR, prechargeTime);
		setOutput(config->prechargeRelay, true); //start the precharge right now
		setOutput(config->mainContactorRelay, false); //just to be sure
	}
	else {
		Logger::info("Not precharging in RC mode");
	}
}

bool MotorController::isRunning() {
	return running;
}

bool MotorController::isFaulted() {
	return faulted;
}

bool MotorController::isWarning() {
	return warning;
}

MotorController::PowerMode MotorController::getPowerMode() {
	return powerMode;
}

void MotorController::setPowerMode(PowerMode mode) {
	powerMode = mode;
}

int16_t MotorController::getThrottle() {
	return throttleRequested;
}

int16_t MotorController::getSpeedRequested() {
	return speedRequested;
}

int16_t MotorController::getSpeedActual() {
	return speedActual;
}

int16_t MotorController::getTorqueRequested() {
	return torqueRequested;
}

int16_t MotorController::getTorqueActual() {
	return torqueActual;
}

MotorController::GearSwitch MotorController::getGearSwitch() {
	return gearSwitch;
}

int16_t MotorController::getTorqueAvailable() {
	return torqueAvailable;
}

uint16_t MotorController::getDcVoltage() {
	return dcVoltage;
}

int16_t MotorController::getDcCurrent() {
	return dcCurrent;
}

uint16_t MotorController::getAcCurrent() {
	return acCurrent;
}

int16_t MotorController::getMechanicalPower() {
	return mechanicalPower;
}

int16_t MotorController::getTemperatureMotor() {
	return temperatureMotor;
}

int16_t MotorController::getTemperatureInverter() {
	return temperatureInverter;
}

int16_t MotorController::getTemperatureSystem() {
	return temperatureSystem;
}

uint32_t MotorController::getStatusBitfield1() {
	return statusBitfield1;
}

uint32_t MotorController::getStatusBitfield2() {
	return statusBitfield2;
}

uint32_t MotorController::getStatusBitfield3() {
	return statusBitfield3;
}

uint32_t MotorController::getStatusBitfield4() {
	return statusBitfield4;
}

uint32_t MotorController::getTickInterval() {
	return 0;
}

bool MotorController::isReady() {
	return false;
}

void MotorController::loadConfiguration() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

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
		prefsHandler->read(EEMC_PRECHARGE_C, &config->prechargeC);
		prefsHandler->read(EEMC_PRECHARGE_R, &config->prechargeR);
		prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
		prefsHandler->read(EEMC_PRECHARGE_RELAY, &config->prechargeRelay);
		prefsHandler->read(EEMC_CONTACTOR_RELAY, &config->mainContactorRelay);
	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		config->speedMax = MaxRPMValue;
		config->torqueMax = MaxTorqueValue;
		config->speedSlewRate = RPMSlewRateValue;
		config->torqueSlewRate = TorqueSlewRateValue;
		config->reversePercent = ReversePercent;
		config->prechargeC = PrechargeC;
		config->prechargeR = PrechargeR;
		config->nominalVolt = NominalVolt;
		config->prechargeRelay = PrechargeRelay;
		config->mainContactorRelay = MainContactorRelay;
	}
	Logger::info("MaxTorque: %i MaxRPM: %i", config->torqueMax, config->speedMax);
}

void MotorController::saveConfiguration() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	Device::saveConfiguration(); // call parent

	prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
	prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
	prefsHandler->write(EEMC_RPM_SLEW_RATE, config->speedSlewRate);
	prefsHandler->write(EEMC_TORQUE_SLEW_RATE, config->torqueSlewRate);
	prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
	prefsHandler->write(EEMC_PRECHARGE_C, config->prechargeC);
	prefsHandler->write(EEMC_PRECHARGE_R, config->prechargeR);
	prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
	prefsHandler->write(EEMC_CONTACTOR_RELAY, config->mainContactorRelay);
	prefsHandler->write(EEMC_PRECHARGE_RELAY, config->prechargeRelay);
	prefsHandler->saveChecksum();
}
