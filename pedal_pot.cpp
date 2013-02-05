/*
 * pedal_pot.c
 *
 * Turn raw ADC readings into [-1000|1000] throttle output. Smooths throttle output
 * and properly handles both positive and negative travel pots
 *
 * Created: 1/13/2013 6:10:24 PM
 *  Author: Collin Kidder
 */

#include "pedal_pot.h"

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
POT_THROTTLE::POT_THROTTLE(uint8_t Throttle1, uint8_t Throttle2) {
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
		ThrottleStatus = OK;
		ThrottleMaxErr = 25; //in tenths of a percent. So 25 = max 2.5% difference
}

int POT_THROTTLE::calcThrottle1() {
    int range, val, retVal;

    if (ThrottleMin1 < ThrottleMax1) { //low to high pot
        range = ThrottleMax1 - ThrottleMin1;
        val = Throttle1Val - ThrottleMin1;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
    }
    else { //high to low pot
        range = ThrottleMin1 - ThrottleMax1;
        val = Throttle1Val - ThrottleMax1;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
        retVal = 1000 - retVal; //reverses the value since the pedal runs reverse
    }

    return retVal;
}

int POT_THROTTLE::calcThrottle2() {
    int range, val, retVal;

    if (ThrottleMin2 < ThrottleMax2) { //low to high pot
        range = ThrottleMax2 - ThrottleMin2;
        val = Throttle2Val - ThrottleMin2;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
    }
    else { //high to low pot
        range = ThrottleMin2 - ThrottleMax2;
        val = Throttle2Val - ThrottleMax2;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
        retVal = 1000 - retVal; //reverses the value since the pedal runs reverse
    }

    return retVal;
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void POT_THROTTLE::handleTick() {
    signed int range;
    signed int temp, temp2;
    static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

    Throttle1Val = analogRead(Throttle1ADC);
    if (numThrottlePots > 1) {
      Throttle2Val = analogRead(Throttle2ADC);
    }

    ThrottleStatus = OK;
    if (Throttle1Val > ThrottleMax1) { // clamp it to allow some dead zone.
		Throttle1Val = ThrottleMax1;
		ThrottleStatus = ERR_HIGH_T1;
    }
    else if (Throttle1Val < ThrottleMin1) {
		Throttle1Val = ThrottleMin1;
		ThrottleStatus = ERR_LOW_T1;
    }

    temp = calcThrottle1();
    
    if (numThrottlePots > 1) { //can only do these things if there are two or more pots
      temp2 = calcThrottle2();
      if ((temp-ThrottleMaxErr) > temp2) { //then throttle1 is too large compared to 2
          ThrottleStatus = ERR_MISMATCH;
      }
      if ((temp2-ThrottleMaxErr) > temp) { //then throttle2 is too large compared to 1
          ThrottleStatus = ERR_MISMATCH;
      }

      temp = (temp + temp2) / 2; //temp now the average of the two
    }

    if (! (ThrottleStatus == OK)) {
        outputThrottle = 0; //no throttle if there is a fault
        return;
    }

    //Apparently all is well with the throttle input
    //so go ahead and calculate the proper throttle output

    ThrottleAvg += temp;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;

	outputThrottle = 0; //by default we give zero throttle

    /* Since this code is now based on tenths of a percent of throttle push it is now agnostic to how that happens
       positive or negative travel doesn't matter and is covered by the calcThrottle functions
    */

	if (ThrottleRegen != 0) {
        if ((ThrottleFeedback <= ThrottleRegen) && (ThrottleFeedback > 3)) {  //give 3 deadzone at start of pedal so car freewheels at no pedal push
            range = ThrottleRegen;
            temp = range - ThrottleFeedback;
            outputThrottle = (signed long)((signed long)(-10) * ThrottleMaxRegen  * temp / range);
        }
	}

	if (ThrottleFeedback >= ThrottleFWD) {
		if (ThrottleFeedback <= ThrottleMAP) { //bottom 50% forward
			range = ThrottleMAP - ThrottleFWD;
			temp = (ThrottleFeedback - ThrottleFWD);
			outputThrottle = (signed long)((signed long)(500) * temp / range);
		}
		else { //more than ThrottleMAP
			range = 1000 - ThrottleMAP;
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

POT_THROTTLE::THROTTLESTATUS POT_THROTTLE::getStatus() {
    return ThrottleStatus;
}

void POT_THROTTLE::setT1Min(uint16_t min) {
	ThrottleMin1 = min;
}

void POT_THROTTLE::setT2Min(uint16_t min) {
	ThrottleMin2 = min;
}

void POT_THROTTLE::setT1Max(uint16_t max) {
	ThrottleMax1 = max;
}
void POT_THROTTLE::setT2Max(uint16_t max) {
	ThrottleMax2 = max;
}

void POT_THROTTLE::setRegenEnd(uint16_t regen) {
	ThrottleRegen = regen;
}

void POT_THROTTLE::setFWDStart(uint16_t fwd) {
	ThrottleFWD = fwd;
}

void POT_THROTTLE::setMAP(uint16_t map) {
	ThrottleMAP = map;
}

void POT_THROTTLE::setMaxRegen(uint16_t regen) {
	ThrottleMaxRegen = regen;
}

DEVICE::DEVID POT_THROTTLE::getDeviceID() {
  return (DEVICE::POTACCELPEDAL);
}
