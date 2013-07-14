/*
 * throttle.cpp
 *
 * Parent class for all throttle controllers
 *
 * Created: 02/05/2013
 *  Author: Collin Kidder
 */

#include "throttle.h" 

Throttle::Throttle() : Device() {
	prefs = new PrefHandler(EE_THROTTLE_START);
}

Throttle::Throttle(CanHandler *canHandler) : Device(canHandler) {
	prefs = new PrefHandler(EE_THROTTLE_START);
}

Device::DeviceType Throttle::getDeviceType() {
	return Device::DEVICE_THROTTLE;
}

int Throttle::getThrottle() {
	return outputThrottle;
}
