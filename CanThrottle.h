/*
 * CanThrottle.h
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

#ifndef CAN_THROTTLE_H_
#define CAN_THROTTLE_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "TickHandler.h"
#include "CanHandler.h"
#include "DeviceManager.h"

class CanThrottleConfiguration : public ThrottleConfiguration
{
public:
};

class CanThrottle: public Throttle, CanObserver
{
public:
    CanThrottle();
    void setup();
    void tearDown();
    void handleTick();
    void handleStateChange(Status::SystemState, Status::SystemState);
    void handleCanFrame(CAN_FRAME *frame);
    DeviceId getId();

    RawSignalData *acquireRawSignal();
    void loadConfiguration();
    void saveConfiguration();

protected:
    bool validateSignal(RawSignalData *);
    uint16_t calculatePedalPosition(RawSignalData *);

private:
    CAN_FRAME requestFrame; // the request frame sent to the car
    RawSignalData rawSignal; // raw signal
    uint8_t ticksNoResponse; // number of ticks no response was received
    uint32_t responseId; // the CAN id with which the response is sent;
    uint32_t responseMask; // the mask for the responseId
    bool responseExtended; // if the response is expected as an extended frame
};

#endif /* CAN_THROTTLE_H_ */
