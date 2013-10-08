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

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
PotThrottle::PotThrottle(uint8_t throttle1, uint8_t throttle2) : Throttle() {
	throttle1ADC = throttle1;
	throttle2ADC = throttle2;
	if (throttle2 == CFG_THROTTLE_NONE)
		numberPotMeters = 1;
	else
		numberPotMeters = 2;
	throttleStatus = OK;
	throttleMaxErr = ThrottleMaxErrValue; //in tenths of a percent. So 25 = max 2.5% difference
	//analogReadResolution(12);
}

void PotThrottle::setup() {
	TickHandler::getInstance()->detach(this); // unregister from TickHandler first
	Throttle::setup(); //call base class
	//set digital ports to inputs and pull them up
	//all inputs currently active low
	//pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
	
#ifndef USE_HARD_CODED
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		Logger::debug(POTACCELPEDAL, "Valid checksum so using stored throttle config values");
		prefsHandler->read(EETH_MIN_ONE, &minimumLevel1);
		prefsHandler->read(EETH_MAX_ONE, &maximumLevel1);
		prefsHandler->read(EETH_MIN_TWO, &minimumLevel2);
		prefsHandler->read(EETH_MAX_TWO, &maximumLevel2);
		prefsHandler->read(EETH_REGEN, &positionRegenStart);
		prefsHandler->read(EETH_FWD, &positionForwardMotionStart);
		prefsHandler->read(EETH_MAP, &positionHalfPower);
		prefsHandler->read(EETH_MAX_ACCEL_REGEN, &maximumRegen);
		prefsHandler->read(EETH_NUM_THROTTLES, &numberPotMeters);
		prefsHandler->read(EETH_THROTTLE_TYPE, &throttleSubType);

		// ** This is potentially a condition that is only met if you don't have the EEPROM hardware **
		// If preferences have never been set before, numThrottlePots and throttleSubType
		// will both be zero.  We really should refuse to operate in this condition and force
		// calibration, but for now at least allow calibration to work by setting numThrottlePots = 2
		if (numberPotMeters == 0 && throttleSubType == 0) {
			Logger::debug(POTACCELPEDAL, "THROTTLE APPEARS TO NEED CALIBRATION/DETECTION - choose 'z' on the serial console menu");
			numberPotMeters = 2;
		}
		
		Logger::debug(POTACCELPEDAL, "# of pots: %d       subtype: %d", numberPotMeters, throttleSubType);
		Logger::debug(POTACCELPEDAL, "T1 MIN: %l MAX: %l      T2 MIN: %l MAX: %l", minimumLevel1, maximumLevel1, minimumLevel2, maximumLevel2);
		Logger::debug(POTACCELPEDAL, "Regen: %l Fwd: %l Map: %l MaxRegen: %d", positionRegenStart, positionForwardMotionStart, positionHalfPower, maximumRegen);
	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		Logger::debug(POTACCELPEDAL, "Invalid checksum so using hard coded throttle config values");

		 //The next three are tenths of a percent
		positionRegenStart = ThrottleRegenValue;
		positionForwardMotionStart = ThrottleFwdValue;
		positionHalfPower = ThrottleMapValue;
		maximumRegen = ThrottleMaxRegenValue; //percentage of full power to use for regen at throttle
		minimumLevel1 = Throttle1MinValue;
		maximumLevel1 = Throttle1MaxValue;
		minimumLevel2 = Throttle2MinValue;
		maximumLevel2 = Throttle2MaxValue;
		numberPotMeters = ThrottleNumPots;
		throttleSubType = ThrottleSubtype;

		prefsHandler->write(EETH_MIN_ONE, minimumLevel1);
		prefsHandler->write(EETH_MAX_ONE, maximumLevel1);
		prefsHandler->write(EETH_MIN_TWO, minimumLevel2);
		prefsHandler->write(EETH_MAX_TWO, maximumLevel2);
		prefsHandler->write(EETH_REGEN, positionRegenStart);
		prefsHandler->write(EETH_FWD, positionForwardMotionStart);
		prefsHandler->write(EETH_MAP, positionHalfPower);
		prefsHandler->write(EETH_MAX_ACCEL_REGEN, maximumRegen);
		prefsHandler->write(EETH_NUM_THROTTLES, numberPotMeters);
		prefsHandler->write(EETH_THROTTLE_TYPE, throttleSubType);
		prefsHandler->saveChecksum();
	}
#else
	Logger::debug(POTACCELPEDAL, "#define USE_HARD_CODED so using hard coded throttle config values");
	positionRegenStart = ThrottleRegenValue;
	positionForwardMotionStart = ThrottleFwdValue;
	positionHalfPower = ThrottleMapValue;
	maximumRegen = ThrottleMaxRegenValue; //percentage of full power to use for regen at throttle
	minimumLevel1 = Throttle1MinValue;
	maximumLevel1 = Throttle1MaxValue;
	minimumLevel2 = Throttle2MinValue;
	maximumLevel2 = Throttle2MaxValue;
#endif

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

int PotThrottle::getRawThrottle1() {
	return rawLevel1;
}

int PotThrottle::getRawThrottle2() {
	return rawLevel2;
}

/*
 * Convert the raw ADC values to a range from 0 to 1000 (per mille) according
 * to the specified range and the type of potentiometer.
 */
int PotThrottle::calcThrottle(int clampedVal, int minVal, int maxVal) {
	return map(clampedVal, minVal, maxVal, 0, 1000);
}

/*
 * Perform sanity check on the ADC input values, convert them to per mille values
 * optionally average two throttle inputs and in the end map the calculated throttle
 * position to a regen/acceleration curve.
 */
void PotThrottle::doAccel() {
	uint16_t calcThrottle1, calcThrottle2, clampedLevel, tempLow;
	throttleAverage = 0;
	throttleFeedback = 0;

	clampedLevel = rawLevel1;

	//The below code now only faults if the value of the ADC is 15 outside of the range +/-
	//otherwise we'll just clamp
	if (rawLevel1 > maximumLevel1) {
		if (rawLevel1 > (maximumLevel1 + CFG_THROTTLE_TOLERANCE)) {
			throttleStatus = ERR_HIGH_T1;
			Logger::error(POTACCELPEDAL, "throttle 1 value out of range: %l", rawLevel1);
		}
		clampedLevel = maximumLevel1;
	}
	tempLow = 0;
	if (minimumLevel1 > (CFG_THROTTLE_TOLERANCE - 1)) {
		tempLow = minimumLevel1 - CFG_THROTTLE_TOLERANCE;
	}
	if (rawLevel1 < minimumLevel1) {
		if (rawLevel1 < tempLow) {
			throttleStatus = ERR_LOW_T1;
			Logger::error(POTACCELPEDAL, "throttle 1 value out of range: %l ", rawLevel1);
		}
		clampedLevel = minimumLevel1;
	}

	calcThrottle1 = calcThrottle(clampedLevel, minimumLevel1, maximumLevel1);
	//Logger::debug(POTACCELPEDAL, "calc throttle: %i", calcThrottle1);

	if (numberPotMeters > 1) { //can only do these things if there are two or more pots
		clampedLevel = rawLevel2;
		if (rawLevel2 > maximumLevel2) {
			if (rawLevel2 > (maximumLevel2 + CFG_THROTTLE_TOLERANCE)) {
				throttleStatus = ERR_HIGH_T2;
				Logger::error(POTACCELPEDAL, "throttle 2 value out of range: %l", rawLevel2);
			}
			clampedLevel = maximumLevel2;
		}
		tempLow = 0;
		if (minimumLevel2 > (CFG_THROTTLE_TOLERANCE - 1)) {
			tempLow = minimumLevel2 - CFG_THROTTLE_TOLERANCE;
		}
		if (rawLevel2 < minimumLevel2) {
			if (rawLevel2 < tempLow) {
				throttleStatus = ERR_LOW_T2;
				Logger::error(POTACCELPEDAL, "throttle 2 value out of range: %l", rawLevel2);
			}
			clampedLevel = minimumLevel2;
		}

		calcThrottle2 = calcThrottle(clampedLevel, minimumLevel2, maximumLevel2);

		if ((calcThrottle1 - throttleMaxErr) > calcThrottle2) { //then throttle1 is too large compared to 2
			throttleStatus = ERR_MISMATCH;
			Logger::error(POTACCELPEDAL, "throttle 1 too high (%l) compared to 2 (%l)", calcThrottle1, calcThrottle2);
		}
		if ((calcThrottle2 - throttleMaxErr) > calcThrottle1) { //then throttle2 is too large compared to 1
			throttleStatus = ERR_MISMATCH;
			Logger::error(POTACCELPEDAL, "throttle 2 too high (%l) compared to 1 (%l)", calcThrottle1, calcThrottle2);
		}

		calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; //temp now the average of the two
	}

	if (throttleStatus != OK) {
		level = 0; //no throttle if there is a fault
		Logger::error(POTACCELPEDAL, "throttle faulted (status=%d), setting level to 0", throttleStatus);
		return;
	}

	//Apparently all is well with the throttle input
	//so go ahead and calculate the proper throttle output

	throttleAverage += calcThrottle1;
	throttleAverage -= throttleFeedback;
	throttleFeedback = throttleAverage >> 4;
	mapThrottle(throttleFeedback);
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void PotThrottle::handleTick() {
	sys_io_adc_poll();

	rawLevel1 = getAnalog(throttle1ADC);
	if (numberPotMeters > 1)
		rawLevel2 = getAnalog(throttle2ADC);

	// Call parent handleTick
	Throttle::handleTick();

	throttleStatus = OK;
	doAccel();
}

PotThrottle::ThrottleStatus PotThrottle::getStatus() {
	return throttleStatus;
}


DeviceId PotThrottle::getId() {
	return (POTACCELPEDAL);
}

void PotThrottle::saveEEPROM() {
	prefsHandler->write(EETH_MIN_ONE, minimumLevel1);
	prefsHandler->write(EETH_MAX_ONE, maximumLevel1);
	prefsHandler->write(EETH_MIN_TWO, minimumLevel2);
	prefsHandler->write(EETH_MAX_TWO, maximumLevel2);
	prefsHandler->write(EETH_REGEN, positionRegenStart);
	prefsHandler->write(EETH_FWD, positionForwardMotionStart);
	prefsHandler->write(EETH_MAP, positionHalfPower);
	prefsHandler->write(EETH_MAX_ACCEL_REGEN, maximumRegen);
	prefsHandler->write(EETH_NUM_THROTTLES, numberPotMeters);
	prefsHandler->write(EETH_THROTTLE_TYPE, throttleSubType);
	prefsHandler->saveChecksum();
}

void PotThrottle::saveConfiguration() {
  Logger::info(POTACCELPEDAL, "Saving throttle settings");
  TickHandler::getInstance()->detach(this); // unregister from TickHandler first
  setMinumumLevel1(throttleDetector->getThrottle1Min());
  setMaximumLevel1(throttleDetector->getThrottle1Max());
  setNumberPotMeters(throttleDetector->getPotentiometerCount());
  if ( getNumberPotMeters() > 1 ) {
	setMinimumLevel2(throttleDetector->getThrottle2Min());
	setMaximumLevel2(throttleDetector->getThrottle2Max());
  }
  else {
    setMinimumLevel2(0);
    setMaximumLevel2(0);
  }
  setSubtype(throttleDetector->getSubtype());
  saveEEPROM();
  
  TickHandler::getInstance()->attach(this, getTickInterval());
}


#endif //CFG_ENABLE_DEVICE_POT_THROTTLE
