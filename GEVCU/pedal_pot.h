/*
 * pedal_pot.h
 *
 * Created: 1/13/2013 7:08:12 PM
 *  Author: Collin
 */ 

#ifndef PEDAL_POT_H_
#define PEDAL_POT_H_

class THROTTLE {
   private:
	uint16_t ThrottleMin1, ThrottleMax1, ThrottleMin2, ThrottleMax2; //Values for when the pedal is at its min and max for each throttle input
	uint16_t Throttle1Val, Throttle2Val;
	uint16_t ThrottleAvg, ThrottleFeedback;
	uint8_t Throttle1ADC, Throttle2ADC; //which ADC pin each are on
	int numThrottlePots; //whether there are one or two pots. Should support three as well since some pedals really do have that many
	uint16_t ThrottleRegen, ThrottleFWD, ThrottleMAP; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	unsigned int ThrottleMaxRegen; //Percentage of max torque allowable for regen
	
  public:
	void handleTick();
	
};

#endif /* PEDAL_POT_H_ */