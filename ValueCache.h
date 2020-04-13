/*
 * ValueCache.h
 *
 *  Created on: 10 Apr 2020
 *      Author: michaeln
 */

#ifndef VALUECACHE_H_
#define VALUECACHE_H_

#include <Arduino.h>
#include "config.h"

/**
 * Cache of param values to avoid sending an update unless changed
 *
 * NOTE: Keep the order and content in line with Websocket.cpp and WifiEsp32.cpp/.h and GEVCU_ESP32Web's code.
 */
class ValueCache
{
public:
    ValueCache();
    void clear();

    uint8_t systemState;
    uint32_t timeRunning;
    int16_t torqueActual;
    int16_t speedActual;
    int16_t throttle;

    uint16_t dcVoltage;
    int16_t dcCurrent;
    int16_t acCurrent;
    int16_t temperatureMotor;
    int16_t temperatureController;
    int16_t mechanicalPower;

    uint32_t bitfieldMotor;
    uint32_t bitfieldBms;
    uint32_t bitfieldIO;

    uint16_t dcDcHvVoltage;
    uint16_t dcDcLvVoltage;
    int16_t dcDcHvCurrent;
    int16_t dcDcLvCurrent;
    int16_t dcDcTemperature;

    uint16_t chargerInputVoltage;
    uint16_t chargerInputCurrent;
    uint16_t chargerBatteryVoltage;
    uint16_t chargerBatteryCurrent;
    int16_t chargerTemperature;
    int16_t maximumSolarCurrent;
    uint16_t chargeHoursRemain;
    uint16_t chargeMinsRemain;
    uint16_t chargeLevel;

    uint32_t flowCoolant;
    uint32_t flowHeater;
    uint16_t heaterPower;
    int16_t temperatureBattery[CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS];
    int16_t temperatureCoolant;
    int16_t temperatureHeater;
    int16_t temperatureExterior;

    bool powerSteering;
    bool enableRegen;
    bool enableHeater;
    bool enableCreep;
    int16_t cruiseControlSpeed;
    bool enableCruiseControl;

    uint16_t soc;
    uint16_t dischargeLimit;
    uint16_t chargeLimit;
    bool chargeAllowed;
    bool dischargeAllowed;
    int16_t lowestCellTemp;
    int16_t highestCellTemp;
    uint16_t lowestCellVolts;
    uint16_t highestCellVolts;
    uint16_t averageCellVolts;
    uint16_t deltaCellVolts;
    uint16_t lowestCellResistance;
    uint16_t highestCellResistance;
    uint16_t averageCellResistance;
    uint16_t deltaCellResistance;
    uint8_t lowestCellTempId;
    uint8_t highestCellTempId;
    uint8_t lowestCellVoltsId;
    uint8_t highestCellVoltsId;
    uint8_t lowestCellResistanceId;
    uint8_t highestCellResistanceId;
    uint16_t packResistance;
    uint8_t packHealth;
    uint16_t packCycles;
    uint8_t bmsTemperature;
};

extern ValueCache valueCache;

#endif /* VALUECACHE_H_ */
