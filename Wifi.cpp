/*
 * Wifi.cpp
 *
 *  Created on: 31 Mar 2020
 *      Author: michaeln
 */

#include "Wifi.h"

/*
 * Constructor
 */
Wifi::Wifi() : Device()
{
    if (systemIO.getSystemType() == GEVCU3 || systemIO.getSystemType() == GEVCU4) {
        serialInterface = &Serial2;
    } else { //older hardware used this instead
        serialInterface = &Serial3;
    }
}

void Wifi::process() {
}

void Wifi::setParam(String key, String value) {
}

/**
 * \brief Process the parameter update.
 *
 * \param key of changed parameter
 * \param value of changed parameter
 */
void Wifi::processParameterChange(char *key, char *value)
{
    if (key && value) {
        if (processParameterChangeThrottle(key, value) || processParameterChangeBrake(key, value) ||
                processParameterChangeMotor(key, value) || processParameterChangeCharger(key, value) ||
                processParameterChangeDcDc(key, value) || processParameterChangeDevices(key, value) ||
                processParameterChangeSystemIO(key, value)) {
            if (logger.isDebug()) {
                logger.debug(this, "parameter change: %s = %s", key, value);
            }
        }
    }
}

/**
 * \brief Process a throttle parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeThrottle(char *key, char *value)
{
    Throttle *throttle = deviceManager.getAccelerator();

    if (throttle) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) throttle->getConfiguration();

        if(config) {
            if (numberPotMeters.equals(key)) {
                config->numberPotMeters = atol(value);
            } else if (throttleSubType.equals(key)) {
                config->throttleSubType = atol(value);
            } else if (minimumLevel.equals(key)) {
                config->minimumLevel = atol(value);
            } else if (minimumLevel2.equals(key)) {
                config->minimumLevel2 = atol(value);
            } else if (maximumLevel.equals(key)) {
                config->maximumLevel = atol(value);
            } else if (maximumLevel2.equals(key)) {
                config->maximumLevel2 = atol(value);
            } else if (positionRegenMaximum.equals(key)) {
                config->positionRegenMaximum = atof(value) * 10;
            } else if (positionRegenMinimum.equals(key)) {
                config->positionRegenMinimum = atof(value) * 10;
            } else if (positionForwardMotionStart.equals(key)) {
                config->positionForwardMotionStart = atof(value) * 10;
            } else if (positionHalfPower.equals(key)) {
                config->positionHalfPower = atof(value) * 10;
            } else if (minimumRegen.equals(key)) {
                config->minimumRegen = atol(value);
            } else if (maximumRegen.equals(key)) {
                config->maximumRegen = atol(value);
            } else {
                return false;
            }
            throttle->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a brake parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeBrake(char *key, char *value)
{
    Throttle *brake = deviceManager.getBrake();

    if (brake) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) brake->getConfiguration();

        if (config) {
            if (brakeMinimumLevel.equals(key)) {
                config->minimumLevel = atol(value);
            } else if (brakeMaximumLevel.equals(key)) {
                config->maximumLevel = atol(value);
            } else if (brakeMinimumRegen.equals(key)) {
                config->minimumRegen = atol(value);
            } else if (brakeMaximumRegen.equals(key)) {
                config->maximumRegen = atol(value);
            } else {
                return false;
            }
            brake->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a motor parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeMotor(char *key, char *value)
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            if (speedMax.equals(key)) {
                config->speedMax = atol(value);
            } else if (torqueMax.equals(key)) {
                config->torqueMax = atof(value) * 10;
            } else if (nominalVolt.equals(key)) {
                config->nominalVolt = atof(value) * 10;
            } else if (motorMode.equals(key)) {
                config->powerMode = (atol(value) ? modeSpeed : modeTorque);
            } else if (invertDirection.equals(key)) {
                config->invertDirection = atol(value);
            } else if (slewRate.equals(key)) {
                config->slewRate = atof(value) * 10;
            } else if (maxMechanicalPowerMotor.equals(key)) {
                config->maxMechanicalPowerMotor = atof(value) * 10;
            } else if (maxMechanicalPowerRegen.equals(key)) {
                config->maxMechanicalPowerRegen = atof(value) * 10;
            } else if (creepLevel.equals(key)) {
                config->creepLevel = atol(value);
            } else if (creepSpeed.equals(key)) {
                config->creepSpeed = atol(value);
            } else if (brakeHold.equals(key)) {
                config->brakeHold = atol(value);
            } else if (brakeHoldLevel.equals(key)) {
                config->brakeHold = atol(value);
            } else if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
                if (dcVoltLimitMotor.equals(key)) {
                    dmc5Config->dcVoltLimitMotor = atof(value) * 10;
                } else if (dcVoltLimitRegen.equals(key)) {
                    dmc5Config->dcVoltLimitRegen = atof(value) * 10;
                } else if (dcCurrentLimitMotor.equals(key)) {
                    dmc5Config->dcCurrentLimitMotor = atof(value) * 10;
                } else if (dcCurrentLimitRegen.equals(key)) {
                    dmc5Config->dcCurrentLimitRegen = atof(value) * 10;
                } else if (enableOscillationLimiter.equals(key)) {
                    dmc5Config->enableOscillationLimiter = atol(value);
                } else {
                    return false;
                }
            } else {
                return false;
            }
            motorController->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a charger parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeCharger(char *key, char *value)
{
    Charger *charger = deviceManager.getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (maximumSolarCurrent.equals(key)) {
            charger->setMaximumSolarCurrent(atof(value));
            return true;
        }

        if (config) {
            if (maximumInputCurrent.equals(key)) {
                config->maximumInputCurrent = atof(value) * 10;
            } else if (constantCurrent.equals(key)) {
                config->constantCurrent = atof(value) * 10;
            } else if (constantVoltage.equals(key)) {
                config->constantVoltage = atof(value) * 10;
            } else if (terminateCurrent.equals(key)) {
                config->terminateCurrent = atof(value) * 10;
            } else if (minimumBatteryVoltage.equals(key)) {
                config->minimumBatteryVoltage = atof(value) * 10;
            } else if (maximumBatteryVoltage.equals(key)) {
                config->maximumBatteryVoltage = atof(value) * 10;
            } else if (minimumTemperature.equals(key)) {
                config->minimumTemperature = atof(value) * 10;
            } else if (maximumTemperature.equals(key)) {
                config->maximumTemperature = atof(value) * 10;
            } else if (maximumAmpereHours.equals(key)) {
                config->maximumAmpereHours = atof(value) * 10;
            } else if (maximumChargeTime.equals(key)) {
                config->maximumChargeTime = atol(value);
            } else if (deratingRate.equals(key)) {
                config->deratingRate = atof(value) * 10;
            } else if (deratingReferenceTemperature.equals(key)) {
                config->deratingReferenceTemperature = atof(value) * 10;
            } else if (hystereseStopTemperature.equals(key)) {
                config->hystereseStopTemperature = atof(value) * 10;
            } else if (hystereseResumeTemperature.equals(key)) {
                config->hystereseResumeTemperature = atof(value) * 10;
            } else {
                return false;
            }
            charger->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a DCDC converter parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeDcDc(char *key, char *value)
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (config) {
            if (dcDcMode.equals(key)) {
                config->mode = atol(value);
            } else if (lowVoltageCommand.equals(key)) {
                config->lowVoltageCommand = atof(value) * 10;
            } else if (hvUndervoltageLimit.equals(key)) {
                config->hvUndervoltageLimit = atof(value);
            } else if (lvBuckModeCurrentLimit.equals(key)) {
                config->lvBuckModeCurrentLimit = atol(value);
            } else if (hvBuckModeCurrentLimit.equals(key)) {
                config->hvBuckModeCurrentLimit = atof(value) * 10;
            } else if (highVoltageCommand.equals(key)) {
                config->highVoltageCommand = atol(value);
            } else if (lvUndervoltageLimit.equals(key)) {
                config->lvUndervoltageLimit = atof(value) * 10;
            } else if (lvBoostModeCurrentLimit.equals(key)) {
                config->lvBoostModeCurrentLinit = atol(value);
            } else if (hvBoostModeCurrentLimit.equals(key)) {
                config->hvBoostModeCurrentLimit = atof(value) * 10;
            } else {
                return false;
            }
            dcDcConverter->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a system I/O parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeSystemIO(char *key, char *value)
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (enableInput.equals(key)) {
        config->enableInput = atol(value);
    } else if (chargePowerAvailableInput.equals(key)) {
        config->chargePowerAvailableInput = atol(value);
    } else if (interlockInput.equals(key)) {
        config->interlockInput = atol(value);
    } else if (reverseInput.equals(key)) {
        config->reverseInput = atol(value);
    } else if (absInput.equals(key)) {
        config->absInput = atol(value);
    } else if (prechargeMillis.equals(key)) {
        config->prechargeMillis = atol(value);
    } else if (prechargeRelayOutput.equals(key)) {
        config->prechargeRelayOutput = atol(value);
    } else if (mainContactorOutput.equals(key)) {
        config->mainContactorOutput = atol(value);
    } else if (secondaryContactorOutput.equals(key)) {
        config->secondaryContactorOutput = atol(value);
    } else if (fastChargeContactorOutput.equals(key)) {
        config->fastChargeContactorOutput = atol(value);
    } else if (enableMotorOutput.equals(key)) {
        config->enableMotorOutput = atol(value);
    } else if (enableChargerOutput.equals(key)) {
        config->enableChargerOutput = atol(value);
    } else if (enableDcDcOutput.equals(key)) {
        config->enableDcDcOutput = atol(value);
    } else if (enableHeaterOutput.equals(key)) {
        config->enableHeaterOutput = atol(value);
    } else if (heaterValveOutput.equals(key)) {
        config->heaterValveOutput = atol(value);
    } else if (heaterPumpOutput.equals(key)) {
        config->heaterPumpOutput = atol(value);
    } else if (coolingPumpOutput.equals(key)) {
        config->coolingPumpOutput = atol(value);
    } else if (coolingFanOutput.equals(key)) {
        config->coolingFanOutput = atol(value);
    } else if (coolingTempOn.equals(key)) {
        config->coolingTempOn = atol(value);
    } else if (coolingTempOff.equals(key)) {
        config->coolingTempOff = atol(value);
    } else if (brakeLightOutput.equals(key)) {
        config->brakeLightOutput = atol(value);
    } else if (reverseLightOutput.equals(key)) {
        config->reverseLightOutput = atol(value);
    } else if (warningOutput.equals(key)) {
        config->warningOutput = atol(value);
    } else if (powerLimitationOutput.equals(key)) {
        config->powerLimitationOutput = atol(value);
    } else {
        return false;
    }
    systemIO.saveConfiguration();
    return true;
}

/**
 * \brief Process a device specific parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
bool Wifi::processParameterChangeDevices(char *key, char *value)
{
    if (logLevel.equals(key)) {
        Logger::LogLevel logLevel = (Logger::LogLevel) atoi(value);
        logger.setLoglevel(logLevel);
        systemIO.setLogLevel(logLevel);
    } else if (key[0] == 'x' && atol(&key[1]) > 0) {
        long deviceId = strtol(key + 1, 0, 16);
        deviceManager.sendMessage(DEVICE_ANY, (DeviceId) deviceId, (atol(value) ? MSG_ENABLE : MSG_DISABLE), NULL);
        return true;
    }
    return false;
}

/*
 * \brief Get parameters from devices and forward them to the wifi device.
 */
void Wifi::loadParameters()
{
    logger.info(this, "loading config params to wifi...");

    loadParametersThrottle();
    loadParametersBrake();
    loadParametersMotor();
    loadParametersCharger();
    loadParametersDcDc();
    loadParametersSystemIO();
    loadParametersDevices();
    loadParametersDashboard();
}

/**
 * \brief Load throttle parameters to the wifi device
 *
 */
void Wifi::loadParametersThrottle()
{
    Throttle *throttle = deviceManager.getAccelerator();

    if (throttle) {
        ThrottleConfiguration *throttleConfig = (ThrottleConfiguration *) throttle->getConfiguration();

        if (throttleConfig) {
            if (throttle->getId() == POTACCELPEDAL) {
                PotThrottleConfiguration *potThrottleConfig = (PotThrottleConfiguration *) throttleConfig;
                setParam(numberPotMeters, potThrottleConfig->numberPotMeters);
                setParam(throttleSubType, potThrottleConfig->throttleSubType);
                setParam(minimumLevel2, potThrottleConfig->minimumLevel2);
                setParam(maximumLevel2, potThrottleConfig->maximumLevel2);
            } else { // set reasonable values so the config page can be saved
                setParam(numberPotMeters, (uint8_t) 1);
                setParam(throttleSubType, (uint8_t)0);
                setParam(minimumLevel2, (uint16_t) 50);
                setParam(maximumLevel2, (uint16_t) 4095);
            }
            setParam(minimumLevel, throttleConfig->minimumLevel);
            setParam(maximumLevel, throttleConfig->maximumLevel);
            setParam(positionRegenMaximum, (uint16_t) (throttleConfig->positionRegenMaximum / 10));
            setParam(positionRegenMinimum, (uint16_t) (throttleConfig->positionRegenMinimum / 10));
            setParam(positionForwardMotionStart, (uint16_t) (throttleConfig->positionForwardMotionStart / 10));
            setParam(positionHalfPower, (uint16_t) (throttleConfig->positionHalfPower / 10));
            setParam(minimumRegen, throttleConfig->minimumRegen);
            setParam(maximumRegen, throttleConfig->maximumRegen);
        }
    }
}

/**
 * \brief Load brake parameters to the wifi device
 *
 */
void Wifi::loadParametersBrake()
{
    Throttle *brake = deviceManager.getBrake();

    if (brake) {
        ThrottleConfiguration *brakeConfig = (ThrottleConfiguration *) brake->getConfiguration();

        if (brakeConfig) {
            setParam(brakeMinimumLevel, brakeConfig->minimumLevel);
            setParam(brakeMaximumLevel, brakeConfig->maximumLevel);
            setParam(brakeMinimumRegen, brakeConfig->minimumRegen);
            setParam(brakeMaximumRegen, brakeConfig->maximumRegen);
        }
    } else {
        setParam(brakeMinimumLevel, (uint8_t)0);
        setParam(brakeMaximumLevel, (uint8_t)100);
        setParam(brakeMinimumRegen, (uint8_t)0);
        setParam(brakeMaximumRegen, (uint8_t)0);
    }
}

/**
 * \brief Load motor parameters to the wifi device
 *
 */
void Wifi::loadParametersMotor()
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            setParam(speedMax, config->speedMax);
            setParam(nominalVolt, config->nominalVolt / 10.0f, 1);
            setParam(torqueMax, config->torqueMax / 10.0f, 1);
            setParam(motorMode, (uint8_t) config->powerMode);
            setParam(invertDirection, (uint8_t)(config->invertDirection ? 1 : 0));
            setParam(slewRate, config->slewRate / 10.0f, 1);
            setParam(maxMechanicalPowerMotor, config->maxMechanicalPowerMotor / 10.0f, 1);
            setParam(maxMechanicalPowerRegen, config->maxMechanicalPowerRegen / 10.0f, 1);
            setParam(creepLevel, config->creepLevel);
            setParam(creepSpeed, config->creepSpeed);
            setParam(brakeHold, config->brakeHold);
            setParam(cruiseUseRpm, (uint8_t)(config->cruiseUseRpm ? 1 : 0));
            setParam(cruiseSpeedStep, config->cruiseStepDelta);

            if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
                setParam(dcVoltLimitMotor, dmc5Config->dcVoltLimitMotor / 10.0f, 1);
                setParam(dcVoltLimitRegen, dmc5Config->dcVoltLimitRegen / 10.0f, 1);
                setParam(dcCurrentLimitMotor, dmc5Config->dcCurrentLimitMotor / 10.0f, 1);
                setParam(dcCurrentLimitRegen, dmc5Config->dcCurrentLimitRegen / 10.0f, 1);
                setParam(enableOscillationLimiter, (uint8_t)(dmc5Config->enableOscillationLimiter ? 1 : 0));
            }
            String sets;
            for (uint8_t i = 0; i < CFG_CRUISE_SIZE_SPEED_SET && config->speedSet[i] != 0; i++) {
                if (i != 0) {
                    sets += ",";
                }
                sets += config->speedSet[i];
            }
            setParam((String)cruiseSpeedSet, sets);
        }
    }
}

/**
 * \brief Load charger parameters to the wifi device
 *
 */
void Wifi::loadParametersCharger()
{
    Charger *charger = deviceManager.getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (config) {
            setParam(maximumInputCurrent, config->maximumInputCurrent / 10.0f, 1);
            setParam(constantCurrent, config->constantCurrent / 10.0f, 1);
            setParam(constantVoltage, config->constantVoltage / 10.0f, 1);
            setParam(terminateCurrent, config->terminateCurrent / 10.0f, 1);
            setParam(minimumBatteryVoltage, config->minimumBatteryVoltage / 10.0f, 1);
            setParam(maximumBatteryVoltage, config->maximumBatteryVoltage / 10.0f, 1);
            setParam(minimumTemperature, config->minimumTemperature / 10.0f, 1);
            setParam(maximumTemperature, config->maximumTemperature / 10.0f, 1);
            setParam(maximumAmpereHours, config->maximumAmpereHours / 10.0f, 1);
            setParam(maximumChargeTime, config->maximumChargeTime);
            setParam(deratingRate, config->deratingRate / 10.0f, 1);
            setParam(deratingReferenceTemperature, config->deratingReferenceTemperature / 10.0f, 1);
            setParam(hystereseStopTemperature, config->hystereseStopTemperature / 10.0f, 1);
            setParam(hystereseResumeTemperature, config->hystereseResumeTemperature / 10.0f, 1);
        }
    }
}

/**
 * \brief Load DCDC converter parameters to the wifi device
 *
 */
void Wifi::loadParametersDcDc()
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *dcDcConfig = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (dcDcConfig) {
            setParam(dcDcMode, dcDcConfig->mode);
            setParam(lowVoltageCommand, dcDcConfig->lowVoltageCommand / 10.0f, 1);
            setParam(hvUndervoltageLimit, dcDcConfig->hvUndervoltageLimit);
            setParam(lvBuckModeCurrentLimit, dcDcConfig->lvBuckModeCurrentLimit);
            setParam(hvBuckModeCurrentLimit, dcDcConfig->hvBuckModeCurrentLimit / 10.0f, 1);
            setParam(highVoltageCommand, dcDcConfig->highVoltageCommand);
            setParam(lvUndervoltageLimit, dcDcConfig->lvUndervoltageLimit / 10.0f, 1);
            setParam(lvBoostModeCurrentLimit, dcDcConfig->lvBoostModeCurrentLinit);
            setParam(hvBoostModeCurrentLimit, dcDcConfig->hvBoostModeCurrentLimit / 10.0f, 1);
        }
    }
}

/**
 * \brief Load system I/O parameters to the wifi device
 *
 */
void Wifi::loadParametersSystemIO()
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (config) {
        setParam(enableInput, config->enableInput);
        setParam(chargePowerAvailableInput, config->chargePowerAvailableInput);
        setParam(interlockInput, config->interlockInput);
        setParam(reverseInput, config->reverseInput);
        setParam(absInput, config->absInput);

        setParam(prechargeMillis, config->prechargeMillis);
        setParam(prechargeRelayOutput, config->prechargeRelayOutput);
        setParam(mainContactorOutput, config->mainContactorOutput);
        setParam(secondaryContactorOutput, config->secondaryContactorOutput);
        setParam(fastChargeContactorOutput, config->fastChargeContactorOutput);

        setParam(enableMotorOutput, config->enableMotorOutput);
        setParam(enableChargerOutput, config->enableChargerOutput);
        setParam(enableDcDcOutput, config->enableDcDcOutput);
        setParam(enableHeaterOutput, config->enableHeaterOutput);

        setParam(heaterValveOutput, config->heaterValveOutput);
        setParam(heaterPumpOutput, config->heaterPumpOutput);
        setParam(coolingPumpOutput, config->coolingPumpOutput);
        setParam(coolingFanOutput, config->coolingFanOutput);
        setParam(coolingTempOn, config->coolingTempOn);
        setParam(coolingTempOff, config->coolingTempOff);

        setParam(brakeLightOutput, config->brakeLightOutput);
        setParam(reverseLightOutput, config->reverseLightOutput);
        setParam(warningOutput, config->warningOutput);
        setParam(powerLimitationOutput, config->powerLimitationOutput);
    }
}

/**
 * \brief Load device specific parameters to the wifi device
 *
 */
void Wifi::loadParametersDevices()
{
    Device *device = NULL;
    char idHex[10];
    int size = sizeof(deviceIds) / sizeof(DeviceId);

    setParam(logLevel, (uint8_t) logger.getLogLevel());
    for (int i = 0; i < size; i++) {
        sprintf(idHex, "x%x", deviceIds[i]);
        device = deviceManager.getDeviceByID(deviceIds[i]);
        if (device != NULL) {
            setParam(idHex, (uint8_t)((device->isEnabled() == true) ? 1 : 0));
        } else {
            setParam(idHex, (uint8_t)0);
        }
    }
}

/**
 * \brief Load dashboard related parameters to the wifi device
 *
 */
void Wifi::loadParametersDashboard()
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            sprintf(buffer, "%d,%d", config->torqueMax / -10, config->torqueMax / 10);
            setParam(torqueRange, buffer);
            sprintf(buffer, "0,%d", config->speedMax);
            setParam(rpmRange, buffer);
            sprintf(buffer, "%d,%d", config->maxMechanicalPowerRegen / -10, config->maxMechanicalPowerMotor / 10);
            setParam(powerRange, buffer);
        } else {
            setParam(torqueRange, "-300,300");
            setParam(rpmRange, "0,8000");
            setParam(powerRange, "-250,250");
        }

        //TODO make params configurable
        setParam(currentRange, "-200,200");
        setParam(batteryRangeLow, "297,357,368");
        setParam(batteryRangeHigh, "387,405,418");
        setParam(motorTempRange, "0,90,120");
        setParam(controllerTempRange, "0,60,80");
        setParam(socRange, "0,20,100");
        setParam(chargeInputLevels, "6,7,8,9,10,13,16,20,32,40,56");
    }
}

/**
 * \brief Set a parameter to the given int32 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void Wifi::setParam(String paramName, int32_t value)
{
    sprintf(buffer, "%ld", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given uint32 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void Wifi::setParam(String paramName, uint32_t value)
{
    sprintf(buffer, "%lu", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given int16 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void Wifi::setParam(String paramName, int16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given uint16 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void Wifi::setParam(String paramName, uint16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given int8 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void Wifi::setParam(String paramName, uint8_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given float value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 * \param precision the number of digits after the decimal sign
 */
void Wifi::setParam(String paramName, float value, int precision)
{
    char format[10];
    sprintf(format, "%%.%df", precision);
    sprintf(buffer, format, value);
    setParam(paramName, buffer);
}

void Wifi::loadConfiguration()
{
//    WifiConfiguration *config = (WifiConfiguration*) getConfiguration();

    Device::loadConfiguration(); // call parent
    logger.info(this, "Wifi configuration:");

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
    } else { //checksum invalid. Reinitialize values and store to EEPROM
    }
}

void Wifi::saveConfiguration()
{
//    WifiConfiguration *config = (WifiConfiguration*) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->saveChecksum();
}
