/*
 * ThinkBatteryManager.cpp
 *
 * Interface to the BMS which is within the Think City battery packs
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

#ifdef CFG_ENABLE_DEVICE_BMS_THINK
#include "ThinkBatteryManager.h"
#include "Params.h"

ThinkBatteryManager::ThinkBatteryManager() : BatteryManager() {
}

void ThinkBatteryManager::handleCanFrame(RX_CAN_FRAME *frame) {
	switch (frame->id) {
	}
}

void ThinkBatteryManager::setup() {
	TickHandler::getInstance()->detach(this);
	BatteryManager::setup(); // run the parent class version of this function

	// register ourselves as observer of 0x23x and 0x65x can frames
	CanHandler::getInstanceEV()->attach(this, 0x230, 0x7f0, false);
	CanHandler::getInstanceEV()->attach(this, 0x650, 0x7f0, false);

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_BMS_THINK);
}

void ThinkBatteryManager::handleTick() {
	BatteryManager::handleTick(); //kick the ball up to papa

	sendKeepAlive();
	
}

void ThinkBatteryManager::sendKeepAlive() 
{
	TX_CAN_FRAME output;
	output.dlc = 8;
	output.id = 0x310;
	output.ide = 0; //standard frame
	output.rtr = 0;
	for (int i = 0; i < 8; i++) output.data[i] = 0;
	CanHandler::getInstanceEV()->sendFrame(output);

	output.id = 0x311;
	CanHandler::getInstanceEV()->sendFrame(output);
}

Device::DeviceId ThinkBatteryManager::getId() {
	return (Device::THINKBMS);
}

#endif
