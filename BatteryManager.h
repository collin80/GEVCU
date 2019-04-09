/*
 * BatteryManager.h
 *
 * Parent class for battery management / monitoring systems
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

#ifndef BATTMANAGE_H_
#define BATTMANAGE_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"

#define ID_UNKNOWN 255

class BatteryManager : public Device
{
public:
    BatteryManager();
    void setup();
    void tearDown();
    DeviceType getType();
    void handleTick();
    //Derived classes must implment these functions to tell what they support
    virtual bool hasPackVoltage();
    virtual bool hasPackCurrent();
    virtual bool hasSoc();
    virtual bool hasChargeLimit();
    virtual bool hasDischargeLimit();
    virtual bool hasAllowCharging();
    virtual bool hasAllowDischarging();
    virtual bool hasCellTemperatures();
    virtual bool hasCellVoltages();
    virtual bool hasCellResistance();
    virtual bool hasPackHealth();
    virtual bool hasPackCycles();
    virtual bool hasPackResistance();

    uint16_t getPackVoltage(); // in 0.1V
    int16_t getPackCurrent(); // in 0.1A
    uint8_t getSoc(); // in 0.5%
    uint16_t getDischargeLimit(); // in 1A
    uint16_t getChargeLimit(); // in 1A
    bool isChargeAllowed();
    bool isDischargeAllowed();
    int16_t getLowestCellTemp(); // in 0.1C
    int16_t getHighestCellTemp(); // in 0.1C
    uint16_t getLowestCellVolts(); // in mV
    uint16_t getHighestCellVolts(); // in mV
    uint16_t getAverageCellVolts(); // in 0.0001V
    uint16_t getLowestCellResistance(); // in 0.01mOhm
    uint16_t getHighestCellResistance(); // in 0.01mOhm
    uint16_t getAverageCellResistance(); // in 0.01mOhm
    uint8_t getLowestCellTempId();
    uint8_t getHighestCellTempId();
    uint8_t getLowestCellVoltsId();
    uint8_t getHighestCellVoltsId();
    uint8_t getLowestCellResistanceId();
    uint8_t getHighestCellResistanceId();
    uint8_t getPackHealth(); // in 1%
    uint16_t getPackCycles();
    uint16_t getPackResistance(); // in 1mOhm
    uint8_t getSystemTemperature(); // in 1C

protected:
    uint16_t packVoltage; //tenths of a volt
    int16_t packCurrent; //tenths of an amp
    uint8_t soc; //state of charge in 0.5%
    uint16_t packAmphours; // Ah of pack in 0.1Ah
    uint16_t dischargeLimit, chargeLimit; // in 1A
    bool allowCharge, allowDischarge;
    int8_t systemTemperature; // in 1C
    int16_t lowestCellTemp, highestCellTemp; // in 0.1C
    uint8_t lowestCellTempId, highestCellTempId; // 0-254, 255=undefined
    uint16_t lowestCellVolts, highestCellVolts, averageCellVolts; // in 0.0001V
    uint8_t lowestCellVoltsId, highestCellVoltsId; // 0-254, 255=undefined
    uint16_t lowestCellResistance, highestCellResistance, averageCellResistance; // in 0.01mOhm
    uint8_t lowestCellResistanceId, highestCellResistanceId; // 0-254, 255=undefined
    uint8_t packHealth; // pack health (1%)
    uint16_t packCycles; // number of total pack cycles
    uint16_t packResistance; // pack resistance (1 mOhm)
private:
};

#endif
