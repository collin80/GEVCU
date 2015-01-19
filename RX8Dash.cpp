/*
 * RX8Dash.cpp
 *
 * Quick and dirty implementation of dash control via canbus
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

#include "RX8Dash.h"

RX8Dash::RX8Dash() : Device() {
	commonName = "RX8 Dash Driver";
	state = SPEEDO;
}

void RX8Dash::setup() {
	TickHandler::getInstance()->detach(this);

	Logger::info("add device: RX8 Dash Driver (id: %X, %X)", RX8DASH, this);

	Device::setup(); // run the parent class version of this function

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_RX8DASH);
}

void RX8Dash::handleTick() {
	Device::handleTick(); //kick the ball up to papa	
	MotorController *mcRef;
	int RPM = 3000;
	int Speed = 10000;
	int Temp = 85;

	CAN_FRAME send;

	send.extended = false;
	send.priority = 10;
	send.rtr = false;
	
	switch (state)
	{
	case SPEEDO:
		mcRef = DeviceManager::getInstance()->getMotorController();
		if (mcRef != NULL)
		{
			RPM = mcRef->getSpeedActual();
			Speed = (RPM / 10) + 9840;
			Temp = mcRef->getTemperatureSystem() + 60;
		}
		send.id = 0x201;
		send.length = 8;
		send.data.byte[0] = (byte)(RPM / 256);
		send.data.byte[1] = (byte)(RPM % 256);
		send.data.byte[2] = 0xFF; // Unknown, 0xFF from 'live'.
		send.data.byte[3] = 0xFF; // Unknown, 0xFF from 'live'.
		send.data.byte[4] = (byte)(Speed / 256);
		send.data.byte[5] = (byte)(Speed % 256);
		send.data.byte[6] = 0; // Unknown possible accelerator pedel if Madox is correct
		send.data.byte[7] = 0; //unknown
		state = WARNING_LIGHTS;
		break;
	case WARNING_LIGHTS:
		send.id = 0x212;
		send.length = 7;
		send.data.byte[0] = 0xFE; // Unknown
		send.data.byte[1] = 0xFE; // Unknown
		send.data.byte[2] = 0xFE; // Unknown
		send.data.byte[3] = 0x34; // DSC OFF in combo with byte 5 Live data only seen 0x34
		send.data.byte[4] = 0; // B01000000; // Brake warning B00001000;  //ABS warning
		send.data.byte[5] = 0x40; // TCS in combo with byte 3
		send.data.byte[6] = 0; // Unknown
		send.data.byte[7] = 0;
		state = STEERING_WARNING;
		break;
	case STEERING_WARNING:
		send.id = 0x300;
		send.length = 1;
		send.data.byte[0] = 0; // Steering Warning Light
		send.data.byte[1] = 0;
		send.data.byte[2] = 0;
		send.data.byte[3] = 0;
		send.data.byte[4] = 0;
		send.data.byte[5] = 0;
		send.data.byte[6] = 0;
		send.data.byte[7] = 0;
		state = OTHER_GAUGES;
		break;
	case OTHER_GAUGES:
		send.id = 0x420;
		send.length = 7;
		send.data.byte[0] = 0x98; // temp gauge //~170 is red, ~165 last bar, 152 centre, 90 first bar, 92 second bar
		send.data.byte[1] = 0; // something to do with trip meter 0x10, 0x11, 0x17 increments by 0.1 miles
		send.data.byte[2] = 0; //??
		send.data.byte[3] = 0; //??
		send.data.byte[4] = 0x01;// Oil Pressure (not really a gauge)
		send.data.byte[5] = 0;// check engine light
		send.data.byte[6] = 0;// Coolant, oil and battery
		send.data.byte[7] = 0;
		state = CRUISE_CONTROL;
		break;
	case CRUISE_CONTROL:
		send.id = 0x650;
		send.length = 1;
		send.data.byte[0] = 0; // Cruise Control Light 0x80 is Yellow, 0xFF is Green
		send.data.byte[1] = 0;
		send.data.byte[2] = 0;
		send.data.byte[3] = 0;
		send.data.byte[4] = 0;
		send.data.byte[5] = 0;
		send.data.byte[6] = 0;
		send.data.byte[7] = 0;
		state = SPEEDO;
		break;
	}

	//note that this uses getInstanceCar which is the second canbus. 
	CanHandler::getInstanceCar()->sendFrame(send);
}

DeviceId RX8Dash::getId() 
{
	return (RX8DASH);
}
