/*
 * cpp
 *
 *  Created on: 10 Apr 2020
 *      Author: michaeln
 */

#include "ValueCache.h"

ValueCache valueCache;

ValueCache::ValueCache()
{
    clear();
}

/**
 * \brief Initialize parameter cache so all params are sent when connecting
 *
 */
void ValueCache::clear()
{
    systemState = 0;
    timeRunning = millis(); // this way less important data is sent one second later
    torqueActual = -1;
    speedActual = -1;
    throttle = -1;

    dcVoltage = -1;
    dcCurrent = -1;
    acCurrent = -1;
    temperatureMotor = -1;
    temperatureController = -1;
    mechanicalPower = 0;

    bitfieldMotor = 0;
    bitfieldBms = 0;
    bitfieldIO = 0;

    dcVoltageMin = 0;
    dcVoltageMax = 0;
    dcCurrentMin = 0;
    dcCurrentMax = 0;
    temperatureMotorMax = 0;
    temperatureControllerMax = 0;

    dcDcHvVoltage = 0;
    dcDcLvVoltage = 0;
    dcDcHvCurrent = 0;
    dcDcLvCurrent = 0;
    dcDcTemperature = 0;

    chargerInputVoltage = 0;
    chargerInputCurrent = 0;
    chargerBatteryVoltage = 0;
    chargerBatteryCurrent = 0;
    chargerTemperature = 0;
    maximumSolarCurrent = -1;
    chargeHoursRemain = 0;
    chargeMinsRemain = 0;
    chargeLevel = 0;

    flowCoolant = 0;
    flowHeater = 0;
    heaterPower = 0;
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        temperatureBattery[i] = CFG_NO_TEMPERATURE_DATA;
    }
    temperatureCoolant = CFG_NO_TEMPERATURE_DATA;
    temperatureHeater = CFG_NO_TEMPERATURE_DATA;
    temperatureExterior = CFG_NO_TEMPERATURE_DATA;

    powerSteering = false;
    enableRegen = false;
    enableHeater = false;
    enableCreep = false;
    cruiseControlSpeed = 0;
    enableCruiseControl = false;

    soc = 0;
    dischargeLimit = 0;
    chargeLimit = 0;
    chargeAllowed = 0;
    dischargeAllowed = 0;
    lowestCellTemp = 0;
    highestCellTemp = 0;
    lowestCellVolts = 0;
    highestCellVolts = 0;
    averageCellVolts = 0;
    deltaCellVolts = 0;
    lowestCellResistance = 0;
    highestCellResistance = 0;
    averageCellResistance = 0;
    deltaCellResistance = 0;
    lowestCellTempId = 0;
    highestCellTempId = 0;
    lowestCellVoltsId = 0;
    highestCellVoltsId = 0;
    lowestCellResistanceId = 0;
    highestCellResistanceId = 0;
    packResistance = 0;
    packHealth = 0;
    packCycles = 0;
    bmsTemperature = 0;
}
