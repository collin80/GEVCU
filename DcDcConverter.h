/*
 * DcDcConverter.h
 *
 * Parent class for DC-DC Converters
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

#ifndef DCDCCONVERTER_H_
#define DCDCCONVERTER_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"
#include "CanHandler.h"

class DcDcConverterConfiguration : public DeviceConfiguration
{
public:
    bool boostMode; // use boost mode (true) or buck mode (false)
    uint16_t lowVoltageCommand; // in 0.1V, commanded LV voltage in buck mode
    uint16_t hvUndervoltageLimit; // in 1V, HV under-voltage limit in buck mode
    uint16_t lvBuckModeCurrentLimit; // in 1A
    uint16_t hvBuckModeCurrentLimit; // in 0.1A
    uint16_t highVoltageCommand; // 1V, commanded HV in boost mode
    uint16_t lvUndervoltageLimit; // in 0.1V, LV under-voltage limit in boost mode
    uint16_t lvBoostModeCurrentLinit; // in 1A
    uint16_t hvBoostModeCurrentLimit; // in 0.1A
};

class DcDcConverter : public Device
{
public:
	DcDcConverter();
    ~DcDcConverter();
    void handleStateChange(Status::SystemState);
    DeviceType getType();

    void loadConfiguration();
    void saveConfiguration();

protected:
    uint16_t hvVoltage; // in 0.1V
    uint16_t lvVoltage; // in 0.1V
    int16_t hvCurrent; // in 0.1A
    int16_t lvCurrent; // in 0.1A
    int16_t temperature; // in 0.1C

private:
};

#endif
