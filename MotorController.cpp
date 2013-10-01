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
	prefsHandler = new PrefHandler(EE_MOTORCTL_START);
	faulted = false;
	running = false;
	motorTemp = 0;
	inverterTemp = 0;
	requestedRPM = 0;
	requestedThrottle = 0;
	requestedTorque = 0;
	actualTorque = 0;
	actualRPM = 0;
	maxTorque = 0;
	maxRPM = 0;
	gearSwitch = GS_FAULT;
	prechargeC = 0;
	prechargeR = 0;
	prechargeTime = 0;
	prechargeSoFar = 0;
	prechargeRelay = 255;
	mainContactorRelay = 255;
	donePrecharge = false;
}

DeviceType MotorController::getType() {
	return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {
	uint8_t forwardSwitch, reverseSwitch;

	//I'm pretty sure this whole block of code is worthless.
	//The inputs that this code uses aren't even actual, valid inputs to the GEVCU box.
	/*
	if (digitalRead(MOTORCTL_INPUT_DRIVE_EN) == LOW)
		running = true;
	else
		running = false;

	forwardSwitch = digitalRead(MOTORCTL_INPUT_FORWARD);
	reverseSwitch = digitalRead(MOTORCTL_INPUT_REVERSE);

	gearSwitch = GS_FAULT;
	if (forwardSwitch == LOW && reverseSwitch == HIGH)
		gearSwitch = GS_FORWARD;
	if (forwardSwitch == HIGH && reverseSwitch == LOW)
		gearSwitch = GS_REVERSE;
	*/

	running = true;
	gearSwitch = GS_FORWARD;

	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	if (accelerator)
		requestedThrottle = accelerator->getLevel();
	if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) //if the brake has been pressed it overrides the accelerator.
		requestedThrottle = brake->getLevel();

	if (prechargeTime == 0) donePrecharge = true;

	if (!donePrecharge) 
	{
		if (prechargeSoFar < prechargeTime) 
		{
			prechargeSoFar += (getTickInterval() / 1000);
		}
		else {
			Logger::info("Done with precharge.");
			setOutput(mainContactorRelay, true);
			setOutput(prechargeRelay, false);
			donePrecharge = true;
		}
	}

	//Logger::debug("Throttle: %d", requestedThrottle);

}

void MotorController::setup() {
	//this is where common parameters for motor controllers should be loaded from EEPROM

	//first set up the appropriate digital pins. All are active low currently
	/*
	 pinMode(MOTORCTL_INPUT_DRIVE_EN, INPUT_PULLUP); //Drive Enable
	 pinMode(MOTORCTL_INPUT_FORWARD, INPUT_PULLUP); //Forward gear
	 pinMode(MOTORCTL_INPUT_REVERSE, INPUT_PULLUP); //Reverse Gear
	 pinMode(MOTORCTL_INPUT_LIMP, INPUT_PULLUP); //Limp mode
	 */
#ifndef USE_HARD_CODED
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		prefsHandler->read(EEMC_MAX_RPM, &maxRPM);
		prefsHandler->read(EEMC_MAX_TORQUE, &maxTorque);
		prefsHandler->read(EEMC_PRECHARGE_C, &prechargeC);
		prefsHandler->read(EEMC_PRECHARGE_R, &prechargeR);
		prefsHandler->read(EEMC_NOMINAL_V, &nominalVolt);
		prefsHandler->read(EEMC_PRECHARGE_RELAY, &prechargeRelay);
		prefsHandler->read(EEMC_CONTACTOR_RELAY, &mainContactorRelay);
		prefsHandler->read(EEMC_REVERSE_LIMIT, &reversePercent);
	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		maxRPM = MaxRPMValue;
		maxTorque = MaxTorqueValue;
		prechargeC = PrechargeC;
		prechargeR = PrechargeR;
		nominalVolt = NominalVolt;
		prechargeRelay = PrechargeRelay;
		mainContactorRelay = MainContactorRelay;
		reversePercent = ReversePercent;
		saveEEPROM();
	}

#else
	maxRPM = MaxRPMValue;
	maxTorque = MaxTorqueValue;
	prechargeC = PrechargeC;
	prechargeR = PrechargeR;
	nominalVolt = NominalVolt;
	prechargeRelay = PrechargeRelay;
	mainContactorRelay = MainContactorRelay;
#endif

	Logger::info("MaxTorque: %i MaxRPM: %i", maxTorque, maxRPM);
	if (prechargeC> 0 && prechargeRelay < NUM_OUTPUT) {
		//precharge time is 5RC which is (R*C / 1000) ms * 5 = RC/200 but ohms is in tenths so divide by another 10 = RC/2000
		prechargeTime = ((int)prechargeC * prechargeR) / 2000;
		Logger::info("RC precharge mode. C: %i  R: %i   Precharge time: %i ms", prechargeC, prechargeR, prechargeTime);
		setOutput(prechargeRelay, true); //start the precharge right now
		setOutput(mainContactorRelay, false); //just to be sure
	}
	else {
		Logger::info("Not precharging in RC mode");
	}
}

int MotorController::getThrottle() {
	return (requestedThrottle);
}

bool MotorController::isRunning() {
	return (running);
}

bool MotorController::isFaulted() {
	return (faulted);
}

uint16_t MotorController::getActualRpm() {
	return actualRPM;
}

uint16_t MotorController::getActualTorque() {
	return actualTorque;
}

MotorController::GearSwitch MotorController::getGearSwitch() {
	return gearSwitch;
}

signed int MotorController::getInverterTemp() {
	return inverterTemp;
}

uint16_t MotorController::getMaxRpm() {
	return maxRPM;
}

uint16_t MotorController::getMaxTorque() {
	return maxTorque;
}

signed int MotorController::getMotorTemp() {
	return motorTemp;
}

uint16_t MotorController::getRequestedRpm() {
	return requestedRPM;
}
void MotorController::setMaxRpm(uint16_t maxRPM) 
{
	this->maxRPM = maxRPM;
}

uint16_t MotorController::getRequestedTorque() {
	return requestedTorque;
}
void MotorController::setMaxTorque(uint16_t maxTorque) 
{
	this->maxTorque = maxTorque;
}

uint16_t MotorController::getPrechargeC()
{
	return prechargeC;
}
void MotorController::setPrechargeC(uint16_t c)
{
	prechargeC = c;
}

uint16_t MotorController::getPrechargeR()
{
	return prechargeR;
}
void MotorController::setPrechargeR(uint16_t r) 
{
	prechargeR = r;
}

uint16_t MotorController::getNominalV()
{
	return nominalVolt;
}
void MotorController::setNominalV(uint16_t v) 
{
	nominalVolt = v;
}

uint8_t MotorController::getPrechargeRelay()
{
	return prechargeRelay;
}
void MotorController::setPrechargeRelay(uint8_t relay) 
{
	prechargeRelay = relay;
}

uint8_t MotorController::getMainRelay()
{
	return mainContactorRelay;
}
void MotorController::setMainRelay(uint8_t relay) 
{
	mainContactorRelay = relay;
}

uint8_t MotorController::getReversePercent()
{
	return reversePercent;
}
void MotorController::setReversePercent(uint8_t perc) 
{
	reversePercent = perc;
}

void MotorController::saveEEPROM()
{
	prefsHandler->write(EEMC_MAX_RPM, maxRPM);
	prefsHandler->write(EEMC_MAX_TORQUE, maxTorque);
	prefsHandler->write(EEMC_PRECHARGE_C, prechargeC);
	prefsHandler->write(EEMC_PRECHARGE_R, prechargeR);
	prefsHandler->write(EEMC_NOMINAL_V, nominalVolt);
	prefsHandler->write(EEMC_CONTACTOR_RELAY, mainContactorRelay);
	prefsHandler->write(EEMC_PRECHARGE_RELAY, prechargeRelay);
	prefsHandler->write(EEMC_REVERSE_LIMIT, reversePercent);
	prefsHandler->saveChecksum();
}

