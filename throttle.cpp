/*
 * throttle.cpp
 *
 * Parent class for all throttle controllers
 *
 * Created: 02/05/2013
 *  Author: Collin Kidder
 */ 
 
#include "throttle.h" 
 
volatile void Throttle::handleTick() {
}

Device::DeviceType Throttle::getDeviceType() {
	return Device::DEVICE_THROTTLE;
}

Device::DeviceId Throttle::getDeviceID() {
	return Device::INVALID;
}

int Throttle::getThrottle() {
	return outputThrottle;
}

void Throttle::setupDevice() {
}

Throttle::Throttle() : Device(0) {
  prefs = new PrefHandler(EE_THROTTLE_START);
}
