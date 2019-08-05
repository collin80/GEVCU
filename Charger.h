/*
 * Charger.h
 *
 * Parent class for chargers
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

#ifndef CHARGER_H_
#define CHARGER_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceManager.h"

class ChargerConfiguration : public DeviceConfiguration
{
public:
    uint16_t maximumInputCurrent; // maximum input current in 0.1A (e.g. from 230V mains)
    uint16_t constantCurrent; // current in 0.1A during constant current phase
    uint16_t constantVoltage; // voltage in 0.1V during constant voltage phase and point where switching from constant current to constant voltage
    uint16_t terminateCurrent; // current in 0.1A at which to terminate the charge process

    uint16_t minimumBatteryVoltage; // minimum battery voltage in 0.1V where to start the charge process
    uint16_t maximumBatteryVoltage; // maximum battery voltage in 0.1V - if exceeded, the charge process will terminate
    int16_t minimumTemperature; // temperature in 0.1 deg Celsius below which charging will not occur
    uint16_t maximumTemperature; // temperature in 0.1 deg Celsius where charging is terminated
    uint16_t maximumAmpereHours; // charge in 0.1 Ah where charging is terminated
    uint16_t maximumChargeTime; // charge time in 1 minutes at which charging is terminated

    uint16_t deratingRate; // 0.1Ah per deg Celsius
    uint16_t deratingReferenceTemperature; // 0.1 deg Celsius where derating will reach 0 Amp (0=disable)
    uint16_t hystereseStopTemperature; // 0.1 deg Celsius where charging will stop in hysterese mode (0=disable)
    uint16_t hystereseResumeTemperature; // 0.1 deg Celsius where charging is resumed
};

class Charger : public Device
{
public:
    Charger();
    ~Charger();
    void handleTick();
    void handleStateChange(Status::SystemState, Status::SystemState);
    DeviceType getType();

    void loadConfiguration();
    void saveConfiguration();

    uint16_t getBatteryCurrent();
    uint16_t getBatteryVoltage();
    uint16_t getInputCurrent();
    uint16_t getInputVoltage();
    int16_t getTemperature();
    void setMaximumInputCurrent(uint16_t current);
    void setMaximumSolarCurrent(int16_t current);
    int16_t getMaximumSolarCurrent();

protected:
    uint16_t inputCurrent; // the reported input current in 0.01A
    uint16_t inputVoltage; // the reported input voltage in 0.1V
    uint16_t batteryVoltage; // the reported battery voltage in 0.1V
    uint16_t batteryCurrent; // the reported battery current in 0.01A
    int16_t temperature; // in 0.1 deg C
    uint32_t chargeStartTime; // timestamp when charging starts in millis
    uint32_t lastTick; // last time in ms when the handleTick method was called
    uint16_t calculateOutputVoltage();
    uint16_t calculateOutputCurrent();

private:
    uint64_t ampereMilliSeconds; // ampere hours put into the battery in 1 ampere-milliseconds (divide by 3600000 to get Ah)
    uint16_t requestedOutputCurrent; // calculated current to be delivered by the charger (in 0.1A), use getOutputCurrent() to retrieve this value - never use it directly !!
    int16_t maximumSolarCurrent; // the maximum current to be drawn from a solar power plant to avoid overload or draw from battery/mains power (-1 = ignore, in 0.1A)
};

#endif
