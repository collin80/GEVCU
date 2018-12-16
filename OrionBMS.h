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

// CAN bus id's for frames received from Orion BMS

#define CAN_ID_VALUES_1         0x3ca // receive actual values information       01111001010
#define CAN_ID_VALUES_2         0x3cb // receive actual values information       01111001011
#define CAN_MASK_1              0x7fe // mask for above id's                     11111111110
#define CAN_MASKED_ID_1         0x3ca // masked id for above id's                01111001010

#define CAN_ID_VALUES_3         0x6b2 // receive actual values information       11010110010
#define CAN_ID_VALUES_4         0x6b3 // receive actual values information       11010110011
#define CAN_ID_CELL_VOLTAGE     0x6b4 // receive cell voltages (min/max/avg)     11010110100
#define CAN_ID_CELL_RESISTANCE  0x6b5 // receive cell resistances (min/max/avg)  11010110101
#define CAN_MASK_2              0x7f8 // mask for above id's                     11111111000
#define CAN_MASKED_ID_2         0x6b0 // masked id for above id's                11010110000

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

protected:
private:
    uint8_t relayStatus, flags;
    uint16_t currentLimit;
    uint16_t packSummedVoltage;
    void sendKeepAlive();
};

#endif
