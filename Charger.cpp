/*
 * Charger.cpp
 *
 * Parent class for all chargers
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

#include "Charger.h"

Charger::Charger() : Device()
{
    inputCurrent = 0;
    inputVoltage = 0;
    batteryVoltage = 0;
    batteryCurrent = 0;
    batteryTemperature = CFG_NO_TEMPERATURE_DATA;
    ampereMilliSeconds = 0;
    wattMilliSeconds = 0;
    chargeStartTime = 0;
    lastTick = 0;
    constantVoltage = false;
}

Charger::~Charger()
{
}

/*
 * Process event from the tick handler.
 */
void Charger::handleTick()
{
    Device::handleTick(); // call parent

    uint32_t timeStamp = millis();
    if (timeStamp < lastTick) {
        lastTick = 0;
    }
    ampereMilliSeconds += (timeStamp - lastTick) * batteryCurrent;
    wattMilliSeconds += (timeStamp - lastTick) * batteryCurrent * batteryVoltage;
    lastTick = timeStamp;
}

/**
 * Act on messages the super-class does not react upon, like state change
 * to charging which should enable the charger
 */
void Charger::handleStateChange(Status::SystemState state)
{
    Device::handleStateChange(state);
    if (state == Status::charging) {
        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();

        // re-initialize variables
        constantVoltage = 0;
        inputCurrent = 0;
        inputVoltage = 0;
        batteryVoltage = 0;
        batteryCurrent = 0;
        ampereMilliSeconds = 0;
        wattMilliSeconds = 0;
        chargeStartTime = millis();
        lastTick = millis();

        requestedOutputCurrent = config->constantCurrent;
        powerOn = true;
    } else {
        if (powerOn) {
            Logger::info(getId(), "Charging finished after %d min, %f Ah / %f kWh", (millis() - chargeStartTime) / 60000, (float) ampereMilliSeconds / 360000000.0f, (float) wattMilliSeconds / 3600000000000.0f);
        }
        requestedOutputCurrent = 0;
        powerOn = false;
    }
    systemIO->setEnableCharger(powerOn);
}

/**
 * Calculate desired output voltage to battery in 0.1V
 */
uint16_t Charger::getOutputVoltage()
{
    if (powerOn && running) {
        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();
        return config->constantVoltage;
    }
    return 0;
}

/**
 * Calculate desired output current to battery in 0.1A
 */
uint16_t Charger::getOutputCurrent()
{
    if (powerOn && running) {
        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();
        int16_t temperature;

//TODO inmplement temp hysterese / derating according to the following config variables:
//        bool useTemperatureDerating; // if true derating is used at high temperatures, otherwise hysterese at fixed temperatures will be used
//        uint16_t deratingTemperature; // 0.1Ah per deg Celsius
//        uint16_t deratingReferenceTemperature; // 0.1 deg Celsius where derating will reach 0 Amp
//        uint16_t hystereseStopTemperature; // 0.1 deg Celsius where charging will stop in hysterese mode
//        uint16_t hystereseResumeTemperature; // 0.1 deg Celsius where charging is resumed

        // in constant voltage phase decrease current accordingly
        if (batteryVoltage > config->constantVoltage) {
            requestedOutputCurrent--; //TODO verify if this is an appropriate way to decrease the current
            if (!constantVoltage) {
                Logger::info(getId(), "Constant current (CC) phase finished, switching to constant voltage (CV) phase");
                constantVoltage = true;
            }
        }
        if (requestedOutputCurrent < config->terminateCurrent ||
                ((millis() - chargeStartTime) > 5000 && batteryCurrent < config->terminateCurrent)) { // give the charger 5sec to ramp up the current
            requestedOutputCurrent = 0;
            status->setSystemState(Status::charged);
        }
        if ((millis() - chargeStartTime) > config->maximumChargeTime * 60000) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Maximum charge time exceeded (%imin)", (millis() - chargeStartTime) / 60000);
            status->setSystemState(Status::error);
        }
        if (batteryVoltage > config->maximumBatteryVoltage) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Maximum battery voltage exceeded (%fV)", (float) batteryVoltage / 10.0f);
            status->setSystemState(Status::error);
        }
        if (batteryVoltage < config->minimumBatteryVoltage) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Battery voltage too low (%fV)", (float) batteryVoltage / 10.0f);
            status->setSystemState(Status::error);
        }
        if (ampereMilliSeconds / 36000000 > config->maximumAmpereHours) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Maximum ampere hours exceeded (%f)", (float) ampereMilliSeconds / 360000000.0f);
            status->setSystemState(Status::error);
        }
        temperature = getHighestBatteryTemperature();
        if (temperature != CFG_NO_TEMPERATURE_DATA && temperature > config->maximumTemperature) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Battery temperature too high (%f deg C)", (float) temperature / 10.0f);
            status->setSystemState(Status::error);
        }
        temperature = getLowestBatteryTemperature();
        if (temperature != CFG_NO_TEMPERATURE_DATA && temperature < config->minimumTemperature) {
            requestedOutputCurrent = 0;
            Logger::error(getId(), "Battery temperature too low (%f deg C)", (float) temperature / 10.0f);
            status->setSystemState(Status::error);
        }
        return requestedOutputCurrent;
    }
    return 0;
}

/**
 * Find highest temperature reported by charger or external temperature sensors (in 0.1 deg C)
 */
int16_t Charger::getHighestBatteryTemperature()
{
    int16_t temperature = batteryTemperature;
    if (status->getHighestExternalTemperature() != CFG_NO_TEMPERATURE_DATA &&
            status->getHighestExternalTemperature() * 10 > temperature) {
        temperature = status->getHighestExternalTemperature() * 10;
    }
    return temperature;
}

/**
 * Find lowest temperature reported by charger or external temperature sensors (in 0.1 deg C)
 */
int16_t Charger::getLowestBatteryTemperature()
{
    int16_t temperature = batteryTemperature;

    if (status->getLowestExternalTemperature() != CFG_NO_TEMPERATURE_DATA &&
            status->getLowestExternalTemperature() * 10 < temperature) {
        temperature = status->getLowestExternalTemperature() * 10;
    }
    return temperature;
}

/*
 * Return the device type
 */
DeviceType Charger::getType()
{
    return (DEVICE_CHARGER);
}

uint16_t Charger::getBatteryCurrent()
{
    return batteryCurrent;
}

int16_t Charger::getBatteryTemperature()
{
    return batteryTemperature;
}

void Charger::setBatteryTemperature(int16_t batteryTemperature)
{
    this->batteryTemperature = batteryTemperature;
}

uint16_t Charger::getBatteryVoltage()
{
    return batteryVoltage;
}

uint16_t Charger::getInputCurrent()
{
    return inputCurrent;
}

uint16_t Charger::getInputVoltage()
{
    return inputVoltage;
}

/*
 * Load configuration data from EEPROM.
 *
 * If not available or the checksum is invalid, default values are chosen.
 */
void Charger::loadConfiguration()
{
    ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();

    if (!config) {
        config = new ChargerConfiguration();
        setConfiguration(config);
    }

    Device::loadConfiguration(); // call parent
    Logger::info(getId(), "Charger configuration:");

#ifdef USE_HARD_CODED

    if (false) {
#else
    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        uint8_t temp;
        Logger::debug(getId(), (char *) Constants::validChecksum);

        prefsHandler->read(CHRG_MAX_INPUT_CURRENT, &config->maximumInputCurrent);
        prefsHandler->read(CHRG_CONSTANT_CURRENT, &config->constantCurrent);
        prefsHandler->read(CHRG_CONSTANT_VOLTAGE, &config->constantVoltage);
        prefsHandler->read(CHRG_TERMINATE_CURRENT, &config->terminateCurrent);
        prefsHandler->read(CHRG_MIN_BATTERY_VOLTAGE, &config->minimumBatteryVoltage);
        prefsHandler->read(CHRG_MAX_BATTERY_VOLTAGE, &config->maximumBatteryVoltage);
        prefsHandler->read(CHRG_MIN_BATTERY_TEMPERATURE, (uint16_t *) &config->minimumTemperature);
        prefsHandler->read(CHRG_MAX_BATTERY_TEMPERATURE, &config->maximumTemperature);
        prefsHandler->read(CHRG_MAX_AMPERE_HOURS, &config->maximumAmpereHours);
        prefsHandler->read(CHRG_MAX_CHARGE_TIME, &config->maximumChargeTime);
        prefsHandler->read(CHRG_TEMPERATURE_MODE, &temp);
        config->useTemperatureDerating = (temp == 1);
        prefsHandler->read(CHRG_DERATING_TEMPERATURE, &config->deratingTemperature);
        prefsHandler->read(CHRG_DERATING_REFERENCE, &config->deratingReferenceTemperature);
        prefsHandler->read(CHRG_HYSTERESE_STOP, &config->hystereseStopTemperature);
        prefsHandler->read(CHRG_HYSTERESE_RESUME, &config->hystereseResumeTemperature);
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn(getId(), (char *) Constants::invalidChecksum);
        config->maximumInputCurrent = 100;
        config->constantCurrent = 100;
        config->constantVoltage = 4165;
        config->terminateCurrent = 50;
        config->minimumBatteryVoltage = 3272;
        config->maximumBatteryVoltage = 4284;
        config->minimumTemperature = 30;
        config->maximumTemperature = 600;
        config->maximumAmpereHours = 1200;
        config->maximumChargeTime = 600;
        config->useTemperatureDerating = 0;
        config->deratingTemperature = 1;
        config->deratingReferenceTemperature = 500;
        config->hystereseStopTemperature = 600;
        config->hystereseResumeTemperature = 500;
        saveConfiguration();
    }

    Logger::info(getId(), "max input current: %fA, constant current: %fA, constant voltage: %fV", (float) config->maximumInputCurrent / 10.0F, (float) config->constantCurrent / 10.0F, (float) config->constantVoltage / 10.0F);
    Logger::info(getId(), "terminate current: %fA, battery min: %fV max: %fV", (float) config->terminateCurrent / 10.0F, (float) config->minimumBatteryVoltage / 10.0F, (float) config->maximumBatteryVoltage / 10.0F);
}

/*
 * Store the current configuration parameters to EEPROM.
 */
void Charger::saveConfiguration()
{
    ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();

    Device::saveConfiguration(); // call parent

    prefsHandler->write(CHRG_MAX_INPUT_CURRENT, config->maximumInputCurrent);
    prefsHandler->write(CHRG_CONSTANT_CURRENT, config->constantCurrent);
    prefsHandler->write(CHRG_CONSTANT_VOLTAGE, config->constantVoltage);
    prefsHandler->write(CHRG_TERMINATE_CURRENT, config->terminateCurrent);
    prefsHandler->write(CHRG_MIN_BATTERY_VOLTAGE, config->minimumBatteryVoltage);
    prefsHandler->write(CHRG_MAX_BATTERY_VOLTAGE, config->maximumBatteryVoltage);
    prefsHandler->write(CHRG_MIN_BATTERY_TEMPERATURE, (uint16_t) config->minimumTemperature);
    prefsHandler->write(CHRG_MAX_BATTERY_TEMPERATURE, config->maximumTemperature);
    prefsHandler->write(CHRG_MAX_AMPERE_HOURS, config->maximumAmpereHours);
    prefsHandler->write(CHRG_MAX_CHARGE_TIME, config->maximumChargeTime);
    prefsHandler->write(CHRG_TEMPERATURE_MODE, (uint8_t)(config->useTemperatureDerating ? 1 : 0));
    prefsHandler->write(CHRG_DERATING_TEMPERATURE, config->deratingTemperature);
    prefsHandler->write(CHRG_DERATING_REFERENCE, config->deratingReferenceTemperature);
    prefsHandler->write(CHRG_HYSTERESE_STOP, config->hystereseStopTemperature);
    prefsHandler->write(CHRG_HYSTERESE_RESUME, config->hystereseResumeTemperature);

    prefsHandler->saveChecksum();
}
