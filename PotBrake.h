/*
 * PotBrake.h
 *
 *  Author: Collin Kidder
 */

#ifndef POT_BRAKE_H_
#define POT_BRAKE_H_

#include <Arduino.h>
#include "config.h"
#include "throttle.h"
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
	Device::DeviceId getId();
	Device::DeviceType getType();

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
