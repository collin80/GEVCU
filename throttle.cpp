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

THROTTLE::THROTTLE() : DEVICE(0) {
}
