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
	~BatteryManager();
	int getPackVoltage(); //in tenths of a volt
	signed int getPackCurrent(); //in tenths of an amp
	//bool allowCharging();
	//bool allowDischarging();
	DeviceType getType();
    void setup();
    void handleTick();
	//a bunch of boolean functions. Derived classes must implment
	//these functions to tell everyone else what they support
	virtual bool hasPackVoltage() = 0;
	virtual bool hasPackCurrent() = 0;
	virtual bool hasTemperatures() = 0;
	virtual bool isChargeOK() = 0;
	virtual bool isDischargeOK() = 0;
protected:
	int packVoltage; //tenths of a volt
	signed int packCurrent; //tenths of an amp
	int SOC; //state of charge in percent
	int lowestCellV, highestCellV; //in mv
	int lowestCellTemp, highestCellTemp;
	//should be some form of discharge and charge limit. I don't know if it should be % or amps
	//some BMS systems will report one way and some the other. 
	int dischargeLimit, chargeLimit;
	bool allowCharge, allowDischarge;

private:
};

#endif
