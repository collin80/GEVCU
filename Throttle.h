/*
 * Throttle.h
 *
 * Parent class for all throttle controllers, be they canbus or pot or hall effect, etc
 * This class is virtually totally virtual. The derived classes redefine most everything
 * about this class. It might even be a good idea to make the class totally abstract.


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

#ifndef THROTTLE_H_
#define THROTTLE_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"

/*
 * Data structure to hold raw signal(s) of the throttle.
 * E.g. for a three pot pedal, all signals could be used.
 */
struct RawSignalData {
	int32_t input1; // e.g. pot #1 or the signal from a can bus throttle
	int32_t input2; // e.g. pot #2 (optional)
	int32_t input3; // e.g. pot #3 (optional)
};

/*
 * A abstract class to hold throttle configuration parameters.
 * Can be extended by the subclass.
 */
class ThrottleConfiguration: public DeviceConfiguration {
public:
	uint16_t positionRegenMaximum, positionRegenMinimum; // throttle position where regen is highest and lowest
	uint16_t positionForwardMotionStart, positionHalfPower; // throttle position where forward motion starts and the mid point of throttle
	uint8_t maximumRegen; // percentage of max torque allowable for regen at maximum level
	uint8_t minimumRegen; // percentage of max torque allowable for regen at minimum level
	uint8_t creep; // percentage of torque used for creep function (imitate creep of automatic transmission, set 0 to disable)
};

/*
 * Abstract class for all throttle implementations.
 */
class Throttle: public Device {
public:
	enum ThrottleStatus {
		OK,
		ERR_LOW_T1,
		ERR_LOW_T2,
		ERR_HIGH_T1,
		ERR_HIGH_T2,
		ERR_MISMATCH,
		ERR_MISC
	};

	Throttle();
	virtual int16_t getLevel();
	void handleTick();
	virtual ThrottleStatus getStatus();
	virtual bool isFaulted();
	virtual DeviceType getType();

	virtual RawSignalData *acquireRawSignal();
	void loadConfiguration();
	void saveConfiguration();

protected:
	ThrottleStatus status;
	virtual bool validateSignal(RawSignalData *);
	virtual uint16_t calculatePedalPosition(RawSignalData *);
	virtual int16_t mapPedalPosition(int16_t);
	uint16_t normalizeAndConstrainInput(int32_t, int32_t, int32_t);
	int32_t normalizeInput(int32_t, int32_t, int32_t);

private:
	int16_t level; // the final signed throttle level. [-1000, 1000] in permille of maximum
};

#endif
