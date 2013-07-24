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
 
MotorController::MotorController(CanHandler *canbus, Throttle *accelerator, Throttle *brake) : Device(canbus) {
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
	this->accelerator = accelerator;
	this->brake = brake;
}

Device::DeviceType MotorController::getType() {
	return (Device::DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {
	uint8_t forwardSwitch, reverseSwitch;
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

	if (accelerator)
		requestedThrottle = accelerator->getThrottle();
	if (brake && brake->getThrottle() != 0) //if the brake has been pressed it overrides the accelerator.
		requestedThrottle = brake->getThrottle();

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
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		prefsHandler->read(EEMC_MAX_RPM, &maxRPM);
		prefsHandler->read(EEMC_MAX_TORQUE, &maxTorque);
	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		maxRPM = 5000;
		maxTorque = 500; //50Nm
		prefsHandler->write(EEMC_MAX_RPM, maxRPM);
		prefsHandler->write(EEMC_MAX_TORQUE, maxTorque);
		prefsHandler->saveChecksum();
	}
}
/*
 #define EEMC_ACTIVE_HIGH		24  //1 byte - bitfield - each bit corresponds to whether a given signal is active high (1) or low (0)
 // bit:		function:
 // 0		Drive enable
 // 1		Gear Select - Park/Neutral
 // 2		Gear Select - Forward
 // 3		Gear Select - Reverse
 #define EEMC_LIMP_SCALE			25 //1 byte - percentage of power to allow during limp mode
 #define EEMC_MAX_REGEN			26 //1 byte - percentage of max torque to apply to regen
 #define EEMC_REGEN_SCALE		28 //1 byte - percentage - reduces all regen related values (throttle, brake, maximum above)
 #define EEMC_PRECHARGE_RELAY	29 //1 byte - 0 = no precharge relay 1 = yes, there is one
 #define EEMC_CONTACTOR_RELAY	30 //1 byte - 0 = no contactor relay 1 = yes there is
 #define EEMC_COOLING			31 //1 byte - set point in C for starting up cooling relay
 #define EEMC_MIN_TEMP_MOTOR		32 //2 bytes - signed int - Smallest value on temp gauge (1% PWM output)
 #define EEMC_MAX_TEMP_MOTOR		34 //2 bytes - signed int - Highest value on temp gauge (99% PWM output)
 #define EEMC_MIN_TEMP_INV		36 //2 bytes - signed int - Smallest value on temp gauge (1% PWM output)
 #define EEMC_MAX_TEMP_INV		38 //2 bytes - signed int - Highest value on temp gauge (99% PWM output)
 */

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

uint16_t MotorController::getRequestedTorque() {
	return requestedTorque;
}
