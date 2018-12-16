/*
 * OrionBMS.cpp
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

#include "OrionBMS.h"

OrionBMS::OrionBMS() :
        BatteryManager()
{
    prefsHandler = new PrefHandler(ORIONBMS);
    commonName = "Orion BMS";
    relayStatus = 0;
    flags = 0;
    currentLimit = 0;
    packSummedVoltage = 0;
}

void OrionBMS::setup()
{
    BatteryManager::setup();

    canHandlerEv.attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
    canHandlerEv.attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);
    ready = true;

    tickHandler.attach(this, CFG_TICK_INTERVAL_BMS_ORION);
}

/**
 * Tear down the device in a safe way.
 */
void OrionBMS::tearDown()
{
    BatteryManager::tearDown();
    canHandlerEv.detach(this, CAN_MASKED_ID_1, CAN_MASK_1);
    canHandlerEv.detach(this, CAN_MASKED_ID_2, CAN_MASK_2);
}

void OrionBMS::handleCanFrame(CAN_FRAME *frame)
{
    byte *data = frame->data.bytes;

    switch (frame->id) {
    case CAN_ID_VALUES_1:
        packCurrent = ((data[0] << 8) | data[1]); // byte 0+1: pack current (0.1A)
        packVoltage = ((data[2] << 8) | data[3]); // byte 2+3: pack voltage (0.1V)
        packSummedVoltage = ((data[4] << 8) | data[5]); // byte 4+5: pack voltage (0.1V)
        flags = data[6];
        systemTemperature = data[7] * 10; // byte 7: temperature of BMS (1C)
        if (Logger::isDebug()) {
            Logger::debug(this, "pack current: %fA, voltage: %fV (summed: %fV), flags: %#08x, temp: %dC",
                    (float) packCurrent / 10.0F, (float) packVoltage / 10.0F, (float) packSummedVoltage / 10.0F, flags, systemTemperature);
        }
        break;

    case CAN_ID_VALUES_2:
//        if (checksumOk) {
            dischargeLimit = data[0]; // byte 0: pack discharge current limit (DCL) (1A)
            allowDischarge = (dischargeLimit > 0);
            chargeLimit = data[1]; // byte 1: pack charge current limit (CCL) (1A)
            allowCharge = (chargeLimit > 0);
            currentLimit = data[2]; // this is acutally a 2 byte flag ?!?!?
            //DCL Reduced Due To Low SOC (Bit #0)
            //DCL Reduced Due To High Cell Resistance (Bit #1)
            //DCL Reduced Due To Temperature (Bit #2)
            //DCL Reduced Due To Low Cell Voltage (Bit #3)
            //DCL Reduced Due To Low Pack Voltage (Bit #4)
            //DCL and CCL Reduced Due To Voltage Failsafe (Bit #6)
            //DCL and CCL Reduced Due To Communication Failsafe (Bit #7): This only applies if there are multiple BMS units connected together in series over CANBUS.
            //CCL Reduced Due To High SOC (Bit #9)
            //CCL Reduced Due To High Cell Resistance (Bit #10)
            //CCL Reduced Due To Temperature (Bit #11)
            //CCL Reduced Due To High Cell Voltage (Bit #12)
            //CCL Reduced Due To High Pack Voltage (Bit #13)
            //CCL Reduced Due To Charger Latch (Bit #14): This means the CCL is likely 0A because the charger has been turned off. This latch is removed when the Charge Power signal is removed and re-applied (ie: unplugging the car and plugging it back in).
            //CCL Reduced Due To Alternate Current Limit [MPI] (Bit #15)
            relayStatus = data[3];
            // Bit #1 (0x01): Discharge relay enabled
            // Bit #2 (0x02): Charge relay enabled
            // Bit #3 (0x04): Charger safety enabled
            // Bit #4 (0x08): Malfunction indicator active (DTC status)
            // Bit #5 (0x10): Multi-Purpose Input signal status
            // Bit #6 (0x20): Always-on signal status
            // Bit #7 (0x40): Is-Ready signal status
            // Bit #8 (0x80): Is-Charging signal status
            soc = data[4]; // byte 4: pack state of charge (0.5%)
            packAmphours = ((data[5] << 8) | data[6]); // byte 5+6: remaining Ah of pack (in 0.1Ah)

            if (Logger::isDebug()) {
                Logger::debug(this, "discharge limit: %dA, charge limit: %dA, limit flags: %#08x, relay: %#08x, soc: %f%%, pack capacity: %f Ah",
                        dischargeLimit, chargeLimit, currentLimit, relayStatus, (float) soc / 2.0F, (float) packAmphours / 10.0F);
            }
//        }
        break;

    case CAN_ID_VALUES_3:
        lowestCellVolts = ((data[0] << 8) | data[1]); // byte 0+1: low cell voltage (0.0001V)
        lowestCellVoltsId = data[2]; // byte 2: low cell voltage ID (0-180)
        highestCellVolts = ((data[3] << 8) | data[4]); // byte 3+4: high cell voltage (0.0001V)
        highestCellVoltsId = data[5]; // byte 5: high cell voltage ID (0-180)
        averageCellVolts = ((data[6] << 8) | data[7]); // byte 6+7: average cell voltage (0.0001V)
        if (Logger::isDebug()) {
            Logger::debug(this, "low cell: %fV (%d), high cell: %fV (%d), avg: %fV",
                    (float) lowestCellVolts / 10000.0F, lowestCellVoltsId, (float) highestCellVolts / 10000.0F, highestCellVoltsId, (float) averageCellVolts / 10000.0F);
        }
        break;

    case CAN_ID_VALUES_4:
        lowestCellResistance = ((data[0] << 8) | data[1]); // byte 0+1: low cell resistance (0.01 mOhm)
        lowestCellResistanceId = data[2]; // byte 2: low cell resistance ID (0-180)
        highestCellResistance = ((data[3] << 8) | data[4]); // byte 3+4: high cell resistance (0.01 mOhm)
        highestCellResistanceId = data[5]; // byte 5: high cell resistance ID (0-180)
        averageCellResistance = ((data[6] << 8) | data[7]); // byte 6+7: average cell resistance (0.01 mOhm)
        if (Logger::isDebug()) {
            Logger::debug(this, "low cell: %fmOhm (%d), high cell: %fmOhm (%d), avg: %fmOhm",
                    (float) lowestCellResistance / 100.0F, lowestCellResistanceId, (float) highestCellResistance / 100.0F, highestCellResistanceId, (float) averageCellResistance / 100.0F);
        }
        break;

    case CAN_ID_CELL_VOLTAGE:
        break;

    case CAN_ID_CELL_RESISTANCE:
        break;

//        lowestCellTempId, highestCellTempId; // 0-254, 255=undefined
//        packResistance = ((data[2] << 8) | data[3]); // byte 2+3: pack resistance (1mOhm)
//        packOpenVoltage = ((data[4] << 8) | data[5]); // byte 4+5: pack open voltage (0.1V)
//        packHealth = data[0]; // byte 0: pack state of health (1%)
//        packDepthOfDischarge = data[1]; // byte 1: pack depth of discharge (0.5%)
//        packCycles = ((data[2] << 8) | data[3]); // byte 2+3: pack cycles (1 Cycle)

    }
}

void OrionBMS::handleTick()
{
    BatteryManager::handleTick();
}

DeviceId OrionBMS::getId()
{
    return (ORIONBMS);
}

bool OrionBMS::hasPackVoltage()
{
    return true;
}

bool OrionBMS::hasPackCurrent()
{
    return true;
}

bool OrionBMS::hasCellTemperatures()
{
    return true;
}

bool OrionBMS::hasSoc()
{
    return true;
}

bool OrionBMS::hasChargeLimit()
{
    return true;
}

bool OrionBMS::hasDischargeLimit()
{
    return true;
}

bool OrionBMS::hasAllowCharging()
{
    return true;
}

bool OrionBMS::hasAllowDischarging()
{
    return true;
}

bool OrionBMS::hasCellVoltages()
{
    return true;
}

bool OrionBMS::hasCellResistance()
{
    return true;
}
