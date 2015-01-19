/*
 * RX8Dash.h
 *
 * A quick and dirty class that drives an RX8 dash via canbus
 *
Copyright (c) 2015 Collin Kidder, Michael Neuweiler, Charles Galpin

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
 
#ifndef RX8DASH_H_
#define RX8DASH_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"
#include "DeviceManager.h"
#include "CanHandler.h"
#include "MotorController.h"

enum RX8DASH_STATE 
{
	SPEEDO,
	WARNING_LIGHTS,
	STEERING_WARNING,
	OTHER_GAUGES,
	CRUISE_CONTROL
};

class RX8Dash : public Device
{
public:
	RX8Dash();
	void setup();
	void handleTick();
	DeviceId getId();
protected:
private:
	RX8DASH_STATE state;
};

#endif
