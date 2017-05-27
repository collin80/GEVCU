/*
 * StatusIndicator.h
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

#ifndef STATUSINDICATOR_H_
#define STATUSINDICATOR_H_

#include "config.h"
#include "TickHandler.h"
#include "DeviceManager.h"
#include "SystemIO.h"
#include "Status.h"

class StatusIndicator: public Device
{
public:
    StatusIndicator();
    void setup();
    void handleTick();
    void handleStateChange(Status::SystemState oldState, Status::SystemState newState);
    void handleMessage(uint32_t messageType, void* message);
    DeviceType getType();
    DeviceId getId();

    void loadConfiguration();
    void saveConfiguration();

protected:

private:
    enum MODE { none, on, off, blink, pulse };

    uint8_t lightLevel;
    MODE mode;
    boolean up;
};

#endif /* STATUSINDICATOR_H_ */
