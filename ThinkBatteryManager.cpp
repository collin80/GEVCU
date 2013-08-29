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
	case 0x300: //Start up message
	case 0x301: //System Data 0
	case 0x302: //System Data 1
	case 0x303: //System Data 2
	case 0x304: //System Data 3
	case 0x305: //System Data 4
	case 0x306: //System Data 5
	case 0x307: //System Data 6
	case 0x308: //System Data 7
	case 0x309: //System Data 8
	case 0x30A: //System Data 9
	case 0x30E: //Serial # part 1
	case 0x30B: //Serial # part 2
	}
}

void ThinkBatteryManager::setup() {
	TickHandler::getInstance()->detach(this);
	BatteryManager::setup(); // run the parent class version of this function

	//Relevant BMS messages are 0x300 - 0x30F
	CanHandler::getInstanceEV()->attach(this, 0x300, 0x7f0, false);	

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_BMS_THINK);
}

void ThinkBatteryManager::handleTick() {
	BatteryManager::handleTick(); //kick the ball up to papa

	sendKeepAlive();
	
}

//Contactors in pack will close if we sent these two frames with all zeros. 
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
