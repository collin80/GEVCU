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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_POT_BRAKE
#include "PotBrake.h"

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
PotBrake::PotBrake(uint8_t brake1, uint8_t brake2) :
		Throttle() {
	brake1ADC = brake1;
	brake2ADC = brake2;
	if (brake2 == 255)
		numberPotMeters = 1;
	else
		numberPotMeters = 2;
	brakeStatus = OK;
	//analogReadResolution(12);
}

void PotBrake::setup() {
	TickHandler::getInstance()->detach(this); // unregister from TickHandler first
	Throttle::setup(); //call base class
	//set digital ports to inputs and pull them up
	//all inputs currently active low
	//pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
#ifndef USE_HARD_CODED	
	 if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		prefsHandler->read(EETH_BRAKE_MIN, &minimumLevel1);
		prefsHandler->read(EETH_BRAKE_MAX, &maximumLevel1);
		prefsHandler->read(EETH_MAX_BRAKE_REGEN, &maximumRegen);
		prefsHandler->read(EETH_MIN_BRAKE_REGEN, &minimumRegen);
		Logger::debug(POTBRAKEPEDAL, "BRAKE MIN: %l MAX: %l", minimumLevel1, maximumLevel1);
		Logger::debug(POTBRAKEPEDAL, "Min: %l MaxRegen: %l", minimumRegen, maximumRegen);
	 }
	 else { //checksum invalid. Reinitialize values and store to EEPROM
	 
		//these four values are ADC values
		//The next three are tenths of a percent
		maximumRegen = BrakeMaxRegenValue; //percentage of full power to use for regen at brake pedal transducer
		minimumRegen = BrakeMinRegenValue;
		minimumLevel1 = BrakeMinValue;
		maximumLevel1 = BrakeMaxValue;
		saveEEPROM();
	}
#else
	 	 maximumRegen = BrakeMaxRegenValue;
	 	 minimumRegen = BrakeMinRegenValue;
	 	 minimumLevel1 = BrakeMinValue;
	 	 maximumLevel1 = BrakeMaxValue;
#endif
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

int PotBrake::getRawThrottle1() {
	return rawLevel1;
}

int PotBrake::getRawThrottle2() {
	return rawLevel2;
}

int PotBrake::calcBrake(int clampedVal, int minVal, int maxVal) {
	return map(clampedVal, minVal, maxVal, 0, 1000);
}

/*
 the brake only really uses one variable input and uses different parameters

 story time: this code will start at ThrottleMaxRegen when applying the brake. It
 will do this even if you're currently flooring it. The accelerator pedal is ignored
 if there is any pressure detected on the brake. This is a sort of failsafe. It should
 not be possible to go racing down the road with a stuck accelerator. As soon as the
 brake is pressed it overrides the accelerator signal. Sorry, no standing burn outs.

 */
void PotBrake::doBrake() {
	signed int range;
	signed int calcBrake1, calcBrake2, clampedVal, tempLow, temp;
	static int brakeAvg = 0, brakeFeedback = 0; //used to create proportional control

	clampedVal = rawLevel1;

	if (maximumLevel1 == 0) { //brake processing disabled if Max is 0
		level = 0;
		return;
	}

	//We do not raise a fault of the brake goes too high. We just clamp. This will lock regen on full blast.
	if (rawLevel1 > maximumLevel1) {
		clampedVal = maximumLevel1;
	}

	tempLow = 0;
	if (minimumLevel1 > 14) {
		tempLow = minimumLevel1 - 15;
	}
	if (rawLevel1 < minimumLevel1) {
		if (rawLevel1 < tempLow) {
			brakeStatus = ERR_LOW_T1;
			//Logger::debug(POTBRAKEPEDAL, "B");
		}
		clampedVal = minimumLevel1;
	}

	if (!(brakeStatus == OK)) {
		level = 0; //no throttle if there is a fault
		return;
	}
	calcBrake1 = calcBrake(clampedVal, minimumLevel1, maximumLevel1);
	//Logger::debug(POTBRAKEPEDAL, "calcBrake: %d", calcBrake1);

	//Apparently all is well with the throttle input
	//so go ahead and calculate the proper throttle output

	//still use this smoothing/easing code for the brake. It works quickly enough
	brakeAvg += calcBrake1;
	brakeAvg -= brakeFeedback;
	brakeFeedback = brakeAvg >> 4;

	level = 0; //by default we give zero throttle

	//I suppose I should explain. This prevents flutter in the ADC readings of the brake from slamming
	//regen on intermittantly just because the value fluttered a couple of numbers. This makes sure
	//that we're actually pushing the pedal. Without this even a small flutter at the brake will send
	//ThrottleMaxRegen regen out and ignore the accelerator. That'd be unpleasant.
	if (brakeFeedback < 15) {
		level = 0;
		return;
	}

	if (maximumRegen != 0) { //is the brake regen even enabled?
		int range = maximumRegen - minimumRegen; //we start the brake at ThrottleMaxRegen so the range is reduced by that amount
		//Logger::debug(POTBRAKEPEDAL, "range: %d", range);
		//Logger::debug(POTBRAKEPEDAL, "brakeFeedback: %d", brakeFeedback);
		if (range < 1) { //detect stupidity and abort
			level = 0;
			return;
		}
		level = (signed int) ((signed int) -10 * range * brakeFeedback) / (signed int) 1000;
		level -= 10 * minimumRegen;
		//Logger::debug(POTBRAKEPEDAL, "level: %d", level);
	}

}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void PotBrake::handleTick() {
	sys_io_adc_poll();

	rawLevel1 = getAnalog(brake1ADC);
	if (numberPotMeters > 1)
		rawLevel2 = getAnalog(brake2ADC);

	// Call parent handleTick
	Throttle::handleTick();

	brakeStatus = OK;
	doBrake();
}

PotBrake::BrakeStatus PotBrake::getStatus() {
	return brakeStatus;
}

DeviceId PotBrake::getId() {
	return (POTBRAKEPEDAL);
}

DeviceType PotBrake::getType() {
	return (DEVICE_BRAKE);
}

void PotBrake::saveEEPROM() {
	prefsHandler->write(EETH_BRAKE_MIN, minimumLevel1);
	prefsHandler->write(EETH_BRAKE_MAX, maximumLevel1);
	prefsHandler->write(EETH_MAX_BRAKE_REGEN, maximumRegen);
	prefsHandler->write(EETH_MIN_BRAKE_REGEN, minimumRegen);
	prefsHandler->saveChecksum();
}

void PotBrake::saveConfiguration() {
  Logger::info(POTBRAKEPEDAL, "Saving brake settings");
  TickHandler::getInstance()->detach(this); // unregister from TickHandler first
  setMinumumLevel1(throttleDetector->getThrottle1Min());
  setMaximumLevel1(throttleDetector->getThrottle1Max());
  saveEEPROM();  
  TickHandler::getInstance()->attach(this, getTickInterval());
}

#endif // CFG_ENABLE_DEVICE_POT_BRAKE
