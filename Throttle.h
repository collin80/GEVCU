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
#include "ThrottleDetector.h"

class ThrottleDetector;
// avoid circular dependency by declaring it here

class Throttle: public Device {
public:
	Throttle();
	~Throttle();

	virtual DeviceType getType();

	virtual void handleTick();
	virtual int getLevel();
	virtual int getRawThrottle1();
	virtual int getRawThrottle2();

	void detectThrottle();
	void detectThrottleMin();
	void detectThrottleMax();

	virtual void mapThrottle(signed int);
	virtual void setRegenEnd(uint16_t regen);
	virtual void setFWDStart(uint16_t fwd);
	virtual void setMAP(uint16_t map);
	virtual void setMaxRegen(uint16_t regen);
	virtual void setMinRegen(uint16_t regen);
    virtual void setT1Min(uint16_t min);
	virtual void setT2Min(uint16_t min);
	virtual void setT1Max(uint16_t max);
	virtual void setT2Max(uint16_t max);
	void setNumThrottlePots(uint8_t num);
	uint8_t getNumThrottlePots();
	void setSubtype(uint8_t num);
	uint8_t getSubtype();


	virtual void saveConfiguration();
	virtual void saveEEPROM(); 

protected:
	signed int level; //the final signed throttle level. [-1000, 1000] in tenths of a percent of maximum
	uint16_t throttleRegen, throttleFwd, throttleMap; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	uint16_t throttleMaxRegen; //Percentage of max torque allowable for regen
	uint16_t brakeMaxRegen; //percentage of max torque allowable for regen at brake pedal
	uint16_t brakeMinRegen; //percentage of min torque allowable for regen at brake pedal
    uint16_t throttleMin1, throttleMax1, throttleMin2, throttleMax2; //Values for when the pedal is at its min and max for each throttle input
	uint16_t throttle1Val, throttle2Val;
	uint8_t numThrottlePots; //whether there are one or two pots. Should support three as well since some pedals really do have that many
	/*
	 * Allows subclasses to have sub types for their pedal type
	 * 0 - unknown type (prefs will return 0 if never set)
	 * 1 - standard linear potentiometer (low-high). If 2 pots, both are low-high and the 2nd mirrors the 1st.
	 * 2 - inverse potentiometer (high-low). If 2 pots, then 1st is low-high and 2nd is high-low)
	 */
	uint8_t throttleSubType;
	uint32_t getTickInterval();
	ThrottleDetector *throttleDetector;

private:
	
};

#endif
