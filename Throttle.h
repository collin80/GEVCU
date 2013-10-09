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
	virtual int16_t getLevel();
	virtual uint16_t getRawThrottle1();
	virtual uint16_t getRawThrottle2();

	bool isFaulted();
	void detectThrottle();
	void detectThrottleMin();
	void detectThrottleMax();

	virtual void mapThrottle(int16_t);
	virtual void saveConfiguration();
	virtual void saveEEPROM();

	virtual void setPositionRegenStart(uint16_t regen);
	virtual uint16_t getPositionRegenStart();
	virtual void setPositionForwardMotionStart(uint16_t fwd);
	virtual uint16_t getPositionForwardMotionStart();
	virtual void setPositionHalfPower(uint16_t map);
	virtual uint16_t getPositionHalfPower();
	virtual void setMaximumRegen(uint16_t regen);
	virtual uint16_t getMaximumRegen();
	virtual void setMinimumRegen(uint16_t regen);
	virtual uint16_t getMinimumRegen();
    virtual void setMinumumLevel1(uint16_t min);
    virtual uint16_t getMinimumLevel1();
	virtual void setMinimumLevel2(uint16_t min);
	virtual uint16_t getMinimumLevel2();
	virtual void setMaximumLevel1(uint16_t max);
	virtual uint16_t getMaximumLevel1();
	virtual void setMaximumLevel2(uint16_t max);
	virtual uint16_t getMaximumLevel2();
	virtual void setNumberPotMeters(uint8_t num);
	virtual uint8_t getNumberPotMeters();
	virtual void setSubtype(uint8_t num);
	virtual uint8_t getSubtype();

protected:
	int16_t level; // the final signed throttle level. [-1000, 1000] in permille of maximum
	uint16_t positionRegenStart, positionForwardMotionStart, positionHalfPower; // value at which regen starts, forward motion starts, and the mid point of throttle
	uint8_t maximumRegen; // percentage of max torque allowable for regen
	uint8_t minimumRegen; // percentage of min torque allowable for regen
    uint16_t minimumLevel1, maximumLevel1, minimumLevel2, maximumLevel2; // values for when the pedal is at its min and max for each input
	uint16_t rawLevel1, rawLevel2; // the raw level of the input potentiometers
	uint8_t numberPotMeters; // the number of potentiometers to be used. Should support three as well since some pedals really do have that many

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
