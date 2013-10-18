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
#include "PotThrottle.h"

/*
 * Constructor
 * Set which two ADC channels to use (or set channel 2 to 255 to disable)
 */
PotThrottle::PotThrottle(uint8_t throttle1Pin, uint8_t throttle2Pin) :
		Throttle() {
	prefsHandler = new PrefHandler(POTACCELPEDAL);
	throttle1AdcPin = throttle1Pin;
	throttle2AdcPin = throttle2Pin;
	throttleStatus = OK;
}

/*
 * Setup the device.
 */
void PotThrottle::setup() {
	TickHandler::getInstance()->detach(this); // unregister from TickHandler first

	loadConfiguration();

	Throttle::setup(); //call base class

	//set digital ports to inputs and pull them up all inputs currently active low
	//pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

/*
 * Process a timer event.
 */
void PotThrottle::handleTick() {
	Throttle::handleTick(); // Call parent which controls the workflow
}

/*
 * Retrieve raw input signals from the throttle hardware.
 */
RawSignalData *PotThrottle::acquireRawSignal() {
	PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

	sys_io_adc_poll();

	rawSignal.input1 = getAnalog(throttle1AdcPin);
	rawSignal.input2 = getAnalog(throttle2AdcPin);

	return &rawSignal;
}

/*
 * Perform sanity check on the ADC input values.
 */
bool PotThrottle::validateSignal(RawSignalData *rawValues) {
	PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
	uint16_t calcThrottle1, calcThrottle2;
	throttleStatus = OK;

	if (rawSignal.input1 > (config->maximumLevel1 + CFG_THROTTLE_TOLERANCE)) {
			throttleStatus = ERR_HIGH_T1;
		Logger::error(POTACCELPEDAL, "ERR_HIGH_T1: throttle 1 value out of range: %l", rawSignal.input1);
		return false;
		}
	if (rawSignal.input1 < (config->minimumLevel1 - CFG_THROTTLE_TOLERANCE)) {
			throttleStatus = ERR_LOW_T1;
		Logger::error(POTACCELPEDAL, "ERR_LOW_T1: throttle 1 value out of range: %l ", rawSignal.input1);
		return false;
		}

	if (config->numberPotMeters > 1) {
		if (rawSignal.input2 > (config->maximumLevel2 + CFG_THROTTLE_TOLERANCE)) {
				throttleStatus = ERR_HIGH_T2;
			Logger::error(POTACCELPEDAL, "ERR_HIGH_T2: throttle 2 value out of range: %l", rawSignal.input2);
			return false;
			}
		if (rawSignal.input2 < (config->minimumLevel2 - CFG_THROTTLE_TOLERANCE)) {
				throttleStatus = ERR_LOW_T2;
			Logger::error(POTACCELPEDAL, "ERR_LOW_T2: throttle 2 value out of range: %l", rawSignal.input2);
			return false;
			}

		calcThrottle1 = map(constrain(rawSignal.input1, config->minimumLevel1, config->maximumLevel1), config->minimumLevel1, config->maximumLevel1,
				(uint16_t) 0, (uint16_t) 1000);
		calcThrottle2 = map(constrain(rawSignal.input2, config->minimumLevel2, config->maximumLevel2), config->minimumLevel2, config->maximumLevel2,
				(uint16_t) 0, (uint16_t) 1000);
		if ((calcThrottle1 - ThrottleMaxErrValue) > calcThrottle2) { //then throttle1 is too large compared to 2
			throttleStatus = ERR_MISMATCH;
			Logger::error(POTACCELPEDAL, "throttle 1 too high (%l) compared to 2 (%l)", calcThrottle1, calcThrottle2);
			return false;
		}
		if ((calcThrottle2 - ThrottleMaxErrValue) > calcThrottle1) { //then throttle2 is too large compared to 1
			throttleStatus = ERR_MISMATCH;
			Logger::error(POTACCELPEDAL, "throttle 2 too high (%l) compared to 1 (%l)", calcThrottle1, calcThrottle2);
			return false;
		}
	}
	return true;
}

/*
 * Convert the raw ADC values to a range from 0 to 1000 (per mille) according
 * to the specified range and the type of potentiometer.
 */
uint16_t PotThrottle::calculatePedalPosition(RawSignalData *rawValues) {
	PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
	uint16_t calcThrottle1, calcThrottle2, clampedLevel;

	clampedLevel = constrain(rawSignal.input1, config->minimumLevel1, config->maximumLevel1);
	calcThrottle1 = map(clampedLevel, config->minimumLevel1, config->maximumLevel1, (uint16_t) 0, (uint16_t) 1000);

	if (config->numberPotMeters > 1) {
		clampedLevel = constrain(rawSignal.input2, config->minimumLevel2, config->maximumLevel2);
		calcThrottle2 = map(clampedLevel, config->minimumLevel2, config->maximumLevel2, (uint16_t) 0, (uint16_t) 1000);
		calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; // now the average of the two
	}
	return calcThrottle1;
}

/*
 * Is the throttle faulted?
 */
bool PotThrottle::isFaulted() {
	return throttleStatus != OK;
}

/*
 * Return the throttle's current status
 */
PotThrottle::ThrottleStatus PotThrottle::getStatus() {
	return throttleStatus;
}

/*
 * Return the device ID
 */
DeviceId PotThrottle::getId() {
	return (POTACCELPEDAL);
}

/*
 * Load the device configuration.
 * If possible values are read from EEPROM. If not, reasonable default values
 * are chosen and the configuration is overwritten in the EEPROM.
 */
void PotThrottle::loadConfiguration() {
	PotThrottleConfiguration *config = (PotThrottleConfiguration *)getConfiguration();

	if(!config) { // as lowest sub-class make sure we have a config object
		config = new PotThrottleConfiguration();
		setConfiguration(config);
	}

	Throttle::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED
	if (false) {
#else
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
		Logger::debug(POTACCELPEDAL, "Valid checksum so using stored throttle config values");
		prefsHandler->read(EETH_MIN_ONE, &config->minimumLevel1);
		prefsHandler->read(EETH_MAX_ONE, &config->maximumLevel1);
		prefsHandler->read(EETH_MIN_TWO, &config->minimumLevel2);
		prefsHandler->read(EETH_MAX_TWO, &config->maximumLevel2);
		prefsHandler->read(EETH_NUM_THROTTLES, &config->numberPotMeters);
		prefsHandler->read(EETH_THROTTLE_TYPE, &config->throttleSubType);

		// ** This is potentially a condition that is only met if you don't have the EEPROM hardware **
		// If preferences have never been set before, numThrottlePots and throttleSubType
		// will both be zero.  We really should refuse to operate in this condition and force
		// calibration, but for now at least allow calibration to work by setting numThrottlePots = 2
		if (config->numberPotMeters == 0 && config->throttleSubType == 0) {
			Logger::debug(POTACCELPEDAL, "THROTTLE APPEARS TO NEED CALIBRATION/DETECTION - choose 'z' on the serial console menu");
			config->numberPotMeters = 2;
		}

		// some safety precautions for new values (depending on eeprom, they might be completely off).
		if (config->creep > 100 || config->positionRegenMaximum > 1000 || config->minimumRegen > 100) {
			config->creep = ThrottleCreepValue;
			config->positionRegenMaximum = ThrottleRegenMaxValue;
			config->minimumRegen = ThrottleMinRegenValue;
		}
	} else { //checksum invalid. Reinitialize values and store to EEPROM
		Logger::warn(POTACCELPEDAL, "Invalid checksum so using hard coded throttle config values");

		config->minimumLevel1 = Throttle1MinValue;
		config->maximumLevel1 = Throttle1MaxValue;
		config->minimumLevel2 = Throttle2MinValue;
		config->maximumLevel2 = Throttle2MaxValue;
		config->numberPotMeters = ThrottleNumPots;
		config->throttleSubType = ThrottleSubtype;

		saveConfiguration();
	}
	Logger::debug(POTACCELPEDAL, "# of pots: %d       subtype: %d", config->numberPotMeters, config->throttleSubType);
	Logger::debug(POTACCELPEDAL, "T1 MIN: %l MAX: %l      T2 MIN: %l MAX: %l", config->minimumLevel1, config->maximumLevel1,
			config->minimumLevel2, config->maximumLevel2);
}

/*
 * Store the current configuration to EEPROM
 */
void PotThrottle::saveConfiguration() {
	PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

	Throttle::saveConfiguration(); // call parent

	prefsHandler->write(EETH_MIN_ONE, config->minimumLevel1);
	prefsHandler->write(EETH_MAX_ONE, config->maximumLevel1);
	prefsHandler->write(EETH_MIN_TWO, config->minimumLevel2);
	prefsHandler->write(EETH_MAX_TWO, config->maximumLevel2);
	prefsHandler->write(EETH_NUM_THROTTLES, config->numberPotMeters);
	prefsHandler->write(EETH_THROTTLE_TYPE, config->throttleSubType);
	prefsHandler->saveChecksum();
}
