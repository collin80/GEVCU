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
 * \brief Process the parameter update we received from a form submit.
 *
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 *
 * \param key and value pair of changed parameter
 */
void Wifi::processParameterChange(String input)
{
    int pos = input.indexOf('=');
    if (pos < 1) {
        return;
    }

    String key = input.substring(0, pos);
    String value = input.substring(pos + 1);

    if (value.charAt(0) == '"' && value.charAt(value.length() - 1) == '"') {
        value = value.substring(1, value.length() - 2); // cut leading/trailing '"' characters
    }

    processParameterChange(key, value);
}

/**
 * \brief Process the parameter update.
 *
 * \param key of changed parameter
 * \param value of changed parameter
 */
void Wifi::processParameterChange(String key, String value)
{
    if (key && value) {
        if (processParameterChangeThrottle(key, value) || processParameterChangeBrake(key, value) ||
                processParameterChangeMotor(key, value) || processParameterChangeCharger(key, value) ||
                processParameterChangeDcDc(key, value) || processParameterChangeDevices(key, value) ||
                processParameterChangeSystemIO(key, value)) {
            if (logger.isDebug()) {
                logger.debug(this, "parameter change: %s = %s", key.c_str(), value.c_str());
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
bool Wifi::processParameterChangeThrottle(String key, String value)
{
    Throttle *throttle = deviceManager.getAccelerator();

    if (throttle) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) throttle->getConfiguration();

        if(config) {
            if (numberPotMeters.equals(key)) {
                config->numberPotMeters = value.toInt();
            } else if (throttleSubType.equals(key)) {
                config->throttleSubType = value.toInt();
            } else if (minimumLevel.equals(key)) {
                config->minimumLevel = value.toInt();
            } else if (minimumLevel2.equals(key)) {
                config->minimumLevel2 = value.toInt();
            } else if (maximumLevel.equals(key)) {
                config->maximumLevel = value.toInt();
            } else if (maximumLevel2.equals(key)) {
                config->maximumLevel2 = value.toInt();
            } else if (positionRegenMaximum.equals(key)) {
                config->positionRegenMaximum = value.toDouble() * 10;
            } else if (positionRegenMinimum.equals(key)) {
                config->positionRegenMinimum = value.toDouble() * 10;
            } else if (positionForwardMotionStart.equals(key)) {
                config->positionForwardMotionStart = value.toDouble() * 10;
            } else if (positionHalfPower.equals(key)) {
                config->positionHalfPower = value.toDouble() * 10;
            } else if (minimumRegen.equals(key)) {
                config->minimumRegen = value.toInt();
            } else if (maximumRegen.equals(key)) {
                config->maximumRegen = value.toInt();
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
bool Wifi::processParameterChangeBrake(String key, String value)
{
    Throttle *brake = deviceManager.getBrake();

    if (brake) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) brake->getConfiguration();

        if (config) {
            if (brakeMinimumLevel.equals(key)) {
                config->minimumLevel = value.toInt();
            } else if (brakeMaximumLevel.equals(key)) {
                config->maximumLevel = value.toInt();
            } else if (brakeMinimumRegen.equals(key)) {
                config->minimumRegen = value.toInt();
            } else if (brakeMaximumRegen.equals(key)) {
                config->maximumRegen = value.toInt();
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
bool Wifi::processParameterChangeMotor(String key, String value)
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            if (speedMax.equals(key)) {
                config->speedMax = value.toInt();
            } else if (torqueMax.equals(key)) {
                config->torqueMax = value.toDouble() * 10;
            } else if (nominalVolt.equals(key)) {
                config->nominalVolt = value.toDouble() * 10;
            } else if (motorMode.equals(key)) {
                config->powerMode = (value.toInt() ? modeSpeed : modeTorque);
            } else if (invertDirection.equals(key)) {
                config->invertDirection = value.toInt();
            } else if (slewRate.equals(key)) {
                config->slewRate = value.toDouble() * 10;
            } else if (maxMechanicalPowerMotor.equals(key)) {
                config->maxMechanicalPowerMotor = value.toDouble() * 10;
            } else if (maxMechanicalPowerRegen.equals(key)) {
                config->maxMechanicalPowerRegen = value.toDouble() * 10;
            } else if (creepLevel.equals(key)) {
                config->creepLevel = value.toInt();
            } else if (creepSpeed.equals(key)) {
                config->creepSpeed = value.toInt();
            } else if (brakeHold.equals(key)) {
                config->brakeHold = value.toInt();
            } else if (brakeHoldLevel.equals(key)) {
                config->brakeHold = value.toInt();
            } else if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
                if (dcVoltLimitMotor.equals(key)) {
                    dmc5Config->dcVoltLimitMotor = value.toDouble() * 10;
                } else if (dcVoltLimitRegen.equals(key)) {
                    dmc5Config->dcVoltLimitRegen = value.toDouble() * 10;
                } else if (dcCurrentLimitMotor.equals(key)) {
                    dmc5Config->dcCurrentLimitMotor = value.toDouble() * 10;
                } else if (dcCurrentLimitRegen.equals(key)) {
                    dmc5Config->dcCurrentLimitRegen = value.toDouble() * 10;
                } else if (enableOscillationLimiter.equals(key)) {
                    dmc5Config->enableOscillationLimiter = value.toInt();
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
bool Wifi::processParameterChangeCharger(String key, String value)
{
    Charger *charger = deviceManager.getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (maximumSolarCurrent.equals(key)) {
            charger->setMaximumSolarCurrent(value.toDouble());
            return true;
        }

        if (config) {
            if (maximumInputCurrent.equals(key)) {
                config->maximumInputCurrent = value.toDouble() * 10;
            } else if (constantCurrent.equals(key)) {
                config->constantCurrent = value.toDouble() * 10;
            } else if (constantVoltage.equals(key)) {
                config->constantVoltage = value.toDouble() * 10;
            } else if (terminateCurrent.equals(key)) {
                config->terminateCurrent = value.toDouble() * 10;
            } else if (minimumBatteryVoltage.equals(key)) {
                config->minimumBatteryVoltage = value.toDouble() * 10;
            } else if (maximumBatteryVoltage.equals(key)) {
                config->maximumBatteryVoltage = value.toDouble() * 10;
            } else if (minimumTemperature.equals(key)) {
                config->minimumTemperature = value.toDouble() * 10;
            } else if (maximumTemperature.equals(key)) {
                config->maximumTemperature = value.toDouble() * 10;
            } else if (maximumAmpereHours.equals(key)) {
                config->maximumAmpereHours = value.toDouble() * 10;
            } else if (maximumChargeTime.equals(key)) {
                config->maximumChargeTime = value.toInt();
            } else if (deratingRate.equals(key)) {
                config->deratingRate = value.toDouble() * 10;
            } else if (deratingReferenceTemperature.equals(key)) {
                config->deratingReferenceTemperature = value.toDouble() * 10;
            } else if (hystereseStopTemperature.equals(key)) {
                config->hystereseStopTemperature = value.toDouble() * 10;
            } else if (hystereseResumeTemperature.equals(key)) {
                config->hystereseResumeTemperature = value.toDouble() * 10;
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
bool Wifi::processParameterChangeDcDc(String key, String value)
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (config) {
            if (dcDcMode.equals(key)) {
                config->mode = value.toInt();
            } else if (lowVoltageCommand.equals(key)) {
                config->lowVoltageCommand = value.toDouble() * 10;
            } else if (hvUndervoltageLimit.equals(key)) {
                config->hvUndervoltageLimit = value.toDouble();
            } else if (lvBuckModeCurrentLimit.equals(key)) {
                config->lvBuckModeCurrentLimit = value.toInt();
            } else if (hvBuckModeCurrentLimit.equals(key)) {
                config->hvBuckModeCurrentLimit = value.toDouble() * 10;
            } else if (highVoltageCommand.equals(key)) {
                config->highVoltageCommand = value.toInt();
            } else if (lvUndervoltageLimit.equals(key)) {
                config->lvUndervoltageLimit = value.toDouble() * 10;
            } else if (lvBoostModeCurrentLimit.equals(key)) {
                config->lvBoostModeCurrentLinit = value.toInt();
            } else if (hvBoostModeCurrentLimit.equals(key)) {
                config->hvBoostModeCurrentLimit = value.toDouble() * 10;
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
bool Wifi::processParameterChangeSystemIO(String key, String value)
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (enableInput.equals(key)) {
        config->enableInput = value.toInt();
    } else if (chargePowerAvailableInput.equals(key)) {
        config->chargePowerAvailableInput = value.toInt();
    } else if (interlockInput.equals(key)) {
        config->interlockInput = value.toInt();
    } else if (reverseInput.equals(key)) {
        config->reverseInput = value.toInt();
    } else if (absInput.equals(key)) {
        config->absInput = value.toInt();
    } else if (prechargeMillis.equals(key)) {
        config->prechargeMillis = value.toInt();
    } else if (prechargeRelayOutput.equals(key)) {
        config->prechargeRelayOutput = value.toInt();
    } else if (mainContactorOutput.equals(key)) {
        config->mainContactorOutput = value.toInt();
    } else if (secondaryContactorOutput.equals(key)) {
        config->secondaryContactorOutput = value.toInt();
    } else if (fastChargeContactorOutput.equals(key)) {
        config->fastChargeContactorOutput = value.toInt();
    } else if (enableMotorOutput.equals(key)) {
        config->enableMotorOutput = value.toInt();
    } else if (enableChargerOutput.equals(key)) {
        config->enableChargerOutput = value.toInt();
    } else if (enableDcDcOutput.equals(key)) {
        config->enableDcDcOutput = value.toInt();
    } else if (enableHeaterOutput.equals(key)) {
        config->enableHeaterOutput = value.toInt();
    } else if (heaterValveOutput.equals(key)) {
        config->heaterValveOutput = value.toInt();
    } else if (heaterPumpOutput.equals(key)) {
        config->heaterPumpOutput = value.toInt();
    } else if (heaterTemperatureOn.equals(key)) {
        config->heaterTemperatureOn = value.toInt();
    } else if (coolingPumpOutput.equals(key)) {
        config->coolingPumpOutput = value.toInt();
    } else if (coolingFanOutput.equals(key)) {
        config->coolingFanOutput = value.toInt();
    } else if (coolingTempOn.equals(key)) {
        config->coolingTempOn = value.toInt();
    } else if (coolingTempOff.equals(key)) {
        config->coolingTempOff = value.toInt();
    } else if (brakeLightOutput.equals(key)) {
        config->brakeLightOutput = value.toInt();
    } else if (reverseLightOutput.equals(key)) {
        config->reverseLightOutput = value.toInt();
    } else if (warningOutput.equals(key)) {
        config->warningOutput = value.toInt();
    } else if (powerLimitationOutput.equals(key)) {
        config->powerLimitationOutput = value.toInt();
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
bool Wifi::processParameterChangeDevices(String key, String value)
{
    if (logLevel.equals(key)) {
        Logger::LogLevel logLevel = (Logger::LogLevel) value.toInt();
        logger.setLoglevel(logLevel);
        systemIO.setLogLevel(logLevel);
    } else if (systemType.equals(key)) {
        systemIO.setSystemType((SystemType) value.toInt());
    } else if (key.charAt(0) == 'x' && key.substring(1).toInt() > 0) {
        long deviceId = strtol(key.c_str() + 1, 0, 16);
        deviceManager.sendMessage(DEVICE_ANY, (DeviceId) deviceId, (value.toInt() ? MSG_ENABLE : MSG_DISABLE), NULL);
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
        setParam(heaterTemperatureOn, config->heaterTemperatureOn);
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
    setParam(systemType, (uint8_t) systemIO.getSystemType());
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
