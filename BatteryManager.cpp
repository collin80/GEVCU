/*
 * BatteryManager.cpp
 *
 * Parent class for all battery management/monitoring systems
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

#include "BatteryManager.h"

BatteryManager::BatteryManager() :
        Device()
{
    packVoltage = 0;
    packCurrent = 0;
    soc = 0;
    packAmphours = 0;
    allowCharge = allowDischarge = false;
    chargeLimit = dischargeLimit = 0;
    systemTemperature = 0;
    lowestCellTemp = highestCellTemp = 0;
    lowestCellTempId = highestCellTempId = ID_UNKNOWN;
    lowestCellVolts = highestCellVolts = averageCellVolts = 0;
    lowestCellVoltsId = highestCellVoltsId = ID_UNKNOWN;
    lowestCellResistance = highestCellResistance = averageCellResistance = 0;
    lowestCellResistanceId = highestCellResistanceId = ID_UNKNOWN;
    packHealth = 0;
    packCycles = packResistance = 0;
    chargerEnabled = false;
}

void BatteryManager::setup()
{
    Device::setup();

}

void BatteryManager::tearDown()
{
    Device::tearDown();

    allowCharge = false;
    allowDischarge = false;
    chargeLimit = 0;
    dischargeLimit = 0;
    chargerEnabled = false;
}

DeviceType BatteryManager::getType()
{
    return DEVICE_BMS;
}

void BatteryManager::handleTick()
{
}

bool BatteryManager::hasPackVoltage()
{
    return false;
}

bool BatteryManager::hasPackCurrent()
{
    return false;
}

bool BatteryManager::hasSoc()
{
    return false;
}

bool BatteryManager::hasChargeLimit()
{
    return false;
}

bool BatteryManager::hasDischargeLimit()
{
    return false;
}

bool BatteryManager::hasAllowCharging()
{
    return false;
}

bool BatteryManager::hasAllowDischarging()
{
    return false;
}

bool BatteryManager::hasCellTemperatures()
{
    return false;
}


bool BatteryManager::hasCellVoltages()
{
    return false;
}

bool BatteryManager::hasCellResistance()
{
    return false;
}

bool BatteryManager::hasPackHealth()
{
    return false;
}

bool BatteryManager::hasPackCycles()
{
    return false;
}

bool BatteryManager::hasPackResistance()
{
    return false;
}

bool BatteryManager::hasChargerEnabled()
{
    return false;
}

uint16_t BatteryManager::getPackVoltage()
{
    return packVoltage;
}

int16_t BatteryManager::getPackCurrent()
{
    return packCurrent;
}

uint8_t BatteryManager::getSoc()
{
    return soc;
}

uint16_t BatteryManager::getDischargeLimit()
{
    return dischargeLimit;
}

uint16_t BatteryManager::getChargeLimit()
{
    return chargeLimit;
}

bool BatteryManager::isChargeAllowed()
{
    return allowCharge;
}

bool BatteryManager::isDischargeAllowed()
{
    return allowDischarge;
}

int16_t BatteryManager::getLowestCellTemp()
{
    return lowestCellTemp;
}

int16_t BatteryManager::getHighestCellTemp()
{
    return highestCellTemp;
}

uint16_t BatteryManager::getLowestCellVolts()
{
    return lowestCellVolts;
}

uint16_t BatteryManager::getHighestCellVolts()
{
    return highestCellVolts;
}

uint16_t BatteryManager::getAverageCellVolts()
{
    return averageCellVolts;
}

uint16_t BatteryManager::getLowestCellResistance()
{
    return lowestCellResistance;
}

uint16_t BatteryManager::getHighestCellResistance()
{
    return highestCellResistance;
}

uint16_t BatteryManager::getAverageCellResistance()
{
    return averageCellVolts;
}

uint8_t BatteryManager::getLowestCellTempId()
{
    return lowestCellTempId;
}

uint8_t BatteryManager::getHighestCellTempId()
{
    return highestCellTempId;
}

uint8_t BatteryManager::getLowestCellVoltsId()
{
    return lowestCellVoltsId;
}

uint8_t BatteryManager::getHighestCellVoltsId()
{
    return highestCellVoltsId;
}

uint8_t BatteryManager::getLowestCellResistanceId()
{
    return lowestCellResistanceId;
}

uint8_t BatteryManager::getHighestCellResistanceId()
{
    return highestCellResistanceId;
}

uint8_t BatteryManager::getPackHealth()
{
    return packHealth;
}

uint16_t BatteryManager::getPackCycles()
{
    return packCycles;
}

uint16_t BatteryManager::getPackResistance()
{
    return packResistance;
}

uint8_t BatteryManager::getSystemTemperature()
{
    return systemTemperature;
}

bool BatteryManager::isChargerEnabled()
{
    return chargerEnabled;
}
