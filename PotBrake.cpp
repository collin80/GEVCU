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
		numBrakePots = 1;
	else
		numBrakePots = 2;
	brakeStatus = OK;
	brakeMaxErr = 75; //in tenths of a percent. So 25 = max 2.5% difference
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
		prefsHandler->read(EETH_BRAKE_MIN, &brakeMin);
		prefsHandler->read(EETH_BRAKE_MAX, &brakeMax);
		prefsHandler->read(EETH_MAX_ACCEL_REGEN, &throttleMaxRegen);
		prefsHandler->read(EETH_MAX_BRAKE_REGEN, &brakeMaxRegen);
		Logger::debug("BRAKE T1 MIN: %i MAX: %i", brakeMin, brakeMax);
		Logger::debug("MaxRegen: %i", brakeMaxRegen);
	 }
	 else { //checksum invalid. Reinitialize values and store to EEPROM
	 
		//these four values are ADC values
		//The next three are tenths of a percent
		throttleMaxRegen = 00; //percentage of full power to use for regen at throttle
		brakeMaxRegen = 80; //percentage of full power to use for regen at brake pedal transducer
		brakeMin = 5;
		brakeMax = 500;
		prefsHandler->write(EETH_BRAKE_MIN, brakeMin);
		prefsHandler->write(EETH_BRAKE_MAX, brakeMax);
		prefsHandler->write(EETH_MAX_BRAKE_REGEN, brakeMaxRegen);
		prefsHandler->saveChecksum();
	}
#else
		throttleMaxRegen = ThrottleMaxRegenValue;
		brakeMaxRegen = BrakeMaxRegenValue;
		brakeMin = BrakeMinValue;
		brakeMax = BrakeMaxValue;
#endif
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

int PotBrake::getRawBrake1() {
	return brake1Val;
}

int PotBrake::getRawBrake2() {
	return brake2Val;
}

int PotBrake::getRawThrottle1() {
	return brake1Val;
}

int PotBrake::getRawThrottle2() {
	return brake2Val;
}

void PotBrake::setMaxRegen(uint16_t regen) 
{
	brakeMaxRegen = regen;
}




int PotBrake::calcBrake(int clampedVal, int minVal, int maxVal) {
	int range, val, retVal;

	if (minVal < maxVal) { //low to high pot
		range = maxVal - minVal;
		val = clampedVal - minVal;
		retVal = (int) (((long) val * 1000) / (long) range); //output is tenths of a percent of max brake
	}
	else { //high to low pot
		range = minVal - maxVal;
		val = clampedVal - maxVal;
		retVal = (int) (((long) val * 1000) / (long) range); //output is tenths of a percent of max brake
		retVal = 1000 - retVal; //reverses the value since the pedal runs reverse
	}

	return retVal;
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
	static uint16_t brakeAvg = 0, brakeFeedback = 0; //used to create proportional control

	clampedVal = brake1Val;

	if (brakeMax == 0) { //brake processing disabled if Max is 0
		level = 0;
		return;
	}

	//The below code now only faults if the value of the ADC is 15 outside of the range +/-
	//otherwise we'll just clamp
	if (brake1Val > brakeMax) {
		if (brake1Val > (brakeMax + 15)) {
			brakeStatus = ERR_HIGH_T1;
			//Logger::debug("A");
		}
		clampedVal = brakeMax;
	}

	tempLow = 0;
	if (brakeMin > 14) {
		tempLow = brakeMin - 15;
	}
	if (brake1Val < brakeMin) {
		if (brake1Val < tempLow) {
			brakeStatus = ERR_LOW_T1;
			//Logger::debug("B");
		}
		clampedVal = brakeMin;
	}

	if (!(brakeStatus == OK)) {
		level = 0; //no throttle if there is a fault
		return;
	}
	calcBrake1 = calcBrake(clampedVal, brakeMin, brakeMax);
	//Logger::debug("calcBrake: %d", calcBrake1);

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

	if (brakeMaxRegen != 0) { //is the brake regen even enabled?
		int range = brakeMaxRegen - throttleMaxRegen; //we start the brake at ThrottleMaxRegen so the range is reduced by that amount
		//Logger::debug("range: %d", range);
		//Logger::debug("brakeFeedback: %d", brakeFeedback);
		if (range < 1) { //detect stupidity and abort
			level = 0;
			return;
		}
		level = (signed int) ((signed int) -10 * range * brakeFeedback) / (signed int) 1000;
		level -= 10 * throttleMaxRegen;
		//Logger::debug("level: %d", level);
	}

}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void PotBrake::handleTick() {
	sys_io_adc_poll();

	brake1Val = getAnalog(brake1ADC);
	if (numBrakePots > 1) {
		brake2Val = getAnalog(brake2ADC);
	}

	// Call parent handleTick
	Throttle::handleTick();

	brakeStatus = OK;
	doBrake();
}

PotBrake::BrakeStatus PotBrake::getStatus() {
	return brakeStatus;
}

Device::DeviceId PotBrake::getId() {
	return (POTBRAKEPEDAL);
}

Device::DeviceType PotBrake::getType() {
	return (DEVICE_BRAKE);
}

void PotBrake::saveEEPROM() {
	prefsHandler->write(EETH_BRAKE_MIN, brakeMin);
	prefsHandler->write(EETH_BRAKE_MAX, brakeMax);
	prefsHandler->write(EETH_MAX_BRAKE_REGEN, brakeMaxRegen);
	prefsHandler->saveChecksum();
}

void PotBrake::saveConfiguration() {
  Logger::info("Saving brake settings");
  TickHandler::getInstance()->detach(this); // unregister from TickHandler first
  setT1Min(throttleDetector->getThrottle1Min());
  setT1Max(throttleDetector->getThrottle1Max());
  saveEEPROM();  
  TickHandler::getInstance()->attach(this, getTickInterval());
}

void PotBrake::setT1Min(uint16_t minVal) 
{
	brakeMin = minVal;
}

void PotBrake::setT1Max(uint16_t maxVal) 
{
	brakeMax = maxVal;
}


#endif // CFG_ENABLE_DEVICE_POT_BRAKE
