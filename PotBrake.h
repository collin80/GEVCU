/*
 * PotBrake.h
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

#ifndef POT_BRAKE_H_
#define POT_BRAKE_H_

#include <Arduino.h>
#include "config.h"
#include "PotThrottle.h"
#include "SystemIO.h"
#include "TickHandler.h"
#include "Logger.h"
#include "DeviceManager.h"

#define THROTTLE_INPUT_BRAKELIGHT  2

/*
 * The extended configuration class with additional parameters for PotBrake
 *
 * NOTE: Because of ThrottleDetector, this currently MUST be the same as PotThrottleConfiguratin !!!
 */
class PotBrakeConfiguration: public PotThrottleConfiguration
{
public:
};

class PotBrake: public Throttle
{
public:
    PotBrake();
    void setup();
    void handleTick();
    DeviceId getId();
    DeviceType getType();

    RawSignalData *acquireRawSignal();

    void loadConfiguration();
    void saveConfiguration();

protected:
    bool validateSignal(RawSignalData *);
    uint16_t calculatePedalPosition(RawSignalData *);
    int16_t mapPedalPosition(int16_t);

private:
    RawSignalData rawSignal;
};

#endif /* POT_BRAKE_H_ */
