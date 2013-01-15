/*
 * pedal_pot.c
 *
 * Routines to handle input of raw ADC values from pedal pot and turn that into an output 
 * the output is 0-255 with 0 being extreme regen, 127 being no throttle, and 255 full throttle
 *
 * Created: 1/13/2013 6:10:24 PM
 *  Author: Collin
 */ 

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
THROTTLE:THROTTLE(uint8_t Throttle1, uint8_t Throttle2) {
	Throttle1ADC = Throttle1;
	Throttle2ADC = Throttle2;
	if (Throttle2 == 255) numThrottlePots = 1;
		else numThrottlePots = 2;
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range. 
THROTTLE:handleTick() {
	Throttle1Val = analogRead(Throttle1ADC);
	if (numThrottlePots > 1) {
		Throttle2Val = analogRead(Throttle2ADC);
	}		
	uint16_t ThrottleAvg, ThrottleFeedback;
	
    ThrottleAvg += Throttle1Val;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;
	
    if (ThrottleFeedback > Throttle1Max) // clamp it to allow some dead zone.
		ThrottleFeedback = Throttle1Max;
    else if (ThrottleFeedback < Throttle1Min)
		ThrottleFeedback = Throttle1Min;
		
	/*	
	uint16_t ThrottleRegen, ThrottleFWD, ThrottleMAP; //Value at which regen finishes, forward motion starts, and the mid point of throttle
	unsigned int ThrottleMaxRegen; //Percentage of max torque allowable for regen
	*/
}

