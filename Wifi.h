/*
 * Wifi.h
 *
 * Parent class for Wifi devices
 *
 Copyright (c) 2020 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#ifndef WIFI_H_
#define WIFI_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceManager.h"
#include "PotThrottle.h"
#include "CanThrottle.h"
#include "PotBrake.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "ELM327Processor.h"
#include "BrusaDMC5.h"
#include "ValueCache.h"

class WifiConfiguration: public DeviceConfiguration
{
};

class Wifi: public Device
{
public:
    Wifi();
    virtual void process();

    void loadConfiguration();
    void saveConfiguration();

protected:
    void processParameterChange(String input);
    void processParameterChange(String key, String value);
    void loadParameters();
    virtual void setParam(String paramName, String value);

    USARTClass *serialInterface; //Allows to retarget the serial port we use

private:
    bool processParameterChangeThrottle(String key, String value);
    bool processParameterChangeBrake(String key, String value);
    bool processParameterChangeMotor(String key, String value);
    bool processParameterChangeCharger(String key, String value);
    bool processParameterChangeDcDc(String key, String value);
    bool processParameterChangeSystemIO(String key, String value);
    bool processParameterChangeDevices(String key, String value);
    void loadParametersThrottle();
    void loadParametersBrake();
    void loadParametersMotor();
    void loadParametersCharger();
    void loadParametersDcDc();
    void loadParametersSystemIO();
    void loadParametersDevices();
    void loadParametersDashboard();
    void setParam(String paramName, int32_t value);
    void setParam(String paramName, int16_t value);
    void setParam(String paramName, uint32_t value);
    void setParam(String paramName, uint16_t value);
    void setParam(String paramName, uint8_t value);
    void setParam(String paramName, float value, int precision);
    void setParam(String paramName, double value);

    char buffer[30]; // a buffer for various string conversions

    /*
     * The following constants are names of ichip variables (see xml files in website)
     * They must be <=20 characters - this is why the content sometimes differs from the internal param name.
     */

    // configuration throttle + motor
    const String numberPotMeters = "numberPotMeters";
    const String throttleSubType = "throttleSubType";
    const String minimumLevel = "minimumLevel";
    const String minimumLevel2 = "minimumLevel2";
    const String maximumLevel = "maximumLevel";
    const String maximumLevel2 = "maximumLevel2";
    const String positionRegenMaximum = "positionRegenMaximum";
    const String positionRegenMinimum = "positionRegenMinimum";
    const String positionForwardMotionStart = "positionForwardStart";
    const String positionHalfPower = "positionHalfPower";
    const String minimumRegen = "minimumRegen";
    const String maximumRegen = "maximumRegen";
    const String throttleAdcPin1 = "throttleAdcPin1";
    const String throttleAdcPin2 = "throttleAdcPin2";
    const String brakeAdcPin1 = "brakeAdcPin1";
    const String brakeAdcPin2 = "brakeAdcPin2";
    const String carType = "carType";
    const String creepLevel = "creepLevel";
    const String creepSpeed = "creepSpeed";
    const String brakeMinimumLevel = "brakeMinimumLevel";
    const String brakeMaximumLevel = "brakeMaximumLevel";
    const String brakeMinimumRegen = "brakeMinimumRegen";
    const String brakeMaximumRegen = "brakeMaximumRegen";
    const String speedMax = "speedMax";
    const String torqueMax = "torqueMax";
    const String logLevel = "logLevel";
    const String systemType = "systemType";
    const String nominalVolt = "nominalVolt";
    const String motorMode = "motorMode";
    const String invertDirection = "invertDirection";
    const String slewRate = "slewRate";
    const String brakeHold = "brakeHold";
    const String brakeHoldForceCoefficient = "brakeHoldForceCoef";
    const String reversePercent = "reversePercent";
    const String gearChangeSupport = "gearChangeSupport";
    const String cruiseKp = "cruiseKp";
    const String cruiseKi = "cruiseKi";
    const String cruiseKd = "cruiseKd";
    const String cruiseUseRpm = "cruiseUseRpm";
    const String cruiseStepDelta = "cruiseStepDelta";
    const String cruiseLongPressDelta = "cruiseLongPressDelta";
    const String cruiseSpeedSet = "cruiseSpeedSet";
    const String cruiseSpeedStep = "cruiseSpeedStep";

    const String maxMechanicalPowerMotor = "maxMechPowerMotor";
    const String maxMechanicalPowerRegen = "maxMechPowerRegen";
    const String dcVoltLimitMotor = "dcVoltLimitMotor";
    const String dcVoltLimitRegen = "dcVoltLimitRegen";
    const String dcCurrentLimitMotor = "dcCurrentLimitMotor";
    const String dcCurrentLimitRegen = "dcCurrentLimitRegen";
    const String enableOscillationLimiter = "enableOscLimiter";
    // input
    const String gearChangeInput = "gearChange";
    const String absInput = "abs";
    const String reverseInput = "reverse";
    const String enableInput = "enable";
    const String chargePowerAvailableInput = "chargePwrAvail";
    const String interlockInput = "interlock";
    // output
    const String brakeLightOutput = "brakeLight";
    const String reverseLightOutput = "reverseLight";
    const String powerSteeringOutput = "powerSteering";
    const String stateOfChargeOutput = "stateOfCharge";
    const String statusLightOutput = "statusLight";
    const String prechargeMillis = "prechargeMillis";
    const String prechargeRelayOutput = "prechargeRelay";
    const String mainContactorOutput = "mainContactor";
    const String secondaryContactorOutput = "secondaryCont";
    const String enableMotorOutput = "enableMotor";
    const String coolingFanOutput = "coolingFan";
    const String coolingTempOn = "coolingTempOn";
    const String coolingTempOff = "coolingTempOff";
    const String fastChargeContactorOutput = "fastChargeCont";
    const String enableChargerOutput = "enableCharger";
    const String enableDcDcOutput = "enableDcDc";
    const String enableHeaterOutput = "enableHeater";
    const String heaterValveOutput = "heaterValve";
    const String heaterPumpOutput = "heaterPump";
    const String heaterTemperatureOn = "heaterTemperatureOn";
    const String coolingPumpOutput = "coolingPump";
    const String warningOutput = "warning";
    const String powerLimitationOutput = "powerLimit";
    // charger
    const String maximumInputCurrent = "maximumInputCurrent";
    const String constantCurrent = "constantCurrent";
    const String constantVoltage = "constantVoltage";
    const String terminateCurrent = "terminateCurrent";
    const String minimumBatteryVoltage = "minBatteryVoltage";
    const String maximumBatteryVoltage = "maxBatteryVoltage";
    const String minimumTemperature = "minimumTemperature";
    const String maximumTemperature = "maximumTemperature";
    const String maximumAmpereHours = "maximumAmpereHours";
    const String maximumChargeTime = "maximumChargeTime";
    const String deratingRate = "deratingRate";
    const String deratingReferenceTemperature = "deratingReferenceTmp";
    const String hystereseStopTemperature = "hystereseStopTemp";
    const String hystereseResumeTemperature = "hystereseResumeTemp";
    // dc dc converter
    const String dcDcMode = "dcDcMode";
    const String lowVoltageCommand = "lowVoltageCommand";
    const String hvUndervoltageLimit = "hvUndervoltageLimit";
    const String lvBuckModeCurrentLimit = "lvBuckCurrentLimit";
    const String hvBuckModeCurrentLimit = "hvBuckCurrentLimit";
    const String highVoltageCommand = "highVoltageCommand";
    const String lvUndervoltageLimit = "lvUndervoltageLimit";
    const String lvBoostModeCurrentLimit = "lvBoostCurrentLimit";
    const String hvBoostModeCurrentLimit = "hvBoostCurrentLimit";

    const String torqueRange = "torqueRange";
    const String rpmRange = "rpmRange";
    const String currentRange = "currentRange";
    const String motorTempRange = "motorTempRange";
    const String controllerTempRange = "controllerTempRange";
    const String batteryRangeLow = "batteryRangeLow";
    const String batteryRangeHigh = "batteryRangeHigh";
    const String socRange = "socRange";
    const String powerRange = "powerRange";
    const String chargeInputLevels = "chargeInputLevels";
    const String maximumSolarCurrent = "maximumSolarCurrent";
};

#endif
