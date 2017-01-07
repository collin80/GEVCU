/*
 * constants.h
 *
 * Defines the global / application wide constants
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
 *      Author: Michael Neuweiler
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

namespace Constants
{
// misc
static const char* ichipCommandPrefix = "AT+i";
static const char* ichipErrorString = "I/ERROR";
static const char* disconnect = "_DISCONNECT_";

// messages
static const char* valueOutOfRange = "value out of range: %ld";
static const char* normalOperation = "normal operation restored";

/*
 * The following constants are names of ichip variables (see xml files in website)
 * They must be <=20 characters - this is why the content sometimes differs from the internal param name.
 */

// configuration throttle + motor
static const char* numberPotMeters = "numberPotMeters";
static const char* throttleSubType = "throttleSubType";
static const char* minimumLevel = "minimumLevel";
static const char* minimumLevel2 = "minimumLevel2";
static const char* maximumLevel = "maximumLevel";
static const char* maximumLevel2 = "maximumLevel2";
static const char* positionRegenMaximum = "positionRegenMaximum";
static const char* positionRegenMinimum = "positionRegenMinimum";
static const char* positionForwardMotionStart = "positionForwardStart";
static const char* positionHalfPower = "positionHalfPower";
static const char* minimumRegen = "minimumRegen";
static const char* maximumRegen = "maximumRegen";
static const char* creep = "creep";
static const char* brakeMinimumLevel = "brakeMinimumLevel";
static const char* brakeMaximumLevel = "brakeMaximumLevel";
static const char* brakeMinimumRegen = "brakeMinimumRegen";
static const char* brakeMaximumRegen = "brakeMaximumRegen";
static const char* speedMax = "speedMax";
static const char* torqueMax = "torqueMax";
static const char* logLevel = "logLevel";
static const char* nominalVolt = "nominalVolt";
static const char* motorMode = "motorMode";
static const char* invertDirection = "invertDirection";
static const char* slewRate = "slewRate";

static const char* maxMechanicalPowerMotor = "maxMechPowerMotor";
static const char* maxMechanicalPowerRegen = "maxMechPowerRegen";
static const char* dcVoltLimitMotor = "dcVoltLimitMotor";
static const char* dcVoltLimitRegen = "dcVoltLimitRegen";
static const char* dcCurrentLimitMotor = "dcCurrentLimitMotor";
static const char* dcCurrentLimitRegen = "dcCurrentLimitRegen";
static const char* enableOscillationLimiter = "enableOscLimiter";

// input
static const char* absInput = "absInput";
static const char* reverseInput = "reverseInput";
static const char* enableInput = "enableInput";
static const char* chargePowerAvailableInput = "chargePwrAvailInput";
static const char* interlockInput = "interlockInput";

// output
static const char* brakeLightOutput = "brakeLightOutput";
static const char* reverseLightOutput = "reverseLightOutput";
static const char* powerSteeringOutput = "powerSteeringOutput";
static const char* unusedOutput = "unusedOutput";
static const char* prechargeMillis = "prechargeMillis";
static const char* prechargeRelayOutput = "prechargeRelayOutput";
static const char* mainContactorOutput = "mainContactorOutput";
static const char* secondaryContactorOutput = "secondaryContOutput";
static const char* enableMotorOutput = "enableMotorOutput";
static const char* coolingFanOutput = "coolingFanOutput";
static const char* coolingTempOn = "coolingTempOn";
static const char* coolingTempOff = "coolingTempOff";
static const char* fastChargeContactorOutput = "fastChargeContOutput";
static const char* enableChargerOutput = "enableChargerOutput";
static const char* enableDcDcOutput = "enableDcDcOutput";
static const char* enableHeaterOutput = "enableHeaterOutput";
static const char* heaterValveOutput = "heaterValveOutput";
static const char* heaterPumpOutput = "heaterPumpOutput";
static const char* coolingPumpOutput = "coolingPumpOutput";
static const char* warningOutput = "warningOutput";
static const char* powerLimitationOutput = "powerLimitOutput";

// charger
static const char* maximumInputCurrent = "maximumInputCurrent";
static const char* constantCurrent = "constantCurrent";
static const char* constantVoltage = "constantVoltage";
static const char* terminateCurrent = "terminateCurrent";
static const char* minimumBatteryVoltage = "minBatteryVoltage";
static const char* maximumBatteryVoltage = "maxBatteryVoltage";
static const char* minimumTemperature = "minimumTemperature";
static const char* maximumTemperature = "maximumTemperature";
static const char* maximumAmpereHours = "maximumAmpereHours";
static const char* maximumChargeTime = "maximumChargeTime";
static const char* deratingRate = "deratingRate";
static const char* deratingReferenceTemperature = "deratingReferenceTmp";
static const char* hystereseStopTemperature = "hystereseStopTemp";
static const char* hystereseResumeTemperature = "hystereseResumeTemp";

// dc dc converter
static const char* dcDcMode = "dcDcMode";
static const char* lowVoltageCommand = "lowVoltageCommand";
static const char* hvUndervoltageLimit = "hvUndervoltageLimit";
static const char* lvBuckModeCurrentLimit = "lvBuckCurrentLimit";
static const char* hvBuckModeCurrentLimit = "hvBuckCurrentLimit";
static const char* highVoltageCommand = "highVoltageCommand";
static const char* lvUndervoltageLimit = "lvUndervoltageLimit";
static const char* lvBoostModeCurrentLimit = "lvBoostCurrentLimit";
static const char* hvBoostModeCurrentLimit = "hvBoostCurrentLimit";

// status + dashboard
static const char* timeRunning = "timeRunning";
static const char* torqueRequested = "torqueRequested";
static const char* torqueActual = "torqueActual";
static const char* throttle = "throttle";
static const char* brake = "brake";
static const char* speedActual = "speedActual";
static const char* dcVoltage = "dcVoltage";
static const char* dcCurrent = "dcCurrent";
static const char* energyConsumption = "energyConsumption";
static const char* bitfield1 = "bitfield1";
static const char* bitfield2 = "bitfield2";
static const char* bitfield3 = "bitfield3";
static const char* systemState = "systemState";
static const char* gear = "gear";
static const char* temperatureMotor = "temperatureMotor";
static const char* temperatureController = "temperatureController";
static const char* mechanicalPower = "mechanicalPower";
static const char* dcDcHvVoltage = "dcDcHvVoltage";
static const char* dcDcLvVoltage = "dcDcLvVoltage";
static const char* dcDcHvCurrent = "dcDcHvCurrent";
static const char* dcDcLvCurrent = "dcDcLvCurrent";
static const char* dcDcTemperature = "dcDcTemperature";
static const char* chargerInputVoltage = "chargerInputVoltage";
static const char* chargerInputCurrent = "chargerInputCurrent";
static const char* chargerBatteryVoltage = "chargerBatteryVoltage";
static const char* chargerBatteryCurrent = "chargerBatteryCurrent";
static const char* chargerTemperature = "chargerTemperature";

static const char* flowCoolant = "flowCoolant";
static const char* flowHeater = "flowHeater";
static const char* heaterPower = "heaterPower";
static const char* temperatureBattery[] = { "temperatureBattery1", "temperatureBattery2", "temperatureBattery3", "temperatureBattery4",
        "temperatureBattery5", "temperatureBattery6" };
static const char* temperatureCoolant = "temperatureCoolant";
static const char* temperatureHeater = "temperatureHeater";
static const char* temperatureExterior = "temperatureExterior";

static const char* torqueRange = "torqueRange";
static const char* rpmRange = "rpmRange";
static const char* currentRange = "currentRange";
static const char* motorTempRange = "motorTempRange";
static const char* controllerTempRange = "controllerTempRange";
static const char* batteryRangeLow = "batteryRangeLow";
static const char* batteryRangeHigh = "batteryRangeHigh";
static const char* energyRange = "energyRange";
static const char* powerRange = "powerRange";
static const char* enableRegen = "enableRegen";
static const char* enableHeater = "enableHeater";
static const char* powerSteering = "powerSteering";
static const char* enableCreep = "enableCreep";

}

#endif /* CONSTANTS_H_ */
