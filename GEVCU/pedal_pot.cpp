/*
 * pedal_pot.c
 *
 * Routines to handle input of raw ADC values from pedal pot and turn that into an output
 * the output is 0-255 with 0 being extreme regen, 127 being no throttle, and 255 full throttle
 *
 * Created: 1/13/2013 6:10:24 PM
 *  Author: Collin
 */

#include "pedal_pot.h"

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
THROTTLE::THROTTLE(uint8_t Throttle1, uint8_t Throttle2) {
	Throttle1ADC = Throttle1;
	Throttle2ADC = Throttle2;
	if (Throttle2 == 255) numThrottlePots = 1;
		else numThrottlePots = 2;

		ThrottleMin1 = ThrottleMin2 = 0;
		ThrottleMax1 = ThrottleMax2 = 1023;
		ThrottleRegen = 300;
		ThrottleFWD = 500;
		ThrottleMAP = 400;
		ThrottleMaxRegen = 30; //30%
		ThrottleValid = false;
		ThrottleMaxErr = 25; //in tenths of a percent. So 25 = max 2.5% difference
}

int THROTTLE::calcThrottle1() {
    int range = ThrottleMax1 - ThrottleMin1;
    int val = Throttle1Val - ThrottleMin1;
    int retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
    return retVal;
}

int THROTTLE::calcThrottle2() {
    int range = ThrottleMax2 - ThrottleMin2;
    int val = Throttle2Val - ThrottleMin2;
    int retVal = (int)(((long)val * 1000) / (long)range);
    return retVal;
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void THROTTLE::handleTick() {
   	signed int range;
	signed int temp, temp2;
	static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

	Throttle1Val = analogRead(Throttle1ADC);
	if (numThrottlePots > 1) {
		Throttle2Val = analogRead(Throttle2ADC);
	}

	//in preparation for checking the two throttles against each other.
	//its going to be sort of complicated by the fact that the two throttles
	//very well might move in opposing directions (low->high or high->low) as
	//pedal is depressed. For now it is assumed throttle1 is working fine and that
	//it runs low to high as pedal is depressed.
	//comparitor = (1000 * Throttle1Val) / Throttle2Val;

    ThrottleValid = true;
    if (Throttle1Val > ThrottleMax1) { // clamp it to allow some dead zone.
		Throttle1Val = ThrottleMax1;
		ThrottleValid = false; //got an error!
    }
    else if (Throttle1Val < ThrottleMin1) {
		Throttle1Val = ThrottleMin1;
		ThrottleValid = false;
    }

    temp = calcThrottle1();
    temp2 = calcThrottle2();
    if ((temp-ThrottleMaxErr) > temp2) { //then throttle1 is too large compared to 2
        ThrottleValid = false;
    }
    if ((temp2-ThrottleMaxErr) > temp) { //then throttle2 is too large compared to 1
        ThrottleValid = false;
    }

    if (!ThrottleValid) {
        outputThrottle = 0; //no throttle if there is a fault
        return;
    }

    //Apparently all is well with the throttle input
    //so go ahead and calculate the proper throttle output

    ThrottleAvg += Throttle1Val;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;

	outputThrottle = 0; //by default we give zero throttle
	if (ThrottleRegen != ThrottleMin1) {
        if ((ThrottleFeedback <= ThrottleRegen) && (ThrottleFeedback > (ThrottleMin1 + 5))) {  //give 5 deadzone at start of pedal so car freewheels at no pedal push
            range = ThrottleRegen - ThrottleMin1;
            temp = range - (ThrottleFeedback - ThrottleMin1);
            outputThrottle = (signed long)((signed long)(-10) * ThrottleMaxRegen  * temp / range);
        }
	}

	/*
       Danger Will Robinson! This code assumes that the pedal goes up in value as you press it
	   not all pedals will do this. The code must be fixed to handle reverse traveling pedals as well
	*/

	if (ThrottleFeedback >= ThrottleFWD) {
		if (ThrottleFeedback <= ThrottleMAP) { //bottom 50% forward
			range = ThrottleMAP - ThrottleFWD;
			temp = (ThrottleFeedback - ThrottleFWD);
			outputThrottle = (signed long)((signed long)(500) * temp / range);
		}
		else { //more than ThrottleMAP
			range = ThrottleMax1 - ThrottleMAP;
			temp = (ThrottleFeedback - ThrottleMAP);
			outputThrottle = 500 + (signed int)((signed long)(500) * temp / range);
		}
	}



	//Debugging output - Sends raw throttle1 and the actual output throttle value
	/*
	Serial.print(Throttle1Val);
	Serial.print("*");
	Serial.println(outputThrottle);
	*/

}

int THROTTLE::getThrottle() {
	return outputThrottle;
}

void THROTTLE::setT1Min(uint16_t min) {
	ThrottleMin1 = min;
}

void THROTTLE::setT2Min(uint16_t min) {
	ThrottleMin2 = min;
}

void THROTTLE::setT1Max(uint16_t max) {
	ThrottleMax1 = max;
}
void THROTTLE::setT2Max(uint16_t max) {
	ThrottleMax2 = max;
}

void THROTTLE::setRegenEnd(uint16_t regen) {
	ThrottleRegen = regen;
}

void THROTTLE::setFWDStart(uint16_t fwd) {
	ThrottleFWD = fwd;
}

void THROTTLE::setMAP(uint16_t map) {
	ThrottleMAP = map;
}

void THROTTLE::setMaxRegen(uint16_t regen) {
	ThrottleMaxRegen = regen;
}
