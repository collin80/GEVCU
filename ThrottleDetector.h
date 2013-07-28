/*
 * ThrottleDetector.h
 *
 * This class can detect up to two potentiometers and determine their min/max values,
 * whether they read low to high or high to low, and if the second potentiometer is
 * the inverse of the first.
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

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

