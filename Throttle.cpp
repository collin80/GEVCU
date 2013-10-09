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
 
Throttle::Throttle() : Device() {
	prefsHandler = new PrefHandler(EE_THROTTLE_START);
    throttleDetector = NULL;
	level = 0;
}

Throttle::~Throttle() {
  if ( throttleDetector != NULL ) {
    delete throttleDetector;
    throttleDetector = NULL;
  }
}

void Throttle::handleTick() {
	Device::handleTick();
	if ( throttleDetector != NULL ) {
	    throttleDetector->handleTick();
	}
}

DeviceType Throttle::getType(){
	return DEVICE_THROTTLE;
}

int16_t Throttle::getLevel(){
	return level;
}

void Throttle::setNumberPotMeters(uint8_t num) {
	numberPotMeters = constrain(num, 1, 2); // Currently only valid values are 1  and 2
}

uint8_t Throttle::getNumberPotMeters() {
	return numberPotMeters;
}

void Throttle::setSubtype(uint8_t num) {
	throttleSubType = constrain(num, 1, 2); // Currently only valid values are 1  and 2
}

uint8_t Throttle::getSubtype() {
	return throttleSubType;
}

/*
 * Give default versions that return 0. Override in a child class if you implement the throttle
 */
uint16_t Throttle::getRawThrottle1() {return 0;}
uint16_t Throttle::getRawThrottle2() {return 0;}

/*
 * Return the tick interval for this throttle. Override in a child class
 * if you use a different tick interval
 */
uint32_t Throttle::getTickInterval() {
	return CFG_TICK_INTERVAL_POT_THROTTLE;
}

/*
 * Maps the input throttle position (0-1000 permille) to an output level which is calculated
 * based on the throttle mapping parameters.
 */
void Throttle::mapThrottle(int16_t currentPosition) {
	int16_t range, value;

	level = 0; //by default we give zero throttle

	if (currentPosition > 0) {
		if (positionRegenStart != 0) {
			if (currentPosition <= positionRegenStart) {
				range = positionRegenStart;
				value = range - currentPosition;
				level = -10 * maximumRegen * value / range;
			}
		}

		if (currentPosition >= positionForwardMotionStart) {
			if (currentPosition <= positionHalfPower) { //bottom 50% forward
				range = positionHalfPower - positionForwardMotionStart;
				value = currentPosition - positionForwardMotionStart;
				level = 500 * value / range;
			}
			else { //more than throttleMap
				range = 1000 - positionHalfPower;
				value = currentPosition - positionHalfPower;
				level = 500 + 500 * value / range;
			}
		}
	}
	else {
		level = -10 * maximumRegen;
	}
	//Logger::debug("throttle level: %d", level);
}

void Throttle::detectThrottle() {
  if ( throttleDetector == NULL )
    throttleDetector = new ThrottleDetector(this);
  throttleDetector->detect();
}

void Throttle::setMinumumLevel1(uint16_t min) {
	minimumLevel1 = min;
}

uint16_t Throttle::getMinimumLevel1() {
	return minimumLevel1;
}

void Throttle::setMinimumLevel2(uint16_t min) {
	minimumLevel2 = min;
}

uint16_t Throttle::getMinimumLevel2() {
	return minimumLevel2;
}

void Throttle::setMaximumLevel1(uint16_t max) {
	maximumLevel1 = max;
}

uint16_t Throttle::getMaximumLevel1() {
	return maximumLevel1;
}

void Throttle::setMaximumLevel2(uint16_t max) {
	maximumLevel2 = max;
}

uint16_t Throttle::getMaximumLevel2() {
	return maximumLevel2;
}

void Throttle::setPositionRegenStart(uint16_t regen) {
	positionRegenStart = regen;
}

uint16_t Throttle::getPositionRegenStart() {
	return positionRegenStart;
}

void Throttle::setPositionForwardMotionStart(uint16_t fwd) {
	positionForwardMotionStart = fwd;
}

uint16_t Throttle::getPositionForwardMotionStart() {
	return positionForwardMotionStart;
}

void Throttle::setPositionHalfPower(uint16_t map) {
	positionHalfPower = map;
}

uint16_t Throttle::getPositionHalfPower() {
	return positionHalfPower;
}

void Throttle::setMaximumRegen(uint16_t regen) {
	maximumRegen = regen;
}

uint16_t Throttle::getMaximumRegen() {
	return maximumRegen;
}

void Throttle::setMinimumRegen(uint16_t regen) {
	minimumRegen = regen;
}

uint16_t Throttle::getMinimumRegen() {
	return minimumRegen;
}

void Throttle::saveConfiguration() {
}

void Throttle::saveEEPROM() {
}
