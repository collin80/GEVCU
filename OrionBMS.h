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

    enum Orion_Flags {
        voltageFailsafe             = 1 << 0,
        currentFailsafe             = 1 << 1,
        depleted                    = 1 << 2,
        balancingActive             = 1 << 3,
        dtcWeakCellFault            = 1 << 4,
        dtcLowCellVolage            = 1 << 5,
        dtcHVIsolationFault         = 1 << 6,
        dtcVoltageRedundancyFault   = 1 << 7
    };

    enum Orion_CurrentLimit {
        dclLowSoc               = 1 << 0, //DCL Reduced Due To Low SOC (Bit #0)
        dclHighCellResistance   = 1 << 1, //DCL Reduced Due To High Cell Resistance (Bit #1)
        dclTemperature          = 1 << 2, //DCL Reduced Due To Temperature (Bit #2)
        dclLowCellVoltage       = 1 << 3, //DCL Reduced Due To Low Cell Voltage (Bit #3)
        dclLowPackVoltage       = 1 << 4, //DCL Reduced Due To Low Pack Voltage (Bit #4)
        dclCclVoltageFailsafe   = 1 << 6, //DCL and CCL Reduced Due To Voltage Failsafe (Bit #6)
        dclCclCommunication     = 1 << 7, //DCL and CCL Reduced Due To Communication Failsafe (Bit #7): This only applies if there are multiple BMS units connected together in series over CANBUS.
        cclHighSoc              = 1 << 9, //CCL Reduced Due To High SOC (Bit #9)
        cclHighCellResistance   = 1 << 10, //CCL Reduced Due To High Cell Resistance (Bit #10)
        cclTemperature          = 1 << 11, //CCL Reduced Due To Temperature (Bit #11)
        cclHighCellVoltage      = 1 << 12, //CCL Reduced Due To High Cell Voltage (Bit #12)
        cclHighPackVoltage      = 1 << 13, //CCL Reduced Due To High Pack Voltage (Bit #13)
        cclChargerLatch         = 1 << 14, //CCL Reduced Due To Charger Latch (Bit #14): This means the CCL is likely 0A because the charger has been turned off. This latch is removed when the Charge Power signal is removed and re-applied (ie: unplugging the car and plugging it back in).
        cclAlternate            = 1 << 15  //CCL Reduced Due To Alternate Current Limit [MPI] (Bit #15)
    };

    enum Orion_RelayStatus {
        relayDischarge      = 1 << 0, // Bit #0 (0x01): Discharge relay enabled
        relayCharge         = 1 << 1, // Bit #1 (0x02): Charge relay enabled
        chagerSafety        = 1 << 2, // Bit #2 (0x04): Charger safety enabled
        dtcPresent          = 1 << 3, // Bit #3 (0x08): Malfunction indicator active (DTC status)
        multiPurposeInput   = 1 << 4, // Bit #4 (0x10): Multi-Purpose Input signal status
        alwaysOn            = 1 << 5, // Bit #5 (0x20): Always-on signal status
        isReady               = 1 << 6, // Bit #6 (0x40): Is-Ready signal status
        isCharging            = 1 << 7  // Bit #7 (0x80): Is-Charging signal status
    };

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
    bool hasChargerEnabled();

protected:
private:
    uint8_t relayStatus, flags;
    uint16_t currentLimit;
    uint16_t packSummedVoltage;
    uint8_t canTickCounter;
    void processPack(uint8_t data[]);
    void processLimitsSoc(uint8_t data[]);
    void processCellVoltage(uint8_t data[]);
    void processCellResistance(uint8_t data[]);
    void processHealth(uint8_t data[]);
};

#endif
