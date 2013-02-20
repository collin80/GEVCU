/*
 * throttle.cpp
 *
 * Parent class for all throttle controllers
 *
 * Created: 02/05/2013
 *  Author: Collin Kidder
 */ 
 
#include "throttle.h" 
 
void THROTTLE::handleTick() {
}

DEVICE::DEVTYPE THROTTLE::getDeviceType() {
	return DEVICE::DEVICE_THROTTLE;
}

DEVICE::DEVID THROTTLE::getDeviceID() {
	return DEVICE::INVALID;
}

int THROTTLE::getThrottle() {
	return outputThrottle;
}

void THROTTLE::setupDevice() {
  //set digital ports to inputs and pull them up
  //all inputs currently active low
  pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
}

THROTTLE::THROTTLE() : DEVICE(0) {
}
