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
	prefsHandler = new PrefHandler(EE_THROTTLE_START);
	level = 0;
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
	ThrottleConfiguration *config = (ThrottleConfiguration *)getConfiguration();

	throttleLevel = 0;

	if (pedalPosition == 0 && config->creep > 0) {
		throttleLevel = 10 * config->creep;
	} else if (pedalPosition <= config->positionRegenMinimum) {
		if (pedalPosition >= config->positionRegenMaximum) {
			range = config->positionRegenMinimum - config->positionRegenMaximum;
			value = pedalPosition - config->positionRegenMinimum;
			if (range != 0) // prevent div by zero, should result in 0 throttle if min==max
				throttleLevel = -10 * config->minimumRegen + value / range * (config->maximumRegen - config->minimumRegen) * 10;
		} else {
			range = config->positionRegenMaximum;
			value = pedalPosition;
			throttleLevel = -10 * config->maximumRegen * value / range;
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
	return throttleLevel;
}

/*
 * Returns the currently calculated/mapped throttle level (from -1000 to 1000).
 */
int16_t Throttle::getLevel(){
	return level;
}

/*
 * Is the throttle faulted?
 */
bool Throttle::isFaulted() {
	return true;
}

/*
 * Return the device type
 */
DeviceType Throttle::getType(){
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
