/*
 * Esp32Wifi.h
 *
 * Class to interface with the ESP32 based wifi adapter
 *
 * Created: 3/31/2020
 *  Author: Michael Neuweiler

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

#ifndef WIFIESP32_H_
#define WIFIESP32_H_

#include <Arduino.h>
#include "Wifi.h"
#include "ParamCache.h"

class WifiEsp32 : public Wifi
{
public:

    WifiEsp32();
    void setup(); //initialization on start up
    void tearDown();
    void handleTick(); //periodic processes
    void handleMessage(uint32_t messageType, void* message);
    void handleStateChange(Status::SystemState, Status::SystemState);
    DeviceType getType();
    DeviceId getId();
    void process();

private:
    char incomingBuffer[CFG_WIFI_BUFFER_SIZE]; //storage for one incoming line
    int ibWritePtr; //write position into above buffer
    int psWritePtr;
    int psReadPtr;
    int tickCounter;
    bool didParamLoad;
    uint32_t lastSendTime;
    uint32_t timeStarted;
    int remainingSocketRead;
    uint8_t watchdogCounter; // a counter to find out if ichip has crashed

    void requestNextParam(); //get next changed parameter
    void requestParamValue(String paramName);  //try to retrieve the value of the given parameter
    void setParam(String paramName, String value);  //set the given parameter with the given string
    void sendCmd(String cmd);
    void sendSocketUpdate();
    void processStartSocketListenerRepsonse();
    void processActiveSocketListResponse();
    void processIncomingSocketData();
    void processSocketSendResponse();

    void reset();
};

#endif
