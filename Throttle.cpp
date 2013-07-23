/*
 * Throttle.cpp
 *
 * Parent class for all throttle controllers
 *
 * Created: 02/05/2013
 *  Author: Collin Kidder
 */ 
 
#include "Throttle.h" 
 
Throttle::Throttle() : Device() {
	prefsHandler = new PrefHandler(EE_THROTTLE_START);
}

Throttle::Throttle(CanHandler *canHandler) : Device(canHandler) {
	prefsHandler = new PrefHandler(EE_THROTTLE_START);
}

Throttle::~Throttle() {
}

Device::DeviceType Throttle::getType() {
	return Device::DEVICE_THROTTLE;
}

int Throttle::getThrottle() {
	return outputThrottle;
}

//TODO: need to plant this in here somehow..

//if (!runStatic)
//	throttleval++;
//if (throttleval > 80)
//	throttleval = 0;
//if (!runThrottle) { //ramping test
//	if (!runRamp) {
//		throttleval = 0;
//	}
//	motorController->setThrottle(throttleval * (int) 12); //with throttle 0-80 this sets throttle to 0 - 960
//}
//else { //use the installed throttle
//}

