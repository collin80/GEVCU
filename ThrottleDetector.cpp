/*
 * ThrottleDetector.cpp
 *
 * This class can detect up to two potentiometers and determine their min/max values,
 * whether they read low to high or high to low, and if the second potentiometer is
 * the inverse of the first.
 *
 * Created: 7/15/2013 11:24:35 PM
 * Author: Charles Galpin
 */

#include "ThrottleDetector.h"
#include "sys_io.h"
//#include "logger.h"

/*
 * Two throttles can be provided. A value of 255 for throttle2 means no throttle
 */
ThrottleDetector::ThrottleDetector(uint8_t throttle1, uint8_t throttle2 = 255)
{
    setThrottle1(throttle1);
    setThrottle2(throttle2);
    void resetValues();

    potentiometerCount = 1;
    throttle1HighLow = false;
    throttle2HighLow = false;
    throttle2Inverse = false;
   
    //Logger::debug("ThrottleDetector ctor with throttles: %d and %d", throttle1, throttle2);
}

/*
 * Nothing to do in the destructor right now but good to have as a general rule
 */
ThrottleDetector::~ThrottleDetector()
{
}

/*
 * Set the first throttle
 */
void ThrottleDetector::setThrottle1(uint8_t throttle)
{
  throttle1 = throttle;
}

/*
 * Set the second throttle
 */
void ThrottleDetector::setThrottle2(uint8_t throttle)
{
  throttle2 = throttle;
}

/*
 * Run the throttle detection.  This currenty uses SerialUSB to instruct the user
 */       
void ThrottleDetector::detect()
{
  SerialUSB.println("Throttle detection starting. Do NOT press the pedal until instructed.");
  
  // pause to give them a chance to get off the throttle (if for some reason they were pressing it)
  delay(3000);
  
  // drop initial readings as they seem to be invalid
  readThrottleValues(true);
  
  // First measure values at rest (MIN throttle) to be able to
  // determine if they are HIGH-LOW pots
  calibrate(true);
  
  // save rest minimums
  int throttle1MinRest = throttle1Min;  // minimum sensor value at rest
  int throttle2MinRest = throttle2Min;  // minimum sensor value at rest
  int throttle2MaxRest = throttle2Max;  // maximum sensor value at rest
  
  SerialUSB.println("");
  SerialUSB.println("Fully depress and hold the pedal until complete");
  
  // pause to give them a chance to press the throttle
  delay(3000);
  
  // Now measure when fully pressed
  calibrate(false);
  
  // Determine throttle type
  if ( throttle1MinRest > throttle1Min ) { // high to low pot
    throttle1HighLow = true;
  }
  
  if ( throttle2MinRest > throttle2Min ) { // high to low pot
    throttle2HighLow = true;
  }
  
  // restore the true min
  throttle1Min = throttle1MinRest;
  
  if ( throttle2Provided() ) 
  {
    
    if ( ( throttle1HighLow && !throttle2HighLow )  || 
       ( throttle2HighLow && !throttle1HighLow ) ) {
       throttle2Inverse = true; 
    }
    
    // Detect grounded pin (always zero) or floating values which indicate no potentiometer provided
    // If the values deviate by more than 10% we assume floating
    int restDiff = abs(throttle2MaxRest-throttle2MinRest)*100/throttle2MaxRest;
    int maxDiff = abs(throttle2Max-throttle2Min)*100/throttle2Max;
    if ( ( throttle2MinRest == 0 && throttle2MaxRest == 0 && throttle2Min == 0 && throttle2Max == 0) ||
         (restDiff > 10 && maxDiff > 10) )
    {
      potentiometerCount = 1;
    } else {
      potentiometerCount = 2;
    }
    
    // restore the true min
    throttle2Min = throttle2MinRest;
  }
  
  SerialUSB.println("");
  SerialUSB.println("=======================================");
  SerialUSB.println("Detection complete");
  SerialUSB.print("Num potentiometers found: ");
  SerialUSB.println(getPotentiometerCount());
  SerialUSB.print("T1: ");
  SerialUSB.print(getThrottle1Min(), DEC);
  SerialUSB.print(" to ");
  SerialUSB.print(getThrottle1Max(), DEC);
  SerialUSB.println(isThrottle1HighLow() ? " HIGH-LOW" : " LOW-HIGH");
  
  if ( getPotentiometerCount() > 1) 
  {
    SerialUSB.print("T2: ");
    SerialUSB.print(isThrottle2Inverse() ? getThrottle2Max() : getThrottle2Min(), DEC);
    SerialUSB.print(" to ");
    SerialUSB.print(isThrottle2Inverse() ? getThrottle2Min() : getThrottle2Max(), DEC);
    SerialUSB.print(isThrottle2HighLow() ? " HIGH-LOW" : " LOW-HIGH");
    SerialUSB.println(isThrottle2Inverse() ? " (Inverse of T1)" : "");
  }

  SerialUSB.println("========================================");
  
  // give the user time to see this
  delay (3000);
}

/*
 * Returns the number of potentiometers detected
 */
int ThrottleDetector::getPotentiometerCount() 
{
  return potentiometerCount;
}

/*
 * Returns true if throttle1 ranges from highest to lowest value
 * as the pedal is pressed
 */
bool ThrottleDetector::isThrottle1HighLow() 
{
    return throttle1HighLow;
}

/*
 * Returns true if throttle2 ranges from highest to lowest value
 * as the pedal is pressed
 */        
bool ThrottleDetector::isThrottle2HighLow()
{
    return throttle2HighLow;
}

/*
 * Returns the minimum value of throttle1
 */        
uint16_t ThrottleDetector::getThrottle1Min()
{
    return throttle1Min;
}

/*
 * Returns the maximum value of throttle1
 */ 
uint16_t ThrottleDetector::getThrottle1Max()
{
    return throttle1Max;
}

/*
 * Returns the minimum value of throttle2
 */ 
uint16_t ThrottleDetector::getThrottle2Min()
{
    return throttle2Min;
}

/*
 * Returns the maximum value of throttle2
 */ 
uint16_t ThrottleDetector::getThrottle2Max()
{
    return throttle2Max;
}

/*
 * Returns true if throttle2 values are the opposite of the
 * throttle1 values. For example, if throttle1 is low-high and 
 * throttle2 is high-low.
 */
bool ThrottleDetector::isThrottle2Inverse()
{
    return throttle2Inverse;
}

/*
 * Returns true if a second throttle was provided
 */
bool ThrottleDetector::throttle2Provided()
{
  return throttle2 != 255;
}

/*
 * Reset/initialize some values
 */
void ThrottleDetector::resetValues()
{
    throttle1Value = 0;
    throttle1Min = INT16_MAX;
    throttle1Max = 0;
    throttle2Value = 0;
    throttle2Min = INT16_MAX;
    throttle2Max = 0;
}

/*
 * Reads values from the throttles. If discardValues is true
 * then the min/max are not affected.
 */
void ThrottleDetector::readThrottleValues(bool discardValues = false)
{
    throttle1Value = getDiffADC(throttle1);
    if ( throttle2Provided() ) 
    {
      throttle2Value = getDiffADC(throttle2);
    }
    
    if ( discardValues ) 
    {
      return;
    }

    // record the minimum sensor value
    if (throttle1Value < throttle1Min) {
      throttle1Min = throttle1Value;
    }
    
    // record the maximum sensor value
    if (throttle1Value > throttle1Max) {
      throttle1Max = throttle1Value;
    }

    if ( throttle2Provided() ) 
    {
      // record the minimum sensor value
      if (throttle2Value < throttle2Min) {
        throttle2Min = throttle2Value;
      }
      
      // record the maximum sensor value
      if (throttle2Value > throttle2Max) {
        throttle2Max = throttle2Value;
      }
    }
}

/*
 * Read some values and save the maximum and minimum values
 */
void ThrottleDetector::calibrate(bool minPedal)
{
  resetValues();
  
  // calibrate for 3 seconds
  unsigned long time = millis();
  int count = 0;
  while ((millis() - time) < 3000) {
    readThrottleValues();
    
    // show progress
    if ( count++ >= 100 ) {
      SerialUSB.print(".");
      count= 0;
    }
    delay(10);
  }
  
  SerialUSB.println("");
  SerialUSB.print("At ");
  if ( minPedal )
  {
    SerialUSB.print("MIN");
  } else {
    SerialUSB.print("MAX");
  }
  SerialUSB.print(" T1: ");
  SerialUSB.print(throttle1Min, DEC);
  SerialUSB.print(" to ");
  SerialUSB.print(throttle1Max, DEC);
  if ( throttle2Provided() ) 
  {
    SerialUSB.print(" T2: ");
    SerialUSB.print(throttle2Min, DEC);
    SerialUSB.print(" to ");
    SerialUSB.print(throttle2Max, DEC);
  }
  SerialUSB.println("");
}

