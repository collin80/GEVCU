/*
 * Delphi DC-DC Converter Controller.cpp
 *
 * CAN Interface to the Delphi DC-DC converter - Handles sending of commands and reception of status frames to drive the DC-DC converter and set its output voltage.  SB following.
 *
Copyright (c) 2014 Jack Rickard

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



#include "DCDCController.h"
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 


	
DCDCController::DCDCController() : Device()
{
    prefsHandler = new PrefHandler(DCDC);
    //prefsHandler->setEnabledStatus(true);
   
    commonName = "Delphi DC-DC Converter";
    
}



void DCDCController::handleCanFrame(CAN_FRAME *frame) 
{
        Logger::debug("DCDC msg: %X", frame->id);
        Logger::debug("DCDC data: %X%X%X%X%X%X%X%X", frame->data.bytes[0],frame->data.bytes[1],frame->data.bytes[2],frame->data.bytes[3],frame->data.bytes[4],frame->data.bytes[5],frame->data.bytes[6],frame->data.bytes[7]);	
}



void DCDCController::setup()
{
	TickHandler::getInstance()->detach(this);

	loadConfiguration();
	Device::setup(); // run the parent class version of this function

	     CanHandler::getInstanceCar()->attach(this, 0x1D5, 0x7ff, false);
        //Watch for 0x1D5 messages from Delphi converter
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_DCDC);
}


void DCDCController::handleTick() {

	Device::handleTick(); //kick the ball up to papa

        sendCmd();   //Send our Delphi voltage control command
 
}


/*
1D7 08 80 77 00 00 00 00 00 00
For 13.0 vdc output.

1D7 08 80 8E 00 00 00 00 00 00
For 13.5 vdc output.

To request 14.0 vdc, the message was:
1D7 08 80 A5 00 00 00 00 00 00
*/

void DCDCController::sendCmd()
{
	DCDCConfiguration *config = (DCDCConfiguration *)getConfiguration();
	
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x1D7;
	output.extended = 0; //standard frame
	output.rtr = 0;  
        output.fid = 0; 
        output.data.bytes[0] = 0x80;
        output.data.bytes[1] = 0x8E;
        output.data.bytes[2] = 0;
        output.data.bytes[3] = 0;
        output.data.bytes[4] = 0;
        output.data.bytes[5] = 0;
        output.data.bytes[6] = 0;
        output.data.bytes[7] = 0x00;       
          
	CanHandler::getInstanceCar()->sendFrame(output);
        timestamp();
        Logger::debug("Delphi DC-DC cmd: %X %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
        output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);           
}

DeviceId DCDCController::getId() {
	return (DCDC);
}

uint32_t DCDCController::getTickInterval()
{
	return CFG_TICK_INTERVAL_DCDC;
}

void DCDCController::loadConfiguration() {
	DCDCConfiguration *config = (DCDCConfiguration *)getConfiguration();

	if (!config) {
		config = new DCDCConfiguration();
		setConfiguration(config);
	}

	Device::loadConfiguration(); // call parent
}

void DCDCController::saveConfiguration() {
	Device::saveConfiguration();
}

void DCDCController::timestamp()
{
   milliseconds = (int) (millis()/1) %1000 ;
   seconds = (int) (millis() / 1000) % 60 ;
    minutes = (int) ((millis() / (1000*60)) % 60);
    hours   = (int) ((millis() / (1000*60*60)) % 24);  
}


