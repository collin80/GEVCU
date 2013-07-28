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
#include "ThrottleDetector.h"
 
Throttle::Throttle() : Device() {
	prefsHandler = new PrefHandler(EE_THROTTLE_START);
        throttleDetector = NULL;
}

Throttle::~Throttle() {
  if ( throttleDetector != NULL ) {
    delete throttleDetector;
    throttleDetector = NULL;
  }
}

Device::DeviceType Throttle::getType() {
	return Device::DEVICE_THROTTLE;
}

int Throttle::getLevel() {
	return level;
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

void Throttle::detectThrottle() {
  //TickHandler::remove(this); // unregister from TickHandler first
  if ( throttleDetector == NULL ) {
    throttleDetector = new ThrottleDetector(this);
  }
  throttleDetector->detect();
  //TickHandler::add(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

void Throttle::detectThrottleMin() {
  //TickHandler::remove(this); // unregister from TickHandler first
  if ( throttleDetector == NULL ) {
    throttleDetector = new ThrottleDetector(this);
  }
  throttleDetector->detectMin();
  //TickHandler::add(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

void Throttle::detectThrottleMax() {
  //TickHandler::remove(this); // unregister from TickHandler first
  if ( throttleDetector == NULL ) {
    throttleDetector = new ThrottleDetector(this);
  }
  throttleDetector->detectMax();
  //TickHandler::add(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

void Throttle::saveConfiguration() {
  Logger::info("Saving throttle settings");
  TickHandler::remove(this); // unregister from TickHandler first
  prefsHandler->write(EETH_MIN_ONE, throttleDetector->getThrottle1Min());
  prefsHandler->write(EETH_MAX_ONE, throttleDetector->getThrottle1Max());
  if ( throttleDetector->getPotentiometerCount() > 1 ) {
	prefsHandler->write(EETH_MIN_TWO, throttleDetector->getThrottle2Min());
	prefsHandler->write(EETH_MAX_TWO, throttleDetector->getThrottle2Max());
  }
  prefsHandler->saveChecksum();
  TickHandler::add(this, CFG_TICK_INTERVAL_POT_THROTTLE);
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

