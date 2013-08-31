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
#include "Logger.h"


/*
 * The constructor takes a pointer to a throttle
 */
ThrottleDetector::ThrottleDetector(Throttle *throttle) {
	this->throttle = throttle;
	state = DoNothing;
	maxThrottleReadingDeviationPercent = 15;
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
	case DetectBothMinWait:
		detectBothMinWait();
		break;
	case DetectBothMinCalibrate:
		detectBothMinCalibrate();
		break;
	case DetectBothMaxWait:
		detectBothMaxWait();
		break;
	case DetectBothMaxCalibrate:
		detectBothMaxCalibrate();
		break;
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

	// we wait for 3 seconds so kick this off
	startTime = millis();
	state = DetectBothMinWait;
}

/*
 * Step 2. Wait for 3 seconds then start taking MIN readings
 */
void ThrottleDetector::detectBothMinWait() {
	if ((millis() - startTime) >= 3000) {
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

		// take MIN readings for 3 seconds
		startTime = millis();
		readThrottleValues();
		state = DetectBothMinCalibrate;
	}
}

/*
 * Step 3. Take MIN readings for 3 seconds then start waiting again
 */
void ThrottleDetector::detectBothMinCalibrate() {
	if ((millis() - startTime) < 3000) {
		readThrottleValues();
	} else {
		displayCalibratedValues(true);

		// save rest minimums
		throttle1MinRest = throttle1Min;  // minimum sensor value at rest
		throttle2MinRest = throttle2Min;  // minimum sensor value at rest
		throttle2MaxRest = throttle2Max;  // maximum sensor value at rest

		SerialUSB.println("");
		SerialUSB.println("Fully depress and hold the pedal until complete");

		// wait for 3 seconds so they can react
		startTime = millis();
		state = DetectBothMaxWait;
	}

}

/*
 * Step 4. Wait for 3 seconds then start taking MAX readings
 */
void ThrottleDetector::detectBothMaxWait() {
	if ((millis() - startTime) >= 3000) {
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

		// take MAX readings for 3 seconds
		startTime = millis();
		readThrottleValues();
		state = DetectBothMaxCalibrate;
	}
}

/*
 * Step 5. Take MAX readings for 3 seconds then show results
 */
void ThrottleDetector::detectBothMaxCalibrate() {
	if ((millis() - startTime) < 3000) {
		readThrottleValues();
	} else {
		displayCalibratedValues(false);

		// Determine throttle type
		if (throttle1MinRest > throttle1Min) { // high to low pot
			throttle1HighLow = true;
		}

		if (throttle2MinRest > throttle2Min) { // high to low pot
			throttle2HighLow = true;
		}

		// restore the true min
		throttle1Min = throttle1MinRest;

		if (throttle2Provided()) {

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
		}

		SerialUSB.println("");
		SerialUSB.println("=======================================");
		SerialUSB.println("Detection complete");
		SerialUSB.print("Num potentiometers found: ");
		SerialUSB.println(getPotentiometerCount());
		SerialUSB.print("T1: ");
		SerialUSB.print(getThrottle1Min(), DEC);
		SerialUSB.print(" to ");
		SerialUSB.print(getThrottle1Max(), DEC);
		SerialUSB.println(isThrottle1HighLow() ? " HIGH-LOW" : " LOW-HIGH");

		if (getPotentiometerCount() > 1) {
			SerialUSB.print("T2: ");
			SerialUSB.print(isThrottle2Inverse() ? getThrottle2Max() : getThrottle2Min(), DEC);
			SerialUSB.print(" to ");
			SerialUSB.print(isThrottle2Inverse() ? getThrottle2Min() : getThrottle2Max(), DEC);
			SerialUSB.print(isThrottle2HighLow() ? " HIGH-LOW" : " LOW-HIGH");
			SerialUSB.println(isThrottle2Inverse() ? " (Inverse of T1)" : "");
		}

		SerialUSB.println("========================================");

		// Done!
		state = DoNothing;
	}
}


/*
 * Run the MIN throttle detection.
 * Step 1. Kick it off
 */
void ThrottleDetector::detectMin() {
	SerialUSB.println("Throttle MIN detection starting. Do NOT press the pedal.");

	resetValues();

	// we wait for 3 seconds so kick this off
	startTime = millis();
	state = DetectMinWait;
}

/*
 * Step 2. Wait for 3 seconds then start taking MIN readings
 */
void ThrottleDetector::detectMinWait() {
	if ((millis() - startTime) >= 3000) {
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

		// take readings for 3 seconds
		startTime = millis();
		readThrottleValues();
		state = DetectMinCalibrate;
	}
}

/*
 * Step 3. Take MIN readings for 3 seconds then show results
 */
void ThrottleDetector::detectMinCalibrate() {
	if ((millis() - startTime) < 3000) {
		readThrottleValues();
	} else {
		displayCalibratedValues(true);

		if (throttle2Provided()) {
			// Detect grounded pin (always zero) or floating values which indicate no potentiometer provided
			// If the values deviate by more than 15% we assume floating
			int restDiff = abs(throttle2Max-throttle2Min) * 100 / throttle2Max;
			if ((throttle2Min == INT16_MAX && throttle2Max == 0) || restDiff > maxThrottleReadingDeviationPercent) {
				potentiometerCount = 1;
			} else {
				potentiometerCount = 2;
			}
		}

		SerialUSB.println("");
		SerialUSB.println("=======================================");
		SerialUSB.println("MIN Detection complete");
		SerialUSB.print("Num potentiometers found: ");
		SerialUSB.println(getPotentiometerCount());
		SerialUSB.print("T1: ");
		SerialUSB.print(getThrottle1Min(), DEC);
		SerialUSB.print(" to ");
		SerialUSB.print(getThrottle1Max(), DEC);
		SerialUSB.print(", using MIN: ");
		SerialUSB.println(getThrottle1Min(), DEC);

		if (getPotentiometerCount() > 1) {
			SerialUSB.print("T2: ");
			SerialUSB.print(getThrottle2Min(), DEC);
			SerialUSB.print(" to ");
			SerialUSB.print(getThrottle2Max(), DEC);
			SerialUSB.print(", using MIN: ");
			SerialUSB.println(getThrottle2Min(), DEC);
		}

		SerialUSB.println("========================================");

		// Done!
		state = DoNothing;
	}
}


/*
 * Run the MAX throttle detection.
 * Step 1. Kick it off
 */
void ThrottleDetector::detectMax() {
	SerialUSB.println("Throttle MAX detection starting. Fully depress and hold the pedal until complete.");

	resetValues();

	// we wait for 3 seconds so kick this off
	startTime = millis();
	state = DetectMaxWait;
}

/*
 * Step 2. Wait for 3 seconds then start taking MAX readings
 */
void ThrottleDetector::detectMaxWait() {
	if ((millis() - startTime) >= 3000) {
		// drop initial readings as they seem to be invalid
		readThrottleValues();
		resetValues();

		// take readings for 3 seconds
		startTime = millis();
		readThrottleValues();
		state = DetectMaxCalibrate;
	}
}

/*
 * Step 3. Take MAX readings for 3 seconds then show results
 */
void ThrottleDetector::detectMaxCalibrate() {
	if ((millis() - startTime) < 3000) {
		readThrottleValues();
	} else {
		displayCalibratedValues(false);

		if (throttle2Provided()) {
			// Detect grounded pin (always zero) or floating values which indicate no potentiometer provided
			// If the values deviate by more than 15% we assume floating
			int maxDiff = abs(throttle2Max-throttle2Min) * 100 / throttle2Max;
			if ((throttle2Min == INT16_MAX && throttle2Max == 0) || maxDiff > maxThrottleReadingDeviationPercent) {
				potentiometerCount = 1;
			} else {
				potentiometerCount = 2;
			}
		}

		SerialUSB.println("");
		SerialUSB.println("=======================================");
		SerialUSB.println("MAX Detection complete");
		SerialUSB.print("Num potentiometers found: ");
		SerialUSB.println(getPotentiometerCount());
		SerialUSB.print("T1: ");
		SerialUSB.print(getThrottle1Min(), DEC);
		SerialUSB.print(" to ");
		SerialUSB.print(getThrottle1Max(), DEC);
		SerialUSB.print(", using MAX: ");
		SerialUSB.println(getThrottle1Max(), DEC);

		if (getPotentiometerCount() > 1) {
			SerialUSB.print("T2: ");
			SerialUSB.print(getThrottle2Min(), DEC);
			SerialUSB.print(" to ");
			SerialUSB.print(getThrottle2Max(), DEC);
			SerialUSB.print(", using MAX: ");
			SerialUSB.println(getThrottle2Max(), DEC);
		}

		SerialUSB.println("========================================");

		// Done!
		state = DoNothing;
	}
}

/*
 * Returns the number of potentiometers detected
 */
int ThrottleDetector::getPotentiometerCount() {
	return potentiometerCount;
}

/*
 * Returns true if throttle1 ranges from highest to lowest value
 * as the pedal is pressed
 */
bool ThrottleDetector::isThrottle1HighLow() {
	return throttle1HighLow;
}

/*
 * Returns true if throttle2 ranges from highest to lowest value
 * as the pedal is pressed
 */
bool ThrottleDetector::isThrottle2HighLow() {
	return throttle2HighLow;
}

/*
 * Returns the minimum value of throttle1
 */
uint16_t ThrottleDetector::getThrottle1Min() {
	return throttle1Min;
}

/*
 * Returns the maximum value of throttle1
 */
uint16_t ThrottleDetector::getThrottle1Max() {
	return throttle1Max;
}

/*
 * Returns the minimum value of throttle2
 */
uint16_t ThrottleDetector::getThrottle2Min() {
	return throttle2Min;
}

/*
 * Returns the maximum value of throttle2
 */
uint16_t ThrottleDetector::getThrottle2Max() {
	return throttle2Max;
}

/*
 * Returns true if throttle2 values are the opposite of the
 * throttle1 values. For example, if throttle1 is low-high and 
 * throttle2 is high-low.
 */
bool ThrottleDetector::isThrottle2Inverse() {
	return throttle2Inverse;
}

/*
 * Returns true if a second throttle was provided
 */
bool ThrottleDetector::throttle2Provided() {
	return true;
}

/*
 * Reset/initialize some values
 */
void ThrottleDetector::resetValues() {
	throttle1Value = 0;
	throttle1Min = INT16_MAX;
	throttle1Max = 0;
	throttle2Value = 0;
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
	throttle1Value = throttle->getRawThrottle1();
	if (throttle2Provided()) {
		throttle2Value = throttle->getRawThrottle2();
	}

	// record the minimum sensor value
	if (throttle1Value < throttle1Min) {
		throttle1Min = throttle1Value;
	}

	// record the maximum sensor value
	if (throttle1Value > throttle1Max) {
		throttle1Max = throttle1Value;
	}

	if (throttle2Provided()) {
		// record the minimum sensor value
		if (throttle2Value < throttle2Min) {
			throttle2Min = throttle2Value;
		}

		// record the maximum sensor value
		if (throttle2Value > throttle2Max) {
			throttle2Max = throttle2Value;
		}
	}
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

