/*
 * PotThrottle.cpp
 *
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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_POT_THROTTLE
#include "PotThrottle.h"
#include "Logger.h"

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
PotThrottle::PotThrottle(uint8_t throttle1, uint8_t throttle2) : Throttle() {
	throttle1ADC = throttle1;
	throttle2ADC = throttle2;
	if (throttle2 == 255)
		numThrottlePots = 1;
	else
		numThrottlePots = 2;
	throttleStatus = OK;
	throttleMaxErr = 75; //in tenths of a percent. So 25 = max 2.5% difference
	//analogReadResolution(12);
}

void PotThrottle::setup() {
	TickHandler::getInstance()->detach(this); // unregister from TickHandler first
	Throttle::setup(); //call base class
	//set digital ports to inputs and pull them up
	//all inputs currently active low
	//pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
	/*
	 if (prefs->checksumValid()) { //checksum is good, read in the values stored in EEPROM
	 prefs->read(EETH_MIN_ONE, &throttleMin1);
	 prefs->read(EETH_MAX_ONE, &throttleMax1);
	 prefs->read(EETH_MIN_TWO, &throttleMin2);
	 prefs->read(EETH_MAX_TWO, &throttleMax2);
	 prefs->read(EETH_REGEN, &throttleRegen);
	 prefs->read(EETH_FWD, &throttleFwd);
	 prefs->read(EETH_MAP, &throttleMap);
	 prefs->read(EETH_MAX_ACCEL_REGEN, &ThrottleMaxRegen);
	 }
	 else { //checksum invalid. Reinitialize values and store to EEPROM
	 */
	//these four values are ADC values
	throttleMin1 = 180;
	throttleMax1 = 930;
	throttleMin2 = 360;
	throttleMax2 = 1900;
	//The next three are tenths of a percent
	throttleRegen = 0;
	throttleFwd = 175;
	throttleMap = 665;
	throttleMaxRegen = 00; //percentage of full power to use for regen at throttle
	prefsHandler->write(EETH_MIN_ONE, throttleMin1);
	prefsHandler->write(EETH_MAX_ONE, throttleMax1);
	prefsHandler->write(EETH_MIN_TWO, throttleMin2);
	prefsHandler->write(EETH_MAX_TWO, throttleMax2);
	prefsHandler->write(EETH_REGEN, throttleRegen);
	prefsHandler->write(EETH_FWD, throttleFwd);
	prefsHandler->write(EETH_MAP, throttleMap);
	prefsHandler->write(EETH_MAX_ACCEL_REGEN, throttleMaxRegen);
	prefsHandler->saveChecksum();
	//}
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

int PotThrottle::getRawThrottle1() {
	return throttle1Val;
}

int PotThrottle::getRawThrottle2() {
	return throttle2Val;
}

int PotThrottle::calcThrottle(int clampedVal, int minVal, int maxVal) {
	int range, val, retVal;

	if (minVal < maxVal) { //low to high pot
		range = maxVal - minVal;
		val = clampedVal - minVal;
		retVal = (int) (((long) val * 1000) / (long) range); //output is tenths of a percent of max throttle
	}
	else { //high to low pot
		range = minVal - maxVal;
		val = clampedVal - maxVal;
		retVal = (int) (((long) val * 1000) / (long) range); //output is tenths of a percent of max throttle
		retVal = 1000 - retVal; //reverses the value since the pedal runs reverse
	}

	return retVal;
}

void PotThrottle::doAccel() {
	signed int range;
	signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
	static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

	clampedVal = throttle1Val;

	//The below code now only faults if the value of the ADC is 15 outside of the range +/-
	//otherwise we'll just clamp
	if (throttle1Val > throttleMax1) {
		if (throttle1Val > (throttleMax1 + CFG_THROTTLE_TOLERANCE)) {
			throttleStatus = ERR_HIGH_T1;
			//Logger::debug("T1H ");
		}
		clampedVal = throttleMax1;
	}
	tempLow = 0;
	if (throttleMin1 > (CFG_THROTTLE_TOLERANCE - 1)) {
		tempLow = throttleMin1 - CFG_THROTTLE_TOLERANCE;
	}
	if (throttle1Val < throttleMin1) {
		if (throttle1Val < tempLow) {
			throttleStatus = ERR_LOW_T1;
			//Logger::debug("T1L ");
		}
		clampedVal = throttleMin1;
	}

	if (!(throttleStatus == OK)) {
		level = 0; //no throttle if there is a fault
		return;
	}
	calcThrottle1 = calcThrottle(clampedVal, throttleMin1, throttleMax1);

	if (numThrottlePots > 1) { //can only do these things if there are two or more pots
		clampedVal = throttle2Val;
		if (throttle2Val > throttleMax2) {
			if (throttle2Val > (throttleMax2 + CFG_THROTTLE_TOLERANCE)) {
				throttleStatus = ERR_HIGH_T2;
				//Logger::debug("T2H ");
			}
			clampedVal = throttleMax2;
		}
		tempLow = 0;
		if (throttleMin2 > (CFG_THROTTLE_TOLERANCE - 1)) {
			tempLow = throttleMin2 - CFG_THROTTLE_TOLERANCE;
		}
		if (throttle2Val < throttleMin2) {
			if (throttle2Val < tempLow) {
				throttleStatus = ERR_LOW_T2;
				//Logger::debug("T2L ");
			}
			clampedVal = throttleMin2;
		}

		calcThrottle2 = calcThrottle(clampedVal, throttleMin2, throttleMax2);

		if ((calcThrottle1 - throttleMaxErr) > calcThrottle2) { //then throttle1 is too large compared to 2
			throttleStatus = ERR_MISMATCH;
			//Logger::debug("MX1 ");
		}
		if ((calcThrottle2 - throttleMaxErr) > calcThrottle1) { //then throttle2 is too large compared to 1
			throttleStatus = ERR_MISMATCH;
			//Logger::debug("MX2 ");
		}

		calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; //temp now the average of the two
	}

	if (!(throttleStatus == OK)) {
		level = 0; //no throttle if there is a fault
		return;
	}

	//Apparently all is well with the throttle input
	//so go ahead and calculate the proper throttle output

	ThrottleAvg += calcThrottle1;
	ThrottleAvg -= ThrottleFeedback;
	ThrottleFeedback = ThrottleAvg >> 4;

	level = 0; //by default we give zero throttle

	/* Since this code is now based on tenths of a percent of throttle push it is now agnostic to how that happens
	 positive or negative travel doesn't matter and is covered by the calcThrottle functions
	 */

	mapThrottle(ThrottleFeedback);
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void PotThrottle::handleTick() {

	//Logger::debug("Pot Throttle HandleTick");

	sys_io_adc_poll();

	throttle1Val = getAnalog(throttle1ADC);
	if (numThrottlePots > 1) {
		throttle2Val = getAnalog(throttle2ADC);
	}

	throttleStatus = OK;
	doAccel();
}

PotThrottle::ThrottleStatus PotThrottle::getStatus() {
	return throttleStatus;
}

void PotThrottle::setT1Min(uint16_t min) {
	throttleMin1 = min;
}

void PotThrottle::setT2Min(uint16_t min) {
	throttleMin2 = min;
}

void PotThrottle::setT1Max(uint16_t max) {
	throttleMax1 = max;
}
void PotThrottle::setT2Max(uint16_t max) {
	throttleMax2 = max;
}

Device::DeviceId PotThrottle::getId() {
	return (POTACCELPEDAL);
}


#endif //CFG_ENABLE_DEVICE_POT_THROTTLE
