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
  ThrottleStatus = OK;
  ThrottleMaxErr = 25; //in tenths of a percent. So 25 = max 2.5% difference
#if defined(__SAM3X8E__)
  analogReadResolution(12);
#endif
}

void POT_THROTTLE::setupDevice() {
  THROTTLE::setupDevice(); //call base class first
  //set digital ports to inputs and pull them up
  //all inputs currently active low
  pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
#ifdef __SAM3X8E__
    //for now hard code values for Due since it has no EEPROM
    ThrottleMin1 = 325;
    ThrottleMax1 = 1620;
    ThrottleMin2 = 642;
    ThrottleMax2 = 3232;
    ThrottleRegen = 0;
    ThrottleFWD = 175;
    ThrottleMAP = 665;
    ThrottleMaxRegen = 30; //mmildly powerful regen for accel pedal
    BrakeMaxRegen = 80; //pretty strong regen for brakes  
    BrakeMin = 0;
    BrakeMax = 0;
#else
  if (prefChecksumValid()) { //checksum is good, read in the values stored in EEPROM
    prefRead(EETH_MIN_ONE, ThrottleMin1);
    prefRead(EETH_MAX_ONE, ThrottleMax1);
    prefRead(EETH_MIN_TWO, ThrottleMin2);
    prefRead(EETH_MAX_TWO, ThrottleMax2);
    prefRead(EETH_REGEN, ThrottleRegen);
    prefRead(EETH_FWD, ThrottleFWD);
    prefRead(EETH_MAP, ThrottleMAP);
    prefRead(EETH_BRAKE_MIN, BrakeMin);
    prefRead(EETH_BRAKE_MAX, BrakeMax);    
    prefRead(EETH_MAX_ACCEL_REGEN, ThrottleMaxRegen);
    prefRead(EETH_MAX_BRAKE_REGEN, BrakeMaxRegen); 
  }
  else { //checksum invalid. Reinitialize values and store to EEPROM
    ThrottleMin1 = 82;
    ThrottleMax1 = 410;
    ThrottleMin2 = 158;
    ThrottleMax2 = 810;
    ThrottleRegen = 0;
    ThrottleFWD = 175;
    ThrottleMAP = 665;
    ThrottleMaxRegen = 30; //mmildly powerful regen for accel pedal
    BrakeMaxRegen = 80; //pretty strong regen for brakes  
    BrakeMin = 0;
    BrakeMax = 0;
    prefWrite(EETH_MIN_ONE, ThrottleMin1);
    prefWrite(EETH_MAX_ONE, ThrottleMax1);
    prefWrite(EETH_MIN_TWO, ThrottleMin2);
    prefWrite(EETH_MAX_TWO, ThrottleMax2);
    prefWrite(EETH_REGEN, ThrottleRegen);
    prefWrite(EETH_FWD, ThrottleFWD);
    prefWrite(EETH_MAP, ThrottleMAP);
    prefWrite(EETH_BRAKE_MIN, BrakeMin);
    prefWrite(EETH_BRAKE_MAX, BrakeMax);    
    prefWrite(EETH_MAX_ACCEL_REGEN, ThrottleMaxRegen);
    prefWrite(EETH_MAX_BRAKE_REGEN, BrakeMaxRegen);
    prefSaveChecksum();
  }
#endif
}

int POT_THROTTLE::getRawThrottle1() {
	return Throttle1Val;
}

int POT_THROTTLE::getRawThrottle2() {
	return Throttle2Val;
}

int POT_THROTTLE::calcThrottle(int clampedVal, int minVal, int maxVal) {
    int range, val, retVal;

    if (minVal < maxVal) { //low to high pot
        range = maxVal - minVal;
        val = clampedVal - minVal;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
    }
    else { //high to low pot
        range = minVal - maxVal;
        val = clampedVal - maxVal;
        retVal = (int)(((long)val * 1000) / (long)range); //output is tenths of a percent of max throttle
        retVal = 1000 - retVal; //reverses the value since the pedal runs reverse
    }

    return retVal;
}

//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void POT_THROTTLE::handleTick() {
    signed int range;
    signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
    static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

    Throttle1Val = analogRead(Throttle1ADC);
    if (numThrottlePots > 1) {
      Throttle2Val = analogRead(Throttle2ADC);
    }

    ThrottleStatus = OK;
    clampedVal = Throttle1Val;
    
    //The below code now only faults if the value of the ADC is 15 outside of the range +/-
    //otherwise we'll just clamp
    if (Throttle1Val > ThrottleMax1) {
       if (Throttle1Val > (ThrottleMax1 + 15)) {
          ThrottleStatus = ERR_HIGH_T1;
       }
       clampedVal = ThrottleMax1;
    }
    tempLow = 0;
    if (ThrottleMin1 > 14) {
      tempLow = ThrottleMin1 - 15;
    } 
    if (Throttle1Val < ThrottleMin1) {
      if (Throttle1Val < tempLow) {
        ThrottleStatus = ERR_LOW_T1;
      }
      clampedVal = ThrottleMin1;
    }

    if (! (ThrottleStatus == OK)) {
        outputThrottle = 0; //no throttle if there is a fault
        return;
    }

    calcThrottle1 = calcThrottle(clampedVal, ThrottleMin1, ThrottleMax1);
    
    if (numThrottlePots > 1) { //can only do these things if there are two or more pots
      clampedVal = Throttle2Val;
      if (Throttle2Val > ThrottleMax2) {
         if (Throttle2Val > (ThrottleMax2 + 15)) {
            ThrottleStatus = ERR_HIGH_T2;
         }
         clampedVal = ThrottleMax2;
      }
      tempLow = 0;
      if (ThrottleMin2 > 14) {
        tempLow = ThrottleMin2 - 15;
      } 
      if (Throttle2Val < ThrottleMin2) {
        if (Throttle2Val < tempLow) {
          ThrottleStatus = ERR_LOW_T2;
        }
        clampedVal = ThrottleMin2;
      }

      calcThrottle2 = calcThrottle(clampedVal, ThrottleMin2, ThrottleMax2);
      
      if ((calcThrottle1 - ThrottleMaxErr) > calcThrottle2) { //then throttle1 is too large compared to 2
          ThrottleStatus = ERR_MISMATCH;
      }
      if ((calcThrottle2 - ThrottleMaxErr) > calcThrottle1) { //then throttle2 is too large compared to 1
          ThrottleStatus = ERR_MISMATCH;
      }

      calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; //temp now the average of the two
    }

    if (! (ThrottleStatus == OK)) {
        outputThrottle = 0; //no throttle if there is a fault
        return;
    }

    //Apparently all is well with the throttle input
    //so go ahead and calculate the proper throttle output

    ThrottleAvg += calcThrottle1;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;

	outputThrottle = 0; //by default we give zero throttle

    /* Since this code is now based on tenths of a percent of throttle push it is now agnostic to how that happens
       positive or negative travel doesn't matter and is covered by the calcThrottle functions
    */

    if (ThrottleRegen != 0) {
        if (ThrottleFeedback <= ThrottleRegen) {
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
