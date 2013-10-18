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
	maxThrottleReadingDeviationPercent = 50; // 5% in 0-1000 scale
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
	SerialUSB.println("Throttle detection starting. Do NOT press the pedal until instructed.");

	resetValues();

	// reset stats
	sampleCount = 0;
	linearCount = 0;
	inverseCount = 0;
	for (int i=0; i<200; i++) {
		throttle1Values[i]=0;
		throttle2Values[i]=0;
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
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

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
	if ((millis() - startTime) < 2000 || sampleCount < maxSamples/3 ) {
		readThrottleValues();
	} else {
		displayCalibratedValues(true);

		// save rest minimums
		throttle1MinRest = throttle1Min;  // minimum sensor value at rest
		throttle2MinRest = throttle2Min;  // minimum sensor value at rest
		throttle2MaxRest = throttle2Max;  // maximum sensor value at rest

		SerialUSB.println("");
		SerialUSB.println("Smoothly depress the pedal to full acceleration");
		SerialUSB.println("and hold the pedal until complete");

		// wait for 5 seconds so they can react and then still get some readings
		startTime = millis();
		state = DetectMaxWait;
	}
}

/*
 * Step 4. Wait for 5 seconds then start taking MAX readings
 */
void ThrottleDetector::detectMaxWait() {
	if ((millis() - startTime) >= 5000 || sampleCount >= maxSamples*2/3) {
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

		// take MAX readings for 2 seconds
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

		// Determine throttle type based off min/max
		if (throttle1MinRest > throttle1Min) { // high to low pot
			throttle1HighLow = true;
		}

		if (throttle2MinRest > throttle2Min) { // high to low pot
			throttle2HighLow = true;
		}

		// restore the true min
		throttle1Min = throttle1MinRest;

		if ((throttle1HighLow && !throttle2HighLow)
				|| (throttle2HighLow && !throttle1HighLow)) {
			throttle2Inverse = true;
		}

		// Detect grounded pin (always zero) or floating values which indicate no potentiometer provided
		// If the values deviate by more than 15% we assume floating
		int restDiff = abs(throttle2MaxRest-throttle2MinRest) * 100 / throttle2MaxRest;
		int maxDiff = abs(throttle2Max-throttle2Min) * 100 / throttle2Max;
		if ((throttle2MinRest == 0 && throttle2MaxRest == 0
				&& throttle2Min == INT16_MAX && throttle2Max == 0)
				|| (restDiff > maxThrottleReadingDeviationPercent
					&& maxDiff > maxThrottleReadingDeviationPercent)) {
			potentiometerCount = 1;
		} else {
			potentiometerCount = 2;
		}

		// restore the true min
		throttle2Min = throttle2MinRest;

		// Determine throttle subtype by examining the data sampled
		for (int i=0; i<sampleCount; i++) {
			// normalize the values to a 0-1000 scale using the found min/max
			uint16_t value1 = normalize(throttle1Values[i], throttle1Min, throttle1Max, 0, 1000);
			uint16_t value2 = normalize(throttle2Values[i],
									throttle2Inverse ? throttle2Max : throttle2Min,
									throttle2Inverse ? throttle2Min : throttle2Max,
									0,1000);

			// see if they match known subtypes
			linearCount += checkLinear(value1, value2);
			inverseCount += checkInverse(value1, value2);

			//SerialUSB.println("NT1: " + String(value1) + ", NT2: " + String(value2) + ", L: " + String(linearCount) + ", I: " + String(inverseCount));
		}

		throttleSubType = 0;
		if (potentiometerCount > 1) {
			// For dual pots, we trust the detection of >75%
			if ( linearCount/sampleCount*100 > 75 ) {
				throttleSubType = 1;
			} else if ( inverseCount/sampleCount*100 > 75 ) {
				throttleSubType =  2;
			}
		} else {
			// For single pots we use the high/low
			if (throttle1HighLow) {
				throttleSubType =  2;
			} else {
				throttleSubType = 1;
			}
		}

		SerialUSB.println("");
		SerialUSB.println("=======================================");
		SerialUSB.println("Detection complete");
		SerialUSB.print("Num samples taken: ");
		SerialUSB.println(sampleCount);
		SerialUSB.print("Num potentiometers found: ");
		SerialUSB.println(potentiometerCount);
		SerialUSB.print("T1: ");
		SerialUSB.print(throttle1Min, DEC);
		SerialUSB.print(" to ");
		SerialUSB.print(throttle1Max, DEC);
		SerialUSB.println(throttle1HighLow ? " HIGH-LOW" : " LOW-HIGH");

		if (potentiometerCount > 1) {
			SerialUSB.print("T2: ");
			SerialUSB.print(throttle2Inverse ? throttle2Max : throttle2Min, DEC);
			SerialUSB.print(" to ");
			SerialUSB.print(throttle2Inverse ? throttle2Min : throttle2Max, DEC);
			SerialUSB.print(throttle2HighLow ? " HIGH-LOW" : " LOW-HIGH");
			SerialUSB.println(throttle2Inverse ? " (Inverse of T1)" : "");
			SerialUSB.print("Num linear throttle matches: ");
			SerialUSB.println(linearCount);
			SerialUSB.print("Num inverse throttle matches: ");
			SerialUSB.println(inverseCount);
		}

		SerialUSB.print("Throttle type: ");
		if (throttleSubType == 0) {
			SerialUSB.println("UNKNOWN");
		} else if (throttleSubType == 1) {
			SerialUSB.println("Linear");
		} else if (throttleSubType == 2) {
			SerialUSB.println("Inverse");
		}

		/*
		SerialUSB.println();
		SerialUSB.println("----- RAW values ----");
		for (int i=0; i<sampleCount; i++) {
			SerialUSB.println("T1: " + String(throttle1Values[i]) + ", T2: " + String(throttle2Values[i]));
		}
		*/

		SerialUSB.println("========================================");

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

	if (throttle2Provided()) {
		// record the minimum sensor value
		if (rawSignal->input2 < throttle2Min) {
			throttle2Min = rawSignal->input2;
		}

		// record the maximum sensor value
		if (rawSignal->input2 > throttle2Max) {
			throttle2Max = rawSignal->input2;
		}
	}
}

/*
 * Compares two throttle readings and returns 1 if they are a mirror
 * of each other (within a tolerance) otherwise returns 0
 * Assumes the values are already mapped to a 0-1000 scale
 */
int ThrottleDetector::checkLinear(uint16_t throttle1Value, uint16_t throttle2Value) {
	int match = 0;

	if ( abs(throttle1Value-throttle2Value) < maxThrottleReadingDeviationPercent) {
		match = 1;
	}
	return match;
}

/*
 * Compares two throttle readings and returns 1 if they are the inverse
 * of each other (within a tolerance) otherwise returns 0
 * Assumes the values are already mapped to a 0-1000 scale
 */
int ThrottleDetector::checkInverse(uint16_t throttle1Value, uint16_t throttle2Value) {
	int match = 0;

	if (abs(1000 - (throttle1Value+throttle2Value)) <  maxThrottleReadingDeviationPercent) {
		match = 1;
	}

	return match;
}

void ThrottleDetector::displayCalibratedValues(bool minPedal) {
	SerialUSB.println("");
	SerialUSB.print("At ");
	if (minPedal) {
		SerialUSB.print("MIN");
	} else {
		SerialUSB.print("MAX");
	}
	SerialUSB.print(" T1: ");
	SerialUSB.print(throttle1Min, DEC);
	SerialUSB.print(" to ");
	SerialUSB.print(throttle1Max, DEC);
	if (throttle2Provided()) {
		SerialUSB.print(" T2: ");
		SerialUSB.print(throttle2Min, DEC);
		SerialUSB.print(" to ");
		SerialUSB.print(throttle2Max, DEC);
	}
	SerialUSB.println("");
}

/**
 * Map and constrain the value to the given range
 */
uint16_t ThrottleDetector::normalize(uint16_t sensorValue, uint16_t sensorMin, uint16_t sensorMax, uint16_t constrainMin, uint16_t constrainMax) {
  int value = map(sensorValue, sensorMin, sensorMax, constrainMin, constrainMax);
  return constrain(value, constrainMin, constrainMax);
}
