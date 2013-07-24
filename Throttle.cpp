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

//Give default versions that return 0. Override in a child class if you implement the throttle
int Throttle::getRawThrottle1() {return 0;}
int Throttle::getRawThrottle2() {return 0;}

//a common function to all throttles that takes as input the throttle position percentage
//and outputs an output throttle percentage which is calculated based on the throttle and
//brake mapping parameters. 
void Throttle::mapThrottle(signed int inVal) 
{
	signed int range, temp;

	//Logger::debug("Entering mapThrottle: %i", inVal);

	if (inVal > 0) {
		if (throttleRegen != 0) {
			if (inVal <= throttleRegen) {
				range = throttleRegen;
				temp = range - inVal;
				outputThrottle = (signed long) ((signed long) (-10) * throttleMaxRegen * temp / range);
			}
		}

		if (inVal >= throttleFwd) {
			if (inVal <= throttleMap) { //bottom 50% forward
				range = throttleMap - throttleFwd;
				temp = (inVal - throttleFwd);
				outputThrottle = (signed long) ((signed long) (500) * temp / range);
			}
			else { //more than throttleMap
				range = 1000 - throttleMap;
				temp = (inVal - throttleMap);
				outputThrottle = 500 + (signed int) ((signed long) (500) * temp / range);
			}
		}
	}
	else {
		if (brakeMaxRegen != 0) { //is the brake regen even enabled?
			int range = brakeMaxRegen - throttleMaxRegen; //we start the brake at ThrottleMaxRegen so the range is reduced by that amount
			if (range < 1) { //detect stupidity and abort
				outputThrottle = 0;
				return;
			}
			outputThrottle = (signed int) ((signed int) 10 * range * inVal) / (signed int) 1000;
			outputThrottle -= -10 * throttleMaxRegen;
		}
	}
	//Logger::debug("outputThrottle: %d", outputThrottle);
}

void Throttle::setRegenEnd(uint16_t regen) {
	throttleRegen = regen;
}

void Throttle::setFWDStart(uint16_t fwd) {
	throttleFwd = fwd;
}

void Throttle::setMAP(uint16_t map) {
	throttleMap = map;
}

void Throttle::setMaxRegen(uint16_t regen) {
	throttleMaxRegen = regen;
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

