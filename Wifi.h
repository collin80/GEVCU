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
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "ELM327Processor.h"
#include "BrusaDMC5.h"
#include "ParamCache.h"

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
    void processParameterChange(char *key, char *value);
    void loadParameters();
    virtual void setParam(String paramName, String value);

    USARTClass *serialInterface; //Allows to retarget the serial port we use
    ParamCache paramCache;

private:
    bool processParameterChangeThrottle(char *key, char *value);
    bool processParameterChangeBrake(char *key, char *value);
    bool processParameterChangeMotor(char *key, char *value);
    bool processParameterChangeCharger(char *key, char *value);
    bool processParameterChangeDcDc(char *key, char *value);
    bool processParameterChangeSystemIO(char *key, char *value);
    bool processParameterChangeDevices(char *key, char *value);
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
    const String creepLevel = "creepLevel";
    const String creepSpeed = "creepSpeed";
    const String brakeMinimumLevel = "brakeMinimumLevel";
    const String brakeMaximumLevel = "brakeMaximumLevel";
    const String brakeMinimumRegen = "brakeMinimumRegen";
    const String brakeMaximumRegen = "brakeMaximumRegen";
    const String speedMax = "speedMax";
    const String torqueMax = "torqueMax";
    const String logLevel = "logLevel";
    const String nominalVolt = "nominalVolt";
    const String motorMode = "motorMode";
    const String invertDirection = "invertDirection";
    const String slewRate = "slewRate";
    const String brakeHold = "brakeHold";
    const String brakeHoldLevel = "brakeHoldLevel";
    const String cruiseUseRpm = "cruiseUseRpm";
    const String cruiseSpeedStep = "cruiseSpeedStep";
    const String cruiseSpeedSet = "cruiseSpeedSet";

    const String maxMechanicalPowerMotor = "maxMechPowerMotor";
    const String maxMechanicalPowerRegen = "maxMechPowerRegen";
    const String dcVoltLimitMotor = "dcVoltLimitMotor";
    const String dcVoltLimitRegen = "dcVoltLimitRegen";
    const String dcCurrentLimitMotor = "dcCurrentLimitMotor";
    const String dcCurrentLimitRegen = "dcCurrentLimitRegen";
    const String enableOscillationLimiter = "enableOscLimiter";
    // input
    const String absInput = "absInput";
    const String reverseInput = "reverseInput";
    const String enableInput = "enableInput";
    const String chargePowerAvailableInput = "chargePwrAvailInput";
    const String interlockInput = "interlockInput";
    // output
    const String brakeLightOutput = "brakeLightOutput";
    const String reverseLightOutput = "reverseLightOutput";
    const String powerSteeringOutput = "powerSteeringOutput";
    const String unusedOutput = "unusedOutput";
    const String prechargeMillis = "prechargeMillis";
    const String prechargeRelayOutput = "prechargeRelayOutput";
    const String mainContactorOutput = "mainContactorOutput";
    const String secondaryContactorOutput = "secondaryContOutput";
    const String enableMotorOutput = "enableMotorOutput";
    const String coolingFanOutput = "coolingFanOutput";
    const String coolingTempOn = "coolingTempOn";
    const String coolingTempOff = "coolingTempOff";
    const String fastChargeContactorOutput = "fastChargeContOutput";
    const String enableChargerOutput = "enableChargerOutput";
    const String enableDcDcOutput = "enableDcDcOutput";
    const String enableHeaterOutput = "enableHeaterOutput";
    const String heaterValveOutput = "heaterValveOutput";
    const String heaterPumpOutput = "heaterPumpOutput";
    const String coolingPumpOutput = "coolingPumpOutput";
    const String warningOutput = "warningOutput";
    const String powerLimitationOutput = "powerLimitOutput";
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
