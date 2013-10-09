/*
 * PotBrake.h
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

#ifndef POT_BRAKE_H_
#define POT_BRAKE_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "sys_io.h"
#include "TickHandler.h"
#include "Logger.h"
#include "ThrottleDetector.h"

#define THROTTLE_INPUT_BRAKELIGHT  2

class PotBrake: public Throttle {
public:
	enum BrakeStatus {
		OK,
		ERR_LOW_T1,
		ERR_LOW_T2,
		ERR_HIGH_T1,
		ERR_HIGH_T2,
		ERR_MISMATCH,
		ERR_MISC
	};

	PotBrake(uint8_t throttle1, uint8_t throttle2);
	void setup();
	void handleTick();
	BrakeStatus getStatus();
	uint16_t getRawThrottle1();
	uint16_t getRawThrottle2();
	DeviceId getId();
	DeviceType getType();
	void saveConfiguration();
	void saveEEPROM();

private:
	uint8_t brake1ADC, brake2ADC; //which ADC pin each are on
	BrakeStatus brakeStatus;
	uint16_t calcBrake(uint16_t, uint16_t, uint16_t);
	void doBrake();
};

#endif /* POT_BRAKE_H_ */
