/*
 * PotBrake.cpp
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

#include "PotBrake.h"

/*
 * Constructor
 * Set which ADC channel to use
 */
PotBrake::PotBrake(uint8_t brake1) : Throttle() {
	prefsHandler = new PrefHandler(POTBRAKEPEDAL);
	brake1AdcPin = brake1;
}

/*
 * Setup the device.
 */
void PotBrake::setup() {
	TickHandler::getInstance()->detach(this); // unregister from TickHandler first
	Throttle::setup(); //call base class

	//set digital ports to inputs and pull them up all inputs currently active low
	//pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch

	loadConfiguration();
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

/*
 * Process a timer event.
 */
void PotBrake::handleTick() {
	Throttle::handleTick(); // Call parent which controls the workflow
}

/*
 * Retrieve raw input signals from the brake hardware.
 */
RawSignalData *PotBrake::acquireRawSignal() {
	sys_io_adc_poll();
	rawSignal.input1 = getAnalog(brake1AdcPin);
	return &rawSignal;
}

/*
 * Perform sanity check on the ADC input values.
 */
bool PotBrake::validateSignal(RawSignalData *rawSignal) {
	PotBrakeConfiguration *config = (PotBrakeConfiguration *) getConfiguration();

	if (rawSignal->input1 > (config->maximumLevel1 + CFG_THROTTLE_TOLERANCE)) {
		if (status == OK)
			Logger::error(POTBRAKEPEDAL, (char *)Constants::valueOutOfRange, rawSignal->input1);
		status = ERR_HIGH_T1;
		return true; // even if it's too high, let it process and apply full regen !
	}
	if (rawSignal->input1 < (config->minimumLevel1 - CFG_THROTTLE_TOLERANCE)) {
		if (status == OK)
			Logger::error(POTBRAKEPEDAL, (char *)Constants::valueOutOfRange, rawSignal->input1);
		status = ERR_LOW_T1;
		return false;
	}

	// all checks passed -> brake is OK
	if (status != OK)
		Logger::info(POTBRAKEPEDAL, (char *)Constants::normalOperation);
	status = OK;
	return true;
}

/*
 * Convert the raw ADC values to a range from 0 to 1000 (per mille) according
 * to the specified range and the type of potentiometer.
 */
uint16_t PotBrake::calculatePedalPosition(RawSignalData *rawSignal) {
	PotBrakeConfiguration *config = (PotBrakeConfiguration *) getConfiguration();
	uint16_t calcBrake1, clampedLevel;

	if (config->maximumLevel1 == 0) //brake processing disabled if max is 0
		return 0;

	clampedLevel = constrain(rawSignal->input1, config->minimumLevel1, config->maximumLevel1);
	calcBrake1 = map(clampedLevel, config->minimumLevel1, config->maximumLevel1, (uint16_t) 0, (uint16_t) 1000);

	//This prevents flutter in the ADC readings of the brake from slamming regen on intermittently
	// just because the value fluttered a couple of numbers. This makes sure that we're actually
	// pushing the pedal. Without this even a small flutter at the brake will send minregen
	// out and ignore the accelerator. That'd be unpleasant.
	if (calcBrake1 < 15)
		calcBrake1 = 0;

	return calcBrake1;
}

/*
 * Overrides the standard implementation of throttle mapping as different rules apply to
 * brake based regen.
 */
int16_t PotBrake::mapPedalPosition(int16_t pedalPosition) {
	ThrottleConfiguration *config = (ThrottleConfiguration *) getConfiguration();
	int16_t brakeLevel, range;

	range = config->maximumRegen - config->minimumRegen;
	brakeLevel = -10 * range * pedalPosition / 1000;
	brakeLevel -= 10 * config->minimumRegen;
	//Logger::debug(POTBRAKEPEDAL, "level: %d", level);

	return brakeLevel;
}

/*
 * Return the device ID
 */
DeviceId PotBrake::getId() {
	return (POTBRAKEPEDAL);
}

/*
 * Return the device type
 */
DeviceType PotBrake::getType() {
	return (DEVICE_BRAKE);
}

/*
 * Load the device configuration.
 * If possible values are read from EEPROM. If not, reasonable default values
 * are chosen and the configuration is overwritten in the EEPROM.
 */
void PotBrake::loadConfiguration() {
	PotBrakeConfiguration *config = new PotBrakeConfiguration();
	setConfiguration(config);

	// we deliberately do not load config via parent class here !

#ifdef USE_HARD_CODED
	if (false) {
#else
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
		prefsHandler->read(EETH_BRAKE_MIN, &config->minimumLevel1);
		prefsHandler->read(EETH_BRAKE_MAX, &config->maximumLevel1);
		prefsHandler->read(EETH_MAX_BRAKE_REGEN, &config->maximumRegen);
		prefsHandler->read(EETH_MIN_BRAKE_REGEN, &config->minimumRegen);
		Logger::debug(POTBRAKEPEDAL, "BRAKE MIN: %l MAX: %l", config->minimumLevel1, config->maximumLevel1);
		Logger::debug(POTBRAKEPEDAL, "Min: %l MaxRegen: %l", config->minimumRegen, config->maximumRegen);
	} else { //checksum invalid. Reinitialize values and store to EEPROM

		//these four values are ADC values
		//The next three are tenths of a percent
		config->maximumRegen = BrakeMaxRegenValue; //percentage of full power to use for regen at brake pedal transducer
		config->minimumRegen = BrakeMinRegenValue;
		config->minimumLevel1 = BrakeMinValue;
		config->maximumLevel1 = BrakeMaxValue;
		saveConfiguration();
	}
}

/*
 * Store the current configuration to EEPROM
 */
void PotBrake::saveConfiguration() {
	PotBrakeConfiguration *config = (PotBrakeConfiguration *) getConfiguration();

	// we deliberately do not save config via parent class here !

	prefsHandler->write(EETH_BRAKE_MIN, config->minimumLevel1);
	prefsHandler->write(EETH_BRAKE_MAX, config->maximumLevel1);
	prefsHandler->write(EETH_MAX_BRAKE_REGEN, config->maximumRegen);
	prefsHandler->write(EETH_MIN_BRAKE_REGEN, config->minimumRegen);
	prefsHandler->saveChecksum();
}

