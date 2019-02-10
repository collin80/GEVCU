/*
 * OrionBMS.h
 *
 * Controller for Orion Battery Management System
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

#ifndef ORIONBMS_H_
#define ORIONBMS_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"
#include "DeviceManager.h"
#include "BatteryManager.h"
#include "CanHandler.h"
#include "FaultHandler.h"
#include "CRC8.h"

// CAN bus id's for frames received from Orion BMS
#define CAN_ID_PACK             0x6b0 // receive actual values information       110 1011 0000
#define CAN_ID_LIMITS_SOC       0x6b1 // receive actual values information       110 1011 0001
#define CAN_ID_CELL_VOLTAGE     0x6b2 // receive actual values information       110 1011 0010
#define CAN_ID_CELL_RESISTANCE  0x6b3 // receive actual values information       110 1011 0011
#define CAN_ID_HEALTH           0x6b4 // receive actual values information       110 1011 0100
#define CAN_MASK                0x7f8 // mask for above id's                     111 1111 1000
#define CAN_MASKED_ID           0x6b0 // masked id for above id's                110 1011 0000

class OrionBMS : public BatteryManager, CanObserver
{
public:
    OrionBMS();
    void setup();
    void tearDown();
    void handleTick();
    void handleCanFrame(CAN_FRAME *frame);
    DeviceId getId();
    bool hasPackVoltage();
    bool hasPackCurrent();
    bool hasCellTemperatures();
    bool hasSoc();
    bool hasChargeLimit();
    bool hasDischargeLimit();
    bool hasAllowCharging();
    bool hasAllowDischarging();
    bool hasCellVoltages();
    bool hasCellResistance();
    bool hasPackHealth();
    bool hasPackCycles();
    bool hasPackResistance();

protected:
private:
    uint8_t relayStatus, flags;
    uint16_t currentLimit;
    uint16_t packSummedVoltage;
    void sendKeepAlive();
};

#endif
