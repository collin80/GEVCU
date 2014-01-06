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

#include "ThinkBatteryManager.h"

ThinkBatteryManager::ThinkBatteryManager() : BatteryManager() {
	prefsHandler = new PrefHandler(THINKBMS);
	allowCharge = false;
	allowDischarge = false;
}

/*For all multibyte integers the format is MSB first, LSB last
*/
void ThinkBatteryManager::handleCanFrame(CAN_FRAME *frame) {
	switch (frame->id) {
	case 0x300: //Start up message
		//we're not really interested in much here except whether init worked.
		if ((frame->data.bytes[6] & 1) == 0)  //there was an initialization error!
		{
			//set fault condition here
		}
		break;
	case 0x301: //System Data 0
		//first two bytes = current, next two voltage, next two DOD, last two avg. temp 
		//readings in tenths
		packVoltage = (frame->data.bytes[0] * 256 + frame->data.bytes[1]);
		packCurrent = (frame->data.bytes[2] * 256 + frame->data.bytes[3]);
		break;
	case 0x302: //System Data 1		 
		if ((frame->data.bytes[0] & 1) == 1) //Byte 0 bit 0 = general error
		{
			//raise a fault
		}
		if ((frame->data.bytes[2] & 1) == 1) //Byte 2 bit 0 = general isolation error
		{
			//raise a fault
		}
		//Min discharge voltage = bytes 4-5 - tenths of a volt
		//Max discharge current = bytes 6-7 - tenths of an amp
		break;
	case 0x303: //System Data 2
		//bytes 0-1 = max charge voltage (tenths of volt)
		//bytes 2-3 = max charge current (tenths of amp)
		//byte 4 bit 1 = regen braking OK, bit 2 = Discharging OK
		//byte 6 bit 3 = EPO (emergency power off) happened, bit 5 = battery pack fan is on
		break;
	case 0x304: //System Data 3
		//Byte 2 lower 4 bits = highest error category
		//categories: 0 = no faults, 1 = Reserved, 2 = Warning, 3 = Delayed switch off, 4 = immediate switch off
		//bytes 4-5 = Pack max temperature (tenths of degree C) - Signed
		//byte 6-7 = Pack min temperature (tenths of a degree C) - Signed
		lowestCellTemp = (S16)(frame->data.bytes[4] * 256 + frame->data.bytes[5]);
		highestCellTemp = (S16)(frame->data.bytes[6] * 256 + frame->data.bytes[7]);
		break;
	case 0x305: //System Data 4
		//byte 2 bits 0-3 = BMS state
		//0 = idle state, 1 = discharge state (contactor closed), 15 = fault state
		//byte 2 bit 4 = Internal HV isolation fault
		//byte 2 bit 5 = External HV isolation fault
		break;
	case 0x306: //System Data 5
		//bytes 0-1 = Equiv. internal resistance in milliohms
		//not recommended to rely on so probably just ignore it
		break;
	//technically there is a specification for frames 0x307 - 0x30A but I have never seen these frames
	//sent on the canbus system so I doubt that they are used.
/*
	case 0x307: //System Data 6
	case 0x308: //System Data 7
	case 0x309: //System Data 8
	case 0x30A: //System Data 9
	//do we care about the serial #? Probably not.
	case 0x30E: //Serial # part 1
	case 0x30B: //Serial # part 2
*/
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
	CAN_FRAME output;
	output.length = 3;
	output.id = 0x310;
	output.extended = 0; //standard frame
	output.rtr = 0;
	for (int i = 0; i < 8; i++) output.data.bytes[i] = 0;
	CanHandler::getInstanceEV()->sendFrame(output);

	output.id = 0x311;
	output.length = 2;
	CanHandler::getInstanceEV()->sendFrame(output);
}

DeviceId ThinkBatteryManager::getId() 
{
	return (THINKBMS);
}

bool ThinkBatteryManager::hasPackVoltage() 
{
	return true;
}

bool ThinkBatteryManager::hasPackCurrent() 
{
	return true;
}

bool ThinkBatteryManager::hasTemperatures() 
{
	return true;
}

bool ThinkBatteryManager::isChargeOK() 
{
	return allowCharge;
}

bool ThinkBatteryManager::isDischargeOK() 
{
	return allowDischarge;
}

