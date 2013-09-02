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
	int getRawBrake1();
	int getRawBrake2();
	int getRawThrottle1();
	int getRawThrottle2();
	Device::DeviceId getId();
	Device::DeviceType getType();
	void saveConfiguration();
	void saveEEPROM();
	void setT1Min(uint16_t min);
	void setT1Max(uint16_t max);
	void setMaxRegen(uint16_t regen);



private:
	uint16_t brakeMin, brakeMax;
	uint16_t brake1Val, brake2Val;
	uint8_t brake1ADC, brake2ADC; //which ADC pin each are on
	int numBrakePots; //whether there are one or two pots. Should support three as well since some pedals really do have that many
	uint16_t throttleMaxRegen; //TODO: should not be used in here anymore - maybe outside ?
	uint16_t brakeMaxRegen; //percentage of max torque allowable for regen at brake pedal
	byte brakeMaxErr;
	BrakeStatus brakeStatus;
	int calcBrake(int, int, int);
	void doBrake();
};

#endif /* POT_BRAKE_H_ */
