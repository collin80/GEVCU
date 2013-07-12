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
POT_THROTTLE::POT_THROTTLE(uint8_t Throttle1, uint8_t Throttle2, bool isAccel = true) {
  Throttle1ADC = Throttle1;
  Throttle2ADC = Throttle2;
  if (Throttle2 == 255) numThrottlePots = 1;
    else numThrottlePots = 2;
  ThrottleStatus = OK;
  ThrottleMaxErr = 75; //in tenths of a percent. So 25 = max 2.5% difference
  isAccelerator = isAccel;
  //analogReadResolution(12);
}

void POT_THROTTLE::setupDevice() {
  THROTTLE::setupDevice(); //call base class first
  //set digital ports to inputs and pull them up
  //all inputs currently active low
  //pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch
  /*
  if (prefs->checksumValid()) { //checksum is good, read in the values stored in EEPROM
    prefs->read(EETH_MIN_ONE, &ThrottleMin1);
    prefs->read(EETH_MAX_ONE, &ThrottleMax1);
    prefs->read(EETH_MIN_TWO, &ThrottleMin2);
    prefs->read(EETH_MAX_TWO, &ThrottleMax2);
    prefs->read(EETH_REGEN, &ThrottleRegen);
    prefs->read(EETH_FWD, &ThrottleFWD);
    prefs->read(EETH_MAP, &ThrottleMAP);
    prefs->read(EETH_BRAKE_MIN, &BrakeMin);
    prefs->read(EETH_BRAKE_MAX, &BrakeMax);
    prefs->read(EETH_MAX_ACCEL_REGEN, &ThrottleMaxRegen);
    prefs->read(EETH_MAX_BRAKE_REGEN, &BrakeMaxRegen);
  }
  else { //checksum invalid. Reinitialize values and store to EEPROM
  */
    //these four values are ADC values
    ThrottleMin1 = 8;
    ThrottleMax1 = 450;
    ThrottleMin2 = 355;
    ThrottleMax2 = 1800;
    //The next three are tenths of a percent
    ThrottleRegen = 0;
    ThrottleFWD = 175;
    ThrottleMAP = 665;
    ThrottleMaxRegen = 00; //percentage of full power to use for regen at throttle
    BrakeMaxRegen = 80; //percentage of full power to use for regen at brake pedal transducer
    BrakeMin = 5;
    BrakeMax = 500;
    prefs->write(EETH_MIN_ONE, ThrottleMin1);
    prefs->write(EETH_MAX_ONE, ThrottleMax1);
    prefs->write(EETH_MIN_TWO, ThrottleMin2);
    prefs->write(EETH_MAX_TWO, ThrottleMax2);
    prefs->write(EETH_REGEN, ThrottleRegen);
    prefs->write(EETH_FWD, ThrottleFWD);
    prefs->write(EETH_MAP, ThrottleMAP);
    prefs->write(EETH_BRAKE_MIN, BrakeMin);
    prefs->write(EETH_BRAKE_MAX, BrakeMax);
    prefs->write(EETH_MAX_ACCEL_REGEN, ThrottleMaxRegen);
    prefs->write(EETH_MAX_BRAKE_REGEN, BrakeMaxRegen);
    prefs->saveChecksum();
  //}
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

void POT_THROTTLE::doAccel() {
  signed int range;
  signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
  static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control
  
  clampedVal = Throttle1Val;

  //The below code now only faults if the value of the ADC is 15 outside of the range +/-
  //otherwise we'll just clamp
  if (Throttle1Val > ThrottleMax1) {
    if (Throttle1Val > (ThrottleMax1 + CFG_THROTTLE_TOLERANCE)) {
      ThrottleStatus = ERR_HIGH_T1;
      //SerialUSB.print("T1H ");      
    }
    clampedVal = ThrottleMax1;
  }
  tempLow = 0;
  if (ThrottleMin1 > (CFG_THROTTLE_TOLERANCE - 1)) {
    tempLow = ThrottleMin1 - CFG_THROTTLE_TOLERANCE;
  } 
  if (Throttle1Val < ThrottleMin1) {
    if (Throttle1Val < tempLow) {
      ThrottleStatus = ERR_LOW_T1;
      //SerialUSB.print("T1L ");      
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
      if (Throttle2Val > (ThrottleMax2 + CFG_THROTTLE_TOLERANCE)) {
	ThrottleStatus = ERR_HIGH_T2;
        //SerialUSB.print("T2H ");      
      }
      clampedVal = ThrottleMax2;
    }
    tempLow = 0;
    if (ThrottleMin2 > (CFG_THROTTLE_TOLERANCE-1)) {
      tempLow = ThrottleMin2 - CFG_THROTTLE_TOLERANCE;
    } 
    if (Throttle2Val < ThrottleMin2) {
      if (Throttle2Val < tempLow) {
        ThrottleStatus = ERR_LOW_T2;
        //SerialUSB.print("T2L ");      
      }
      clampedVal = ThrottleMin2;
    }

    calcThrottle2 = calcThrottle(clampedVal, ThrottleMin2, ThrottleMax2);
      
    if ((calcThrottle1 - ThrottleMaxErr) > calcThrottle2) { //then throttle1 is too large compared to 2
      ThrottleStatus = ERR_MISMATCH;
      //SerialUSB.print("MX1 ");      
    }
    if ((calcThrottle2 - ThrottleMaxErr) > calcThrottle1) { //then throttle2 is too large compared to 1
      ThrottleStatus = ERR_MISMATCH;
      //SerialUSB.print("MX2 ");      
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
}

/*
the brake only really uses one variable input and uses different parameters

story time: this code will start at ThrottleMaxRegen when applying the brake. It
will do this even if you're currently flooring it. The accelerator pedal is ignored
if there is any pressure detected on the brake. This is a sort of failsafe. It should
not be possible to go racing down the road with a stuck accelerator. As soon as the
brake is pressed it overrides the accelerator signal. Sorry, no standing burn outs.

*/
void POT_THROTTLE::doBrake() {
    signed int range;
    signed int calcThrottle1, calcThrottle2, clampedVal, tempLow, temp;
	static uint16_t ThrottleAvg = 0, ThrottleFeedback = 0; //used to create proportional control

    clampedVal = Throttle1Val;
    
    if (BrakeMax == 0) {//brake processing disabled if Max is 0
       outputThrottle = 0; 
       return; 
    }    

	//The below code now only faults if the value of the ADC is 15 outside of the range +/-
	//otherwise we'll just clamp
	if (Throttle1Val > BrakeMax) {
	  if (Throttle1Val > (BrakeMax + 15)) {
	    ThrottleStatus = ERR_HIGH_T1;
            //SerialUSB.print("A");
	  }
	  clampedVal = BrakeMax;
	}

	tempLow = 0;
	if (BrakeMin > 14) {
		tempLow = BrakeMin - 15;
	} 
	if (Throttle1Val < BrakeMin) {
	  if (Throttle1Val < tempLow) {
	    ThrottleStatus = ERR_LOW_T1;
            //SerialUSB.print("B");
	  }
	  clampedVal = BrakeMin;
	}

	if (! (ThrottleStatus == OK)) {
		outputThrottle = 0; //no throttle if there is a fault
		return;
	}
	calcThrottle1 = calcThrottle(clampedVal, BrakeMin, BrakeMax);
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

    if (BrakeMaxRegen != 0) { //is the brake regen even enabled?
      int range = BrakeMaxRegen - ThrottleMaxRegen; //we start the brake at ThrottleMaxRegen so the range is reduced by that amount
      if (range < 1) { //detect stupidity and abort
        outputThrottle = 0;
	return;
      }
      outputThrottle = (signed int)((signed int)-10 * range * ThrottleFeedback) / (signed int)1000;
      outputThrottle -= -10 * ThrottleMaxRegen;    
      //SerialUSB.println(outputThrottle);
    }

}


//right now only the first throttle ADC port is used. Eventually the second one should be used to cross check so dumb things
//don't happen. Also, right now values of ADC outside the proper range are just clamped to the proper range.
void POT_THROTTLE::handleTick() {
  Throttle1Val = getAnalog(Throttle1ADC);
  if (numThrottlePots > 1) {
    Throttle2Val = getAnalog(Throttle2ADC);
  }

  ThrottleStatus = OK;

  if (isAccelerator) 
  {
    doAccel();
  }
  else 
  {	
    doBrake();
  }
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

