/*
 * ThrottleDetector.h
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

#ifndef THROTTLE_DETECTOR_H_
#define THROTTLE_DETECTOR_H_

#include "PotThrottle.h"
#include "Logger.h"
#include "DeviceManager.h"

class Throttle;

class ThrottleDetector : public TickObserver {

public:
	ThrottleDetector(Throttle *throttle);
	~ThrottleDetector();
	void handleTick();
	void detect();

private:
	enum DetectionState {
		DoNothing,
		DetectMinWait,
		DetectMinCalibrate,
		DetectMaxWait,
		DetectMaxCalibrate
	};

	void detectMinWait();
	void detectMinCalibrate();
	void detectMaxWait();
	void detectMaxCalibrate();
	void displayCalibratedValues(bool minPedal);
	void resetValues();
	void readThrottleValues();
	int checkLinear(uint16_t, uint16_t);
	int checkInverse(uint16_t, uint16_t);
	uint16_t normalize(uint16_t sensorValue, uint16_t sensorMin, uint16_t sensorMax, uint16_t constrainMin, uint16_t constrainMax);

	Throttle *throttle;
	PotThrottleConfiguration *config;
	DetectionState state;
	unsigned long startTime;
	int potentiometerCount; // the number of potentiometers detected
	uint16_t throttle1Min; // the minimum value of throttle1
	uint16_t throttle1Max; // the maximum value of throttle1
	uint16_t throttle2Min; // the minimum value of throttle2
	uint16_t throttle2Max; // the maximum value of throttle2
	uint8_t throttleSubType; // the throttle sub type
	bool throttle1HighLow; // true if throttle1 ranges from highest to lowest value as the pedal is pressed
	bool throttle2HighLow; // true if throttle2 ranges from highest to lowest value as the pedal is pressed
	bool throttle2Inverse; // true if throttle2 values are the opposite of the throttle1 values.
	int throttle1MinRest;  // minimum sensor value at rest
	int throttle1MaxRest;  // minimum sensor value at rest
	int throttle2MinRest;  // minimum sensor value at rest
	int throttle2MaxRest;  // maximum sensor value at rest
	int throttle1MinFluctuationPercent;
	int throttle1MaxFluctuationPercent;
	int throttle2MinFluctuationPercent;
	int throttle2MaxFluctuationPercent;
	int maxThrottleReadingDeviationPercent;
	// stats/counters when sampling
	static const int maxSamples = 300;
	int sampleCount;
	int linearCount;
	int inverseCount;
	uint16_t throttle1Values[maxSamples];
	uint16_t throttle2Values[maxSamples];

};

#endif /* THROTTLE_DETECTOR_H_ */

