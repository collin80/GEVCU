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
}

THROTTLE::THROTTLE() : DEVICE(0) {
    pref_base_addr = EE_THROTTLE_START;
}
