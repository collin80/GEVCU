/*
 * pedal_pot.h
 *
 * Created: 1/13/2013 7:08:12 PM
 *  Author: Collin Kidder
 */

#ifndef PEDAL_POT_H_
#define PEDAL_POT_H_

#include <Arduino.h>
#include "config.h"
#include "throttle.h"
#include "sys_io.h"
#include "FaultHandler.h"

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
	void setupDevice();
	ThrottleStatus getStatus();
	void setT1Min(uint16_t min);
	void setT2Min(uint16_t min);
	void setT1Max(uint16_t max);
	void setT2Max(uint16_t max);
	void setRegenEnd(uint16_t regen);
	void setFWDStart(uint16_t fwd);
	void setMAP(uint16_t map);
	void setMaxRegen(uint16_t regen);
	int getRawThrottle1();
	int getRawThrottle2();
	PotThrottle(uint8_t throttle1, uint8_t throttle2, bool isAccel);
	Device::DeviceId getDeviceID();

private:
	uint16_t throttleMin1, throttleMax1, throttleMin2, throttleMax2; //Values for when the pedal is at its min and max for each throttle input
	uint16_t brakeMin, brakeMax;
	uint16_t throttle1Val, throttle2Val;
	uint8_t throttle1ADC, throttle2ADC; //which ADC pin each are on
	int numThrottlePots; //whether there are one or two pots. Should support three as well since some pedals really do have that many
	uint16_t throttleRegen, throttleFwd, throttleMap; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	uint16_t throttleMaxRegen; //Percentage of max torque allowable for regen
	uint16_t brakeMaxRegen; //percentage of max torque allowable for regen at brake pedal
	byte throttleMaxErr;
	bool isAccelerator; //is this throttle for an accelerator or a brake? defaults to accelerator
	ThrottleStatus throttleStatus;
	int calcThrottle(int, int, int);
	void doAccel();
	void doBrake();
};

#endif /* PEDAL_POT_H_ */
