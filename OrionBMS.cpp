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
    canTickCounter = 0;
}

void OrionBMS::setup()
{
    BatteryManager::setup();

    tickHandler.attach(this, CFG_TICK_INTERVAL_BMS_ORION);
    canHandlerEv.attach(this, ORION_CAN_MASKED_ID, ORION_CAN_MASK, false);
    ready = true;
}

/**
 * Tear down the device in a safe way.
 */
void OrionBMS::tearDown()
{
    BatteryManager::tearDown();
    canHandlerEv.detach(this, ORION_CAN_MASKED_ID, ORION_CAN_MASK);
}

/*
 * Process event from the tick handler.
 */
void OrionBMS::handleTick()
{
    BatteryManager::handleTick(); // call parent

    // check if we get a message, if not received for 10 sec, the BMS is not ready
    if (canTickCounter < 20) {
        canTickCounter++;
    } else {
        ready = false;
        running = false;
        if (status.getSystemState() == Status::charging) {
            logger.error(this, "no message from BMS received for 10sec");
            status.setSystemState(Status::error);
        }
    }
}

void OrionBMS::handleCanFrame(CAN_FRAME *frame)
{
    switch (frame->id) {
    case ORION_CAN_ID_PACK:
        processPack(frame->data.bytes);
        break;
    case ORION_CAN_ID_LIMITS_SOC:
        processLimitsSoc(frame->data.bytes);
        break;
    case ORION_CAN_ID_CELL_VOLTAGE:
        processCellVoltage(frame->data.bytes);
        break;
    case ORION_CAN_ID_CELL_RESISTANCE:
        processCellResistance(frame->data.bytes);
        break;
    case ORION_CAN_ID_HEALTH:
        processHealth(frame->data.bytes);
        break;
    }
}

void OrionBMS::processPack(uint8_t data[])
{
    canTickCounter = 0;
    packCurrent = ((data[0] << 8) | data[1]); // byte 0+1: pack current (0.1A)
    packVoltage = ((data[2] << 8) | data[3]); // byte 2+3: pack voltage (0.1V)
    packSummedVoltage = ((data[4] << 8) | data[5]); // byte 4+5: pack voltage (0.1V)
    flags = data[6];
    status.bmsVoltageFailsafe = (flags & voltageFailsafe) ? true : false;
    status.bmsCurrentFailsafe = (flags & currentFailsafe) ? true : false;
    status.bmsDepleted = (flags & depleted) ? true : false;
    status.bmsBalancingActive = (flags & balancingActive) ? true : false;
    status.bmsDtcWeakCellFault = (flags & dtcWeakCellFault) ? true : false;
    status.bmsDtcLowCellVolage = (flags & dtcLowCellVolage) ? true : false;
    status.bmsDtcHVIsolationFault = (flags & dtcHVIsolationFault) ? true : false;
    status.bmsDtcVoltageRedundancyFault = (flags & dtcVoltageRedundancyFault) ? true : false;
    systemTemperature = data[7]; // byte 7: temperature of BMS (1C)
    if (logger.isDebug()) {
        logger.debug(this, "pack current: %fA, voltage: %fV (summed: %fV), flags: %#08x, temp: %dC", (float) packCurrent / 10.0F,
                (float) packVoltage / 10.0F, (float) packSummedVoltage / 10.0F, flags, systemTemperature);
    }
}

void OrionBMS::processLimitsSoc(uint8_t data[])
{
    canTickCounter = 0;
    dischargeLimit = ((data[0] << 8) | data[1]); // byte 0+1: pack discharge current limit (DCL) (1A)
    allowDischarge = (dischargeLimit > 0);
    chargeLimit = ((data[2] << 8) | data[3]); // byte 2+3: pack charge current limit (CCL) (1A)
    allowCharge = (chargeLimit > 0);
    currentLimit = ((data[4] << 8) | data[5]); // byte 4+5: bitfield with reason for current limmitation
    status.bmsDclLowSoc = (currentLimit & dclLowSoc) ? true : false;
    status.bmsDclHighCellResistance = (currentLimit & dclHighCellResistance) ? true : false;
    status.bmsDclTemperature = (currentLimit & dclTemperature) ? true : false;
    status.bmsDclLowCellVoltage = (currentLimit & dclLowCellVoltage) ? true : false;
    status.bmsDclLowPackVoltage = (currentLimit & dclLowPackVoltage) ? true : false;
    status.bmsDclCclVoltageFailsafe = (currentLimit & dclCclVoltageFailsafe) ? true : false;
    status.bmsDclCclCommunication = (currentLimit & dclCclCommunication) ? true : false;
    status.bmsCclHighSoc = (currentLimit & cclHighSoc) ? true : false;
    status.bmsCclHighCellResistance = (currentLimit & cclHighCellResistance) ? true : false;
    status.bmsCclTemperature = (currentLimit & cclTemperature) ? true : false;
    status.bmsCclHighCellVoltage = (currentLimit & cclHighCellVoltage) ? true : false;
    status.bmsCclHighPackVoltage = (currentLimit & cclHighPackVoltage) ? true : false;
    status.bmsCclChargerLatch = (currentLimit & cclChargerLatch) ? true : false;
    status.bmsCclAlternate = (currentLimit & cclAlternate) ? true : false;
    relayStatus = data[6];
    status.bmsRelayDischarge = (relayStatus & relayDischarge) ? true : false; // Bit #1 (0x01): Discharge relay enabled
    status.bmsRelayCharge = (relayStatus & relayCharge) ? true : false; // Bit #2 (0x02): Charge relay enabled
    chargerEnabled = (relayStatus & chagerSafety) ? true : false; // Bit #3 (0x04): Charger safety enabled
    status.bmsChagerSafety = chargerEnabled;
    status.bmsDtcPresent = (relayStatus & dtcPresent) ? true : false; // Bit #4 (0x08): Malfunction indicator active (DTC status)
    if (logger.isDebug()) {
        logger.debug(this, "discharge limit: %dA, charge limit: %dA, limit flags: %#08x, relay: %#08x", dischargeLimit, chargeLimit, currentLimit,
                relayStatus);
    }
}

void OrionBMS::processCellVoltage(uint8_t data[])
{
    canTickCounter = 0;
    lowestCellVolts = ((data[0] << 8) | data[1]); // byte 0+1: low cell voltage (0.0001V)
    lowestCellVoltsId = data[2]; // byte 2: low cell voltage ID (0-180)
    highestCellVolts = ((data[3] << 8) | data[4]); // byte 3+4: high cell voltage (0.0001V)
    highestCellVoltsId = data[5]; // byte 5: high cell voltage ID (0-180)
    averageCellVolts = ((data[6] << 8) | data[7]); // byte 6+7: average cell voltage (0.0001V)
    if (logger.isDebug()) {
        logger.debug(this, "low cell: %fV (%d), high cell: %fV (%d), avg: %fV", (float) lowestCellVolts / 10000.0F, lowestCellVoltsId,
                (float) highestCellVolts / 10000.0F, highestCellVoltsId, (float) averageCellVolts / 10000.0F);
    }
}

void OrionBMS::processCellResistance(uint8_t data[])
{
    canTickCounter = 0;
    lowestCellResistance = ((data[0] << 8) | data[1]); // byte 0+1: low cell resistance (0.01 mOhm)
    lowestCellResistanceId = data[2]; // byte 2: low cell resistance ID (0-180)
    highestCellResistance = ((data[3] << 8) | data[4]); // byte 3+4: high cell resistance (0.01 mOhm)
    highestCellResistanceId = data[5]; // byte 5: high cell resistance ID (0-180)
    averageCellResistance = ((data[6] << 8) | data[7]); // byte 6+7: average cell resistance (0.01 mOhm)
    if (logger.isDebug()) {
        logger.debug(this, "low cell: %fmOhm (%d), high cell: %fmOhm (%d), avg: %fmOhm", (float) lowestCellResistance / 100.0F,
                lowestCellResistanceId, (float) highestCellResistance / 100.0F, highestCellResistanceId, (float) averageCellResistance / 100.0F);
    }
}
void OrionBMS::processHealth(uint8_t data[])
{
    canTickCounter = 0;
    packHealth = data[0]; // byte 0: pack health (1%)
    packCycles = ((data[1] << 8) | data[2]); // byte 1+2: number of total pack cycles
    packResistance = ((data[3] << 8) | data[4]); // byte 3+4: pack resistance (1 mOhm)
    lowestCellTemp = data[5] * 10;
    highestCellTemp = data[6] * 10;
    soc = data[7]; // byte 7: pack state of charge (0.5%)
    if (logger.isDebug()) {
        logger.debug(this, "pack health: %d, pack cycles: %d, pack Resistance: %dmOhm, low temp: %d, high temp: %d, soc: %.1f", packHealth,
                packCycles, packResistance, lowestCellTemp, highestCellTemp, (float) soc / 2.0F);
    }
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

bool OrionBMS::hasPackHealth()
{
    return true;
}

bool OrionBMS::hasPackCycles()
{
    return true;
}

bool OrionBMS::hasPackResistance()
{
    return true;
}

bool OrionBMS::hasChargerEnabled()
{
    return true;
}
