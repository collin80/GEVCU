/*
 * Throttle.h
 *
 * Parent class for all throttle controllers, be they canbus or pot or hall effect, etc
 * Though, actually right now it can't be canbus. There are no plans to support canbus throttles at the moment
 * Created: 2/05/2013
 *  Author: Collin Kidder
 */

#ifndef THROTTLE_H_
#define THROTTLE_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"

class Throttle: public Device {
public:
	Throttle();
	Throttle(CanHandler *canHandler);
        ~Throttle();
	Device::DeviceType getType();
	virtual int getThrottle();
	virtual int getRawThrottle1();
	virtual int getRawThrottle2();
	void mapThrottle(signed int);
	void setRegenEnd(uint16_t regen);
	void setFWDStart(uint16_t fwd);
	void setMAP(uint16_t map);
	void setMaxRegen(uint16_t regen);


protected:
	signed int outputThrottle; //the final signed throttle. [-1000, 1000] in tenths of a percent of maximum
	uint16_t throttleRegen, throttleFwd, throttleMap; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	uint16_t throttleMaxRegen; //Percentage of max torque allowable for regen
	uint16_t brakeMaxRegen; //percentage of max torque allowable for regen at brake pedal
};

#endif
