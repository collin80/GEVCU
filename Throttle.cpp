/*
 * Throttle.cpp
 *
 * Parent class for all throttle controllers

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

#include "Throttle.h"

/*
 * Constructor
 */
Throttle::Throttle() : Device() {
	level = 0;
	status = OK;
}

/*
 * Controls the main flow of throttle data acquisiton, validation and mapping to
 * user defined behaviour.
 *
 * Get's called by the sub-class which is triggered by the tick handler
 */
void Throttle::handleTick() {
	Device::handleTick();

	RawSignalData *rawSignals = acquireRawSignal(); // get raw data from the throttle device
	if (validateSignal(rawSignals)) { // validate the raw data
		uint16_t position = calculatePedalPosition(rawSignals); // bring the raw data into a range of 0-1000 (without mapping)
		level = mapPedalPosition(position); // apply mapping of the 0-1000 range to the user defined settings
	} else
		level = 0;
}

/*
 * Maps the input throttle position (0-1000 permille) to an output level which is
 * calculated based on the throttle mapping parameters (free float, regen, acceleration,
 * 50% acceleration).
 * The output value will be in the range of -1000 to 1000. The value will be used by the
 * MotorController class to calculate commanded torque or speed. Positive numbers result in
 * acceleration, negative numbers in regeneration. 0 will result in no force applied by
 * the motor.
 *
 * Configuration parameters:
 * positionRegenMaximum: The pedal position (0-1000) where maximumRegen will be applied. If not 0, then
 *                       moving the pedal from positionRegenMaximum to 0 will result in linear reduction
 *                       of regen from maximumRegen.
 * positionRegenMinimum: The pedal position (0-1000) where minimumRegen will be applied. If not 0, then
 *                       a linear regen force will be applied when moving the pedal from positionRegenMinimum
 *                       to positionRegenMaximum.
 * positionForwardMotionStart: The pedal position where the car starts to accelerate. If not equal to
 *                       positionRegenMinimum, then the gap between the two positions will result in no force
 *                       applied (free coasting).
 * positionHalfPower:    Position of the pedal where 50% of the maximum torque will be applied. To gain more
 *                       fine control in the lower speed range (e.g. when parking) it might make sense to
 *                       set this position higher than the mid point of positionForwardMotionStart and full
 *                       throttle.
 *
 * Important pre-condition (to be checked when changing parameters) :
 * 0 <= positionRegenMaximum <= positionRegenMinimum <= positionForwardMotionStart <= positionHalfPower
 */
int16_t Throttle::mapPedalPosition(int16_t pedalPosition) {
	int16_t throttleLevel, range, value;
	ThrottleConfiguration *config = (ThrottleConfiguration *) getConfiguration();

	throttleLevel = 0;

	if (pedalPosition == 0 && config->creep > 0) {
		throttleLevel = 10 * config->creep;
	} else if (pedalPosition <= config->positionRegenMinimum) {
		if (pedalPosition >= config->positionRegenMaximum) {
			range = config->positionRegenMinimum - config->positionRegenMaximum;
			value = pedalPosition - config->positionRegenMaximum;
			if (range != 0) // prevent div by zero, should result in 0 throttle if min==max
				throttleLevel = -10 * config->minimumRegen + (config->maximumRegen - config->minimumRegen) * (100 - value * 100 / range) / -10;
		} else {
			// no ramping yet below positionRegenMaximum, just drop to 0
//			range = config->positionRegenMaximum;
//			value = pedalPosition;
//			throttleLevel = -10 * config->maximumRegen * value / range;
		}
	}

	if (pedalPosition >= config->positionForwardMotionStart) {
		if (pedalPosition <= config->positionHalfPower) {
			range = config->positionHalfPower - config->positionForwardMotionStart;
			value = pedalPosition - config->positionForwardMotionStart;
			if (range != 0) // prevent div by zero, should result in 0 throttle if half==startFwd
				throttleLevel = 500 * value / range;
		} else {
			range = 1000 - config->positionHalfPower;
			value = pedalPosition - config->positionHalfPower;
			throttleLevel = 500 + 500 * value / range;
		}
	}
	//Logger::debug("throttle level: %d", throttleLevel);

	//A bit of a kludge. Normally it isn't really possible to ever get to
	//100% output. This next line just fudges the numbers a bit to make it
	//more likely to get that last bit of power
	if (throttleLevel > 979) throttleLevel = 1000;

	return throttleLevel;
}

/*
 * Make sure input level stays within margins (min/max) then map the constrained
 * level linearly to a value from 0 to 1000.
 */
uint16_t Throttle::normalizeAndConstrainInput(int32_t input, int32_t min, int32_t max) {
	return constrain(normalizeInput(input, min, max), (int32_t) 0, (int32_t) 1000);
}

/*
 * Map the constrained level linearly to a signed value from 0 to 1000.
 */
int32_t Throttle::normalizeInput(int32_t input, int32_t min, int32_t max) {
	return map(input, min, max, (int32_t) 0, (int32_t) 1000);
}

/*
 * Returns the currently calculated/mapped throttle level (from -1000 to 1000).
 */
int16_t Throttle::getLevel() {
	return level;
}

/*
 * Return the throttle's current status
 */
Throttle::ThrottleStatus Throttle::getStatus() {
	return status;
}

/*
 * Is the throttle faulted?
 */
bool Throttle::isFaulted() {
	return status != OK;
}

/*
 * Return the device type
 */
DeviceType Throttle::getType() {
	return DEVICE_THROTTLE;
}

RawSignalData* Throttle::acquireRawSignal() {
	return NULL;
}

bool Throttle::validateSignal(RawSignalData*) {
	return false;
}

uint16_t Throttle::calculatePedalPosition(RawSignalData*) {
	return 0;
}

/*
 * Load the config parameters which are required by all throttles
 */
void Throttle::loadConfiguration() {
	ThrottleConfiguration *config = (ThrottleConfiguration *) getConfiguration();

	Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED
	if (false) {
#else
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
		prefsHandler->read(EETH_REGEN_MIN, &config->positionRegenMinimum);
		prefsHandler->read(EETH_REGEN_MAX, &config->positionRegenMaximum);
		prefsHandler->read(EETH_FWD, &config->positionForwardMotionStart);
		prefsHandler->read(EETH_MAP, &config->positionHalfPower);
		prefsHandler->read(EETH_CREEP, &config->creep);
		prefsHandler->read(EETH_MIN_ACCEL_REGEN, &config->minimumRegen);
		prefsHandler->read(EETH_MAX_ACCEL_REGEN, &config->maximumRegen);
	} else { //checksum invalid. Reinitialize values, leave storing them to the subclasses
		config->positionRegenMinimum = ThrottleRegenMinValue;
		config->positionRegenMaximum = ThrottleRegenMaxValue;
		config->positionForwardMotionStart = ThrottleFwdValue;
		config->positionHalfPower = ThrottleMapValue;
		config->creep = ThrottleCreepValue;
		config->minimumRegen = ThrottleMinRegenValue; //percentage of minimal power to use when regen starts
		config->maximumRegen = ThrottleMaxRegenValue; //percentage of full power to use for regen at throttle
	}
	Logger::debug(THROTTLE, "RegenMax: %l RegenMin: %l Fwd: %l Map: %l", config->positionRegenMaximum, config->positionRegenMinimum,
			config->positionForwardMotionStart, config->positionHalfPower);
	Logger::debug(THROTTLE, "MinRegen: %d MaxRegen: %d", config->minimumRegen, config->maximumRegen);
}

/*
 * Store the current configuration to EEPROM
 */
void Throttle::saveConfiguration() {
	ThrottleConfiguration *config = (ThrottleConfiguration *) getConfiguration();

	Device::saveConfiguration(); // call parent

	prefsHandler->write(EETH_REGEN_MIN, config->positionRegenMinimum);
	prefsHandler->write(EETH_REGEN_MAX, config->positionRegenMaximum);
	prefsHandler->write(EETH_FWD, config->positionForwardMotionStart);
	prefsHandler->write(EETH_MAP, config->positionHalfPower);
	prefsHandler->write(EETH_CREEP, config->creep);
	prefsHandler->write(EETH_MIN_ACCEL_REGEN, config->minimumRegen);
	prefsHandler->write(EETH_MAX_ACCEL_REGEN, config->maximumRegen);
	prefsHandler->saveChecksum();

	Logger::console("Throttle configuration saved");
}
