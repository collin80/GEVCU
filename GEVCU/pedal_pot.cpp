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
		
		ThrottleMin1 = ThrottleMin2 = 0;
		ThrottleMax1 = ThrottleMax2 = 1023;
		ThrottleAvg = ThrottleFeedback = 0;
		ThrottleRegen = 300;
		ThrottleFWD = 500;
		ThrottleMAP = 400;
		ThrottleMaxRegen = 30; //30%
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range. 
void THROTTLE::handleTick() {
	long comparitor;
	
	Throttle1Val = analogRead(Throttle1ADC);
	if (numThrottlePots > 1) {
		Throttle2Val = analogRead(Throttle2ADC);
	}		
	uint16_t ThrottleAvg, ThrottleFeedback;
	
	//in preparation for checking the two throttles against each other.
	//its going to be sort of complicated by the fact that the two throttles
	//very well might move in opposing directions (low->high or high->low) as
	//pedal is depressed. For now it is assumed throttle1 is working fine and that
	//it runs low to high as pedal is depressed.
	//comparitor = (1000 * Throttle1Val) / Throttle2Val;
	
	
	
    ThrottleAvg += Throttle1Val;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;
	
    if (ThrottleFeedback > ThrottleMax1) // clamp it to allow some dead zone.
		ThrottleFeedback = ThrottleMax1;
    else if (ThrottleFeedback < ThrottleMin1)
		ThrottleFeedback = ThrottleMin1;
		
	//Now ThrottleFeedback is between Min and Max. Time to figure out where in the throttle position that is and set
	//outputThrottle accordingly.
	
	ThrottleFeedback = 0; //by default we give zero throttle
	if (ThrottleFeedback <= ThrottleRegen) {
		//-1000 is max throttle regen * (ThrottleMaxRegen / 100) * (How much we're over the minimum divided by the total range)
		//the operation is cast to long because it could easily be well over 16 bits
		outputThrottle = -10 * ThrottleMaxRegen  * (long)(ThrottleFeedback - ThrottleMin1) / (long)(ThrottleRegen - ThrottleMin1);
	}
	else if ((ThrottleFeedback >= ThrottleFWD) && (ThrottleFeedback <= ThrottleMAP)) { //bottom 50% forward
		outputThrottle = 500 * (long)(ThrottleFeedback - ThrottleFWD) / (long)(ThrottleMAP - ThrottleFWD);
	}
	else { //more than ThrottleMAP
		outputThrottle = 500 + 500 * (long)(ThrottleFeedback - ThrottleMAP) / (long)(ThrottleMax1 - ThrottleMAP);
	}
	
}

int THROTTLE::getThrottle() {
	return outputThrottle;
}