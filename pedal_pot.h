/*
 * pedal_pot.h
 *
 * Created: 1/13/2013 7:08:12 PM
 *  Author: Collin Kidder
 */

#ifndef PEDAL_POT_H_
#define PEDAL_POT_H_

#include "Arduino.h"
#include "throttle.h"

class POT_THROTTLE : public THROTTLE {
    public:

        enum THROTTLESTATUS {
            OK,
            ERR_LOW_T1,
            ERR_LOW_T2,
            ERR_HIGH_T1,
            ERR_HIGH_T2,
            ERR_MISMATCH,
            ERR_MISC
        };


   private:
	uint16_t ThrottleMin1, ThrottleMax1, ThrottleMin2, ThrottleMax2; //Values for when the pedal is at its min and max for each throttle input
	uint16_t Throttle1Val, Throttle2Val;
	uint8_t Throttle1ADC, Throttle2ADC; //which ADC pin each are on
	int numThrottlePots; //whether there are one or two pots. Should support three as well since some pedals really do have that many
	uint16_t ThrottleRegen, ThrottleFWD, ThrottleMAP; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	uint16_t ThrottleMaxRegen; //Percentage of max torque allowable for regen
	byte ThrottleMaxErr;
	THROTTLESTATUS ThrottleStatus;	
        int calcThrottle(int, int, int);

  public:

	void handleTick();
        void setupDevice();
	THROTTLESTATUS getStatus();
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
	POT_THROTTLE(uint8_t Throttle1, uint8_t Throttle2);
	DEVICE::DEVID getDeviceID();

};

#endif /* PEDAL_POT_H_ */
