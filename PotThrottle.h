/*
 * PotThrottle.h
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

#ifndef PEDAL_POT_H_
#define PEDAL_POT_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "sys_io.h"
#include "TickHandler.h"
#include "ThrottleDetector.h"
#include "Logger.h"

#define THROTTLE_INPUT_BRAKELIGHT  2

class PotThrottle: public Throttle {
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

	void handleTick();
	void setup();
	bool isFaulted();
	ThrottleStatus getStatus();
	uint16_t getRawThrottle1();
	uint16_t getRawThrottle2();
	void saveEEPROM();
	void saveConfiguration();

	PotThrottle(uint8_t throttle1, uint8_t throttle2);
	DeviceId getId();

private:
	uint16_t throttleAverage, throttleFeedback; //used to create proportional control
	uint8_t throttle1ADC, throttle2ADC; //which ADC pin each are on
	byte throttleMaxErr;
	ThrottleStatus throttleStatus;

	uint16_t calcThrottle(uint16_t, uint16_t, uint16_t);
	void doAccel();
};

#endif /* POT_THROTTLE_H_ */
