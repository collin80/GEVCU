/*
 * Throttle.h
 *
 * Parent class for all throttle controllers, be they canbus or pot or hall effect, etc
 * Though, actually right now it can't be canbus. There are no plans to support canbus throttles at the moment
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

class ThrottleDetector; // avoid circular dependency by declaring it here

class Throttle: public Device {
public:
	Throttle();
	~Throttle();
	Device::DeviceType getType();
	virtual int getLevel();
	virtual int getRawThrottle1();
	virtual int getRawThrottle2();
        void detectThrottle();
        void detectThrottleMin();
        void detectThrottleMax();
        void saveConfiguration();
	void mapThrottle(signed int);
	void setRegenEnd(uint16_t regen);
	void setFWDStart(uint16_t fwd);
	void setMAP(uint16_t map);
	void setMaxRegen(uint16_t regen);


protected:
	signed int level; //the final signed throttle level. [-1000, 1000] in tenths of a percent of maximum
	uint16_t throttleRegen, throttleFwd, throttleMap; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	uint16_t throttleMaxRegen; //Percentage of max torque allowable for regen
	uint16_t brakeMaxRegen; //percentage of max torque allowable for regen at brake pedal

private:
        ThrottleDetector *throttleDetector;
};

#endif
