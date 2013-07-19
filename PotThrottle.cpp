/*
 * pedal_pot.c
 *
 * Turn raw ADC readings into [-1000|1000] throttle output. Smooths throttle output
 * and properly handles both positive and negative travel pots
 *
 * Created: 1/13/2013 6:10:24 PM
 *  Author: Collin Kidder
 */

#include "PotThrottle.h"

char *t1TooHigh = "Throttle 1 Value Too High!";
char *t1TooLow = "Throttle 1 Value Too Low!";
char *t2TooHigh = "Throttle 2 Value Too High!";
char *t2TooLow = "Throttle 2 Value Too Low!";
char *tMismatch = "Throttle Value Mismatch";

//initialize by telling the code which two ADC channels to use (or set channel 2 to 255 to disable)
PotThrottle::PotThrottle(uint8_t throttle1, uint8_t throttle2, bool isAccel = true) {
  throttle1ADC = throttle1;
  throttle2ADC = throttle2;
  if (throttle2 == 255) numThrottlePots = 1;
    else numThrottlePots = 2;
  throttleStatus = OK;
  throttleMaxErr = 75; //in tenths of a percent. So 25 = max 2.5% difference
  isAccelerator = isAccel;
  //analogReadResolution(12);
}

void PotThrottle::setupDevice() {
  Throttle::setupDevice(); //call base class first
  //set digital ports to inputs and pull them up
  //all inputs currently active low
  //pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
  /*
  if (prefs->checksumValid()) { //checksum is good, read in the values stored in EEPROM
    prefs->read(EETH_MIN_ONE, &throttleMin1);
    prefs->read(EETH_MAX_ONE, &throttleMax1);
    prefs->read(EETH_MIN_TWO, &throttleMin2);
    prefs->read(EETH_MAX_TWO, &throttleMax2);
    prefs->read(EETH_REGEN, &throttleRegen);
    prefs->read(EETH_FWD, &throttleFwd);
    prefs->read(EETH_MAP, &throttleMap);
    prefs->read(EETH_BRAKE_MIN, &BrakeMin);
    prefs->read(EETH_BRAKE_MAX, &BrakeMax);
    prefs->read(EETH_MAX_ACCEL_REGEN, &ThrottleMaxRegen);
    prefs->read(EETH_MAX_BRAKE_REGEN, &BrakeMaxRegen);
  }
  else { //checksum invalid. Reinitialize values and store to EEPROM
  */
    //these four values are ADC values
    throttleMin1 = 180;
    throttleMax1 = 935;
    throttleMin2 = 360;
    throttleMax2 = 1900;
    //The next three are tenths of a percent
    throttleRegen = 0;
    throttleFwd = 175;
    throttleMap = 665;
    throttleMaxRegen = 00; //percentage of full power to use for regen at throttle
    brakeMaxRegen = 80; //percentage of full power to use for regen at brake pedal transducer
    brakeMin = 5;
    brakeMax = 500;
    prefs->write(EETH_MIN_ONE, throttleMin1);
    prefs->write(EETH_MAX_ONE, throttleMax1);
    prefs->write(EETH_MIN_TWO, throttleMin2);
    prefs->write(EETH_MAX_TWO, throttleMax2);
    prefs->write(EETH_REGEN, throttleRegen);
    prefs->write(EETH_FWD, throttleFwd);
    prefs->write(EETH_MAP, throttleMap);
    prefs->write(EETH_BRAKE_MIN, brakeMin);
    prefs->write(EETH_BRAKE_MAX, brakeMax);
    prefs->write(EETH_MAX_ACCEL_REGEN, throttleMaxRegen);
    prefs->write(EETH_MAX_BRAKE_REGEN, brakeMaxRegen);
    prefs->saveChecksum();
  //}
}

int PotThrottle::getRawThrottle1() {
	return throttle1Val;
}

int PotThrottle::getRawThrottle2() {
	return throttle2Val;
}

int PotThrottle::calcThrottle(int clampedVal, int minVal, int maxVal) {
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

void PotThrottle::doAccel() {
  signed int range;
  signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
  static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control
  char *errorMsg;
  
  clampedVal = throttle1Val;

  //The below code now only faults if the value of the ADC is 15 outside of the range +/-
  //otherwise we'll just clamp
  if (throttle1Val > throttleMax1) {
    if (throttle1Val > (throttleMax1 + CFG_THROTTLE_TOLERANCE)) {
      throttleStatus = ERR_HIGH_T1;
	  errorMsg = t1TooHigh;
      //SerialUSB.print("T1H ");      
    }
    clampedVal = throttleMax1;
  }
  tempLow = 0;
  if (throttleMin1 > (CFG_THROTTLE_TOLERANCE - 1)) {
    tempLow = throttleMin1 - CFG_THROTTLE_TOLERANCE;
  } 
  if (throttle1Val < throttleMin1) {
    if (throttle1Val < tempLow) {
      throttleStatus = ERR_LOW_T1;
	  errorMsg = t1TooLow;
      //SerialUSB.print("T1L ");      
    }
    clampedVal = throttleMin1;
  }

  if (! (throttleStatus == OK)) {
    outputThrottle = 0; //no throttle if there is a fault
	faultHandler.raiseFault(getDeviceID(), throttleStatus, errorMsg);
    return;
  }
  calcThrottle1 = calcThrottle(clampedVal, throttleMin1, throttleMax1);
   
  if (numThrottlePots > 1) { //can only do these things if there are two or more pots
    clampedVal = throttle2Val;
    if (throttle2Val > throttleMax2) {
      if (throttle2Val > (throttleMax2 + CFG_THROTTLE_TOLERANCE)) {
		throttleStatus = ERR_HIGH_T2;
		errorMsg = t2TooHigh;
        //SerialUSB.print("T2H ");      
      }
      clampedVal = throttleMax2;
    }
    tempLow = 0;
    if (throttleMin2 > (CFG_THROTTLE_TOLERANCE-1)) {
      tempLow = throttleMin2 - CFG_THROTTLE_TOLERANCE;
    } 
    if (throttle2Val < throttleMin2) {
      if (throttle2Val < tempLow) {
        throttleStatus = ERR_LOW_T2;
		errorMsg = t2TooLow;
        //SerialUSB.print("T2L ");      
      }
      clampedVal = throttleMin2;
    }

    calcThrottle2 = calcThrottle(clampedVal, throttleMin2, throttleMax2);
      
    if ((calcThrottle1 - throttleMaxErr) > calcThrottle2) { //then throttle1 is too large compared to 2
      throttleStatus = ERR_MISMATCH;
	  errorMsg = tMismatch;
      //SerialUSB.print("MX1 ");      
    }
    if ((calcThrottle2 - throttleMaxErr) > calcThrottle1) { //then throttle2 is too large compared to 1
      throttleStatus = ERR_MISMATCH;
	  errorMsg = tMismatch;
      //SerialUSB.print("MX2 ");      
    }

    calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; //temp now the average of the two
  }

  if (! (throttleStatus == OK)) {
    outputThrottle = 0; //no throttle if there is a fault
	faultHandler.raiseFault(getDeviceID(), throttleStatus, errorMsg);
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

    if (throttleRegen != 0) {
        if (ThrottleFeedback <= throttleRegen) {
            range = throttleRegen;
            temp = range - ThrottleFeedback;
            outputThrottle = (signed long)((signed long)(-10) * throttleMaxRegen  * temp / range);
        }
    }

    if (ThrottleFeedback >= throttleFwd) {
      if (ThrottleFeedback <= throttleMap) { //bottom 50% forward
        range = throttleMap - throttleFwd;
	temp = (ThrottleFeedback - throttleFwd);
	outputThrottle = (signed long)((signed long)(500) * temp / range);
      }
      else { //more than throttleMap
        range = 1000 - throttleMap;
	temp = (ThrottleFeedback - throttleMap);
        outputThrottle = 500 + (signed int)((signed long)(500) * temp / range);
      }
    }
}

/*
the brake only really uses one variable input and uses different parameters

story time: this code will start at ThrottleMaxRegen when applying the brake. It
will do this even if you're currently flooring it. The accelerator pedal is ignored
if there is any pressure detected on the brake. This is a sort of failsafe. It should
not be possible to go racing down the road with a stuck accelerator. As soon as the
brake is pressed it overrides the accelerator signal. Sorry, no standing burn outs.

*/
void PotThrottle::doBrake() {
    signed int range;
    signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
	static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

    clampedVal = throttle1Val;
    
    if (brakeMax == 0) {//brake processing disabled if Max is 0
       outputThrottle = 0; 
       return; 
    }    

	//The below code now only faults if the value of the ADC is 15 outside of the range +/-
	//otherwise we'll just clamp
	if (throttle1Val > brakeMax) {
	  if (throttle1Val > (brakeMax + 15)) {
	    throttleStatus = ERR_HIGH_T1;
            //SerialUSB.print("A");
	  }
	  clampedVal = brakeMax;
	}

	tempLow = 0;
	if (brakeMin > 14) {
		tempLow = brakeMin - 15;
	} 
	if (throttle1Val < brakeMin) {
	  if (throttle1Val < tempLow) {
	    throttleStatus = ERR_LOW_T1;
            //SerialUSB.print("B");
	  }
	  clampedVal = brakeMin;
	}

	if (! (throttleStatus == OK)) {
		outputThrottle = 0; //no throttle if there is a fault
		return;
	}
	calcThrottle1 = calcThrottle(clampedVal, brakeMin, brakeMax);
        //SerialUSB.println(calcThrottle1);
   

    //Apparently all is well with the throttle input
    //so go ahead and calculate the proper throttle output


    //still use this smoothing/easing code for the brake. It works quickly enough
    ThrottleAvg += calcThrottle1;
    ThrottleAvg -= ThrottleFeedback;
    ThrottleFeedback = ThrottleAvg >> 4;

    outputThrottle = 0; //by default we give zero throttle

    //I suppose I should explain. This prevents flutter in the ADC readings of the brake from slamming
    //regen on intermittantly just because the value fluttered a couple of numbers. This makes sure
    //that we're actually pushing the pedal. Without this even a small flutter at the brake will send
    //ThrottleMaxRegen regen out and ignore the accelerator. That'd be unpleasant.
    if (ThrottleFeedback < 15) {
      outputThrottle = 0;
      return;
    }

    if (brakeMaxRegen != 0) { //is the brake regen even enabled?
      int range = brakeMaxRegen - throttleMaxRegen; //we start the brake at ThrottleMaxRegen so the range is reduced by that amount
      if (range < 1) { //detect stupidity and abort
        outputThrottle = 0;
	return;
      }
      outputThrottle = (signed int)((signed int)-10 * range * ThrottleFeedback) / (signed int)1000;
      outputThrottle -= -10 * throttleMaxRegen;
      //SerialUSB.println(outputThrottle);
    }

}


//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void PotThrottle::handleTick() {
  throttle1Val = getAnalog(throttle1ADC);
  if (numThrottlePots > 1) {
    throttle2Val = getAnalog(throttle2ADC);
  }

  throttleStatus = OK;

  if (isAccelerator) 
  {
    doAccel();
  }
  else 
  {	
    doBrake();
  }
}

PotThrottle::ThrottleStatus PotThrottle::getStatus() {
    return throttleStatus;
}

void PotThrottle::setT1Min(uint16_t min) {
	throttleMin1 = min;
}

void PotThrottle::setT2Min(uint16_t min) {
	throttleMin2 = min;
}

void PotThrottle::setT1Max(uint16_t max) {
	throttleMax1 = max;
}
void PotThrottle::setT2Max(uint16_t max) {
	throttleMax2 = max;
}

void PotThrottle::setRegenEnd(uint16_t regen) {
	throttleRegen = regen;
}

void PotThrottle::setFWDStart(uint16_t fwd) {
	throttleFwd = fwd;
}

void PotThrottle::setMAP(uint16_t map) {
	throttleMap = map;
}

void PotThrottle::setMaxRegen(uint16_t regen) {
	throttleMaxRegen = regen;
}

Device::DeviceId PotThrottle::getDeviceID() {
  return (Device::POTACCELPEDAL);
}

