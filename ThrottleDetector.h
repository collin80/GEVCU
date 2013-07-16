/*
 * ThrottleDetector.h
 *
 * This class can detect up to two potentiometers and determine their min/max values,
 * whether they read low to high or high to low, and if the second potentiometer is
 * the inverse of the first.
 *
 * Created: 7/15/2013 11:24:35 PM
 * Author: Charles Galpin
 */

#ifndef THROTTLE_DETECTOR_H_
#define THROTTLE_DETECTOR_H_

#include <Arduino.h>

class ThrottleDetector {

    public:
        ThrottleDetector(uint8_t throttle1, uint8_t throttle2);
        void setThrottle1(uint8_t throttle);
        void setThrottle2(uint8_t throttle);
        void detect();
        int getPotentiometerCount();
        bool isThrottle1HighLow();
        bool isThrottle2HighLow();
        bool isThrottle2Inverse();
        uint16_t getThrottle1Min();
        uint16_t getThrottle1Max();
        uint16_t getThrottle2Min();
        uint16_t getThrottle2Max(); 
        ~ThrottleDetector();

    private:
        void resetValues();
        void readThrottleValues(bool discardValues);
        void calibrate(bool minPedal);
        bool throttle2Provided();
        int potentiometerCount;
        uint8_t throttle1;
        uint8_t throttle2;
        uint16_t throttle1Value;
        uint16_t throttle1Min;
        uint16_t throttle1Max;
        uint16_t throttle2Value;
        uint16_t throttle2Min;
        uint16_t throttle2Max;
        bool throttle1HighLow;
        bool throttle2HighLow;
        bool throttle2Inverse;
};

#endif /* THROTTLE_DETECTOR_H_ */

