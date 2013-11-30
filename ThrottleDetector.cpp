/*
 * ThrottleDetector.cpp
 *
 * This class can detect up to two potentiometers and determine their min/max values,
 * whether they read low to high or high to low, and if the second potentiometer is
 * the inverse of the first.
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

#include "ThrottleDetector.h"

/*
 * The constructor takes a pointer to a throttle
 */
ThrottleDetector::ThrottleDetector(Throttle *throttle) {
	this->throttle = throttle;
	config = (PotThrottleConfiguration *) throttle->getConfiguration();
	state = DoNothing;
	maxThrottleReadingDeviationPercent = 100; // 10% in 0-1000 scale
	Logger::debug("ThrottleDetector constructed with throttle %d", throttle);
	resetValues();
}

/*
 * Nothing to do in the destructor right now but good to have as a general rule
 */
ThrottleDetector::~ThrottleDetector() {
}

/*
 * Operations run asynchronously in small increments each time this
 * method is called so we track the state and resume the operation each call
 */
void ThrottleDetector::handleTick() {
	switch (state) {
	case DetectMinWait:
		detectMinWait();
		break;
	case DetectMinCalibrate:
		detectMinCalibrate();
		break;
	case DetectMaxWait:
		detectMaxWait();
		break;
	case DetectMaxCalibrate:
		detectMaxCalibrate();
		break;

	case DoNothing:
		break;
	}
}

/*
 * Run the complete throttle detection.
 * Step 1. Kick it off
 */
void ThrottleDetector::detect() {
	Logger::console("Throttle detection starting. Do NOT press the pedal until instructed.");

	resetValues();

	// reset stats
	sampleCount = 0;
	linearCount = 0;
	inverseCount = 0;
	for (int i = 0; i < 200; i++) {
		throttle1Values[i] = 0;
		throttle2Values[i] = 0;
	}

	// we wait for 2 seconds so kick this off
	startTime = millis();
	state = DetectMinWait;

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

/*
 * Step 2. Wait for 2 seconds then start taking MIN readings
 */
void ThrottleDetector::detectMinWait() {
	if ((millis() - startTime) >= 2000) {
		// take MIN readings for 2 seconds
		startTime = millis();
		readThrottleValues();
		state = DetectMinCalibrate;
	}
}

/*
 * Step 3. Take MIN readings for 2 seconds then start waiting again
 */
void ThrottleDetector::detectMinCalibrate() {
	if ((millis() - startTime) < 2000 || sampleCount < maxSamples / 3) {
		readThrottleValues();
	} else {
		displayCalibratedValues(true);

		// save rest minimums
		throttle1MinRest = throttle1Min;
		throttle1MaxRest = throttle1Max;
		throttle2MinRest = throttle2Min;
		throttle2MaxRest = throttle2Max;

		Logger::console("\nSmoothly depress the pedal to full acceleration");
		Logger::console("and hold the pedal until complete");

		// wait for 5 seconds so they can react and then still get some readings
		startTime = millis();
		state = DetectMaxWait;
	}
}

/*
 * Step 4. Wait for 5 seconds then start taking MAX readings
 */
void ThrottleDetector::detectMaxWait() {
	if ((millis() - startTime) >= 5000 || sampleCount >= maxSamples * 2 / 3) {

		// take MAX readings for 2 seconds
		resetValues();
		startTime = millis();
		readThrottleValues();
		state = DetectMaxCalibrate;
	} else {
		readThrottleValues();
	}
}

/*
 * Step 5. Take MAX readings for 3 seconds then show results
 */
void ThrottleDetector::detectMaxCalibrate() {
	if ((millis() - startTime) < 2000 && sampleCount < maxSamples) {
		readThrottleValues();
	} else {
		displayCalibratedValues(false);

		// take some stats before normalizing min/max
		int throttle1MinFluctuation = abs(throttle1MaxRest-throttle1MinRest);
		int throttle2MinFluctuation = abs(throttle2MaxRest-throttle2MinRest);
		int throttle1MaxFluctuation = abs(throttle1Max-throttle1Min);
		int throttle2MaxFluctuation = abs(throttle2Max-throttle2Min);

		// Determine throttle type based off min/max
		if (throttle1MinRest > throttle1Min+maxThrottleReadingDeviationPercent) { // high to low pot
			throttle1HighLow = true;
		}

		if (throttle2MinRest > throttle2Min+maxThrottleReadingDeviationPercent) { // high to low pot
			throttle2HighLow = true;
		}

		// restore the true min
		throttle1Min = throttle1MinRest;

		if ((throttle1HighLow && !throttle2HighLow) || (throttle2HighLow && !throttle1HighLow)) {
			throttle2Inverse = true;
		}

		// Detect grounded pin (always zero) or floating values which indicate no potentiometer provided
		if ((throttle2MinRest == 0 && throttle2MaxRest == 0 && throttle2Min == INT16_MAX && throttle2Max == 0)
				|| (abs(throttle2MaxRest-throttle2Max) < maxThrottleReadingDeviationPercent
						&& abs(throttle2MinRest-throttle2Min) < maxThrottleReadingDeviationPercent)) {
			potentiometerCount = 1;
		} else {
			potentiometerCount = 2;
		}

		// restore the true min/max for T2
		if ( throttle2Inverse ) {
			throttle2Max = throttle2MaxRest;
		} else {
			throttle2Min = throttle2MinRest;
		}

		Logger::debug("Inverse: %s, throttle2Min: %d, throttle2Max: %d", (throttle2Inverse?"true":"false"), throttle2Min, throttle2Max);

		// fluctuation percentages
		throttle1MinFluctuationPercent = throttle1MinFluctuation*100/abs(throttle1Max-throttle1Min);
		throttle1MaxFluctuationPercent = throttle1MaxFluctuation*100/abs(throttle1Max-throttle1Min);
		throttle2MinFluctuationPercent = throttle2MinFluctuation*100/abs(throttle2Max-throttle2Min);
		throttle2MaxFluctuationPercent = throttle2MaxFluctuation*100/abs(throttle2Max-throttle2Min);

		// Determine throttle subtype by examining the data sampled
		for (int i = 0; i < sampleCount; i++) {
			// normalize the values to a 0-1000 scale using the found min/max
			uint16_t value1 = normalize(throttle1Values[i], throttle1Min, throttle1Max, 0, 1000);
			uint16_t value2 = normalize(throttle2Values[i], throttle2Min, throttle2Max, 0, 1000);

			// see if they match known subtypes
			linearCount += checkLinear(value1, value2);
			inverseCount += checkInverse(value1, value2);

			//Logger::debug("T1: %d, T2: %d = NT1: %d, NT2: %d, L: %d, I: %d", throttle1Values[i], throttle2Values[i], value1, value2, linearCount, inverseCount);
		}

		throttleSubType = 0;
		if (potentiometerCount > 1) {
			// For dual pots, we trust the detection of >75%
			if ((linearCount * 100) / sampleCount > 75) {
				throttleSubType = 1;
			} else if ((inverseCount * 100) / sampleCount > 75) {
				throttleSubType = 2;
			}
		} else {
			// For single pots we use the high/low
			if (throttle1HighLow) {
				throttleSubType = 2;
			} else {
				throttleSubType = 1;
			}
		}

		char *type = "UNKNOWN";
		if (throttleSubType == 1) {
			type = "Linear";
		} else if (throttleSubType == 2) {
			type = "Inverse";
		}

		if ( Logger::isDebug()) {
			Logger::console("\n----- RAW values ----");
			for (int i = 0; i < sampleCount; i++) {
				Logger::console("T1: %d, T2: %d", throttle1Values[i], throttle2Values[i]);
			}
		}

		Logger::console("\n=======================================");
		Logger::console("Detection complete");
		Logger::console("Num samples taken: %d", sampleCount);
		Logger::console("Num potentiometers found: %d", potentiometerCount);
		Logger::console("T1: %d to %d %s", (throttle1HighLow ? throttle1Max : throttle1Min), (throttle1HighLow ? throttle1Min : throttle1Max),
				(throttle1HighLow ? "HIGH-LOW" : "LOW-HIGH"));
		Logger::console("T1: rest fluctuation %d%%, full throttle fluctuation %d%%", throttle1MinFluctuationPercent, throttle1MaxFluctuationPercent);

		if (potentiometerCount > 1) {
			Logger::console("T2: %d to %d %s %s", (throttle2HighLow ? throttle2Max : throttle2Min), (throttle2HighLow ? throttle2Min : throttle2Max),
					(throttle2HighLow ? "HIGH-LOW" : "LOW-HIGH"), (throttle2Inverse ? " (Inverse of T1)" : ""));
			Logger::console("T2: rest fluctuation %d%%, full throttle fluctuation %d%%", throttle2MinFluctuationPercent, throttle2MaxFluctuationPercent);
			Logger::console("Num linear throttle matches: %d", linearCount);
			Logger::console("Num inverse throttle matches: %d", inverseCount);
		}

		Logger::console("Throttle type: %s", type);
		Logger::console("========================================");

		// update the throttle's configuration (without storing it yet)
		config->minimumLevel1 = throttle1Min;
		config->maximumLevel1 = throttle1Max;
		config->numberPotMeters = potentiometerCount;
		if (config->numberPotMeters > 1) {
			config->minimumLevel2 = throttle2Min;
			config->maximumLevel2 = throttle2Max;
		} else {
			config->minimumLevel2 = 0;
			config->maximumLevel2 = 0;
		}
		config->throttleSubType = throttleSubType;

		// Done!
		state = DoNothing;
		TickHandler::getInstance()->detach(this);

		// send updates to ichip wifi
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);
	}
}

/*
 * Reset/initialize some values
 */
void ThrottleDetector::resetValues() {
	throttle1Min = INT16_MAX;
	throttle1Max = 0;
	throttle2Min = INT16_MAX;
	throttle2Max = 0;

	potentiometerCount = 1;
	throttle1HighLow = false;
	throttle2HighLow = false;
	throttle2Inverse = false;
}

/*
 * Reads values from the throttles.
 */
void ThrottleDetector::readThrottleValues() {
	RawSignalData *rawSignal = throttle->acquireRawSignal();
	throttle1Values[sampleCount] = rawSignal->input1;
	throttle2Values[sampleCount] = rawSignal->input2;
	sampleCount++;

	// record the minimum sensor value
	if (rawSignal->input1 < throttle1Min) {
		throttle1Min = rawSignal->input1;
	}

	// record the maximum sensor value
	if (rawSignal->input1 > throttle1Max) {
		throttle1Max = rawSignal->input1;
	}

	// record the minimum sensor value
	if (rawSignal->input2 < throttle2Min) {
		throttle2Min = rawSignal->input2;
	}

	// record the maximum sensor value
	if (rawSignal->input2 > throttle2Max) {
		throttle2Max = rawSignal->input2;
	}
}

/*
 * Compares two throttle readings and returns 1 if they are a mirror
 * of each other (within a tolerance) otherwise returns 0
 * Assumes the values are already mapped to a 0-1000 scale
 */
int ThrottleDetector::checkLinear(uint16_t throttle1Value, uint16_t throttle2Value) {
	if ( abs(throttle1Value-throttle2Value) < maxThrottleReadingDeviationPercent)
		return 1;

	return 0;
}

/*
 * Compares two throttle readings and returns 1 if they are the inverse
 * of each other (within a tolerance) otherwise returns 0
 * Assumes the values are already mapped to a 0-1000 scale
 */
int ThrottleDetector::checkInverse(uint16_t throttle1Value, uint16_t throttle2Value) {
	if (abs(1000 - (throttle1Value+throttle2Value)) < maxThrottleReadingDeviationPercent)
		return 1;

	return 0;
}

void ThrottleDetector::displayCalibratedValues(bool minPedal) {
	Logger::console("\nAt %s T1: %d to %d", (minPedal ? "MIN" : "MAX"), throttle1Min, throttle1Max);
	//if (potentiometerCount > 1)
		Logger::console(" T2: %d to %d", throttle2Min, throttle2Max);
	Logger::console("");
}

/**
 * Map and constrain the value to the given range
 */
uint16_t ThrottleDetector::normalize(uint16_t sensorValue, uint16_t sensorMin, uint16_t sensorMax, uint16_t constrainMin, uint16_t constrainMax) {
	int value = map(sensorValue, sensorMin, sensorMax, constrainMin, constrainMax);
	return constrain(value, constrainMin, constrainMax);
}
