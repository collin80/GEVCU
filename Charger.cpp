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
void Charger::handleStateChange(Status::SystemState state) {
    Device::handleStateChange(state);
    if (state == Status::charging) {
//TODO
//        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();
//        desiredOutputCurrent = config->constantCurrent;
        powerOn = true;
        chargeStartTime = millis();
        lastTick = millis();
        wattMilliSeconds = 0;
        ampereMilliSeconds = 0;
    } else {
        if (powerOn) {
            Logger::info(getId(), "Charging finished after %d min, %f Ah / %f kWh, final voltage %f, final current %f", (millis() - chargeStartTime) / 60, (float) ampereMilliSeconds / 3600000.0f, (float) wattMilliSeconds / 3600.0f, batteryVoltage, batteryCurrent);
        }
//        desiredOutputCurrent = 0;
        powerOn = false;
    }
    systemIO->setEnableCharger(powerOn);
}

/**
 * Calculate desired output voltage to battery in 0.1V
 */
//uint16_t Charger::getOutputVoltage() {
//    if (powerOn && running) {
//        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();
//        return config->constantVoltage;
//    }
//    return 0;
//}

/**
 * Calculate desired output current to battery in 0.1A
 */
//uint16_t Charger::getOutputCurrent() {
//    if (powerOn && running) {
//        ChargerConfiguration *config = (ChargerConfiguration *) getConfiguration();
//
//        // in constant voltage phase decrease current accordingly
//        if (batteryVoltage > config->constantVoltage) {
//            desiredOutputCurrent--; //TODO verify if this is an appropriate way to decrease the current
//        }
//    }
//    return 0;
//}

/*
 * Return the device type
 */
DeviceType Charger::getType()
{
    return (DEVICE_CHARGER);
}

uint16_t Charger::getBatteryCurrent() {
    return batteryCurrent;
}

int16_t Charger::getBatteryTemperature() {
    return batteryTemperature;
}

void Charger::setBatteryTemperature(int16_t batteryTemperature) {
    this->batteryTemperature = batteryTemperature;
}

uint16_t Charger::getBatteryVoltage() {
    return batteryVoltage;
}

uint16_t Charger::getInputCurrent() {
    return inputCurrent;
}

uint16_t Charger::getInputVoltage() {
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

#ifndef USE_HARD_CODED

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
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        Logger::warn(getId(), (char *) Constants::invalidChecksum);
        config->maximumInputCurrent = 100;
        config->constantCurrent = 100;
        config->constantVoltage = 4165;
        config->terminateCurrent = 50;
        config->minimumBatteryVoltage = 3272;
        config->maximumBatteryVoltage = 4284;
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

    prefsHandler->saveChecksum();
}
