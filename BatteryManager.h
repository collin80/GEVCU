/*
 * BatteryManager.h
 *
 * Parent class for battery management / monitoring systems
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
 
#ifndef BATTMANAGE_H_
#define BATTMANAGE_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"
#include "DeviceManager.h"

class BatteryManager : public Device {
public:
	BatteryManager();
	int getPackVoltage(); //in tenths of a volt
	signed int getPackCurrent(); //in tenths of an amp
	bool allowCharging();
	bool allowDischarging();
	virtual DeviceType getType();
    virtual void setup();
    virtual void handleTick();
protected:
private:
	int packVoltage;
	signed int packCurrent;
	bool allowCharge, allowDischarge;
};

#endif
