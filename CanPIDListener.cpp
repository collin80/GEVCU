/*
 * CanPIDListener.cpp
 *
 * Listens for PID requests over canbus and responds with the relevant information in the proper format
 *	
 * Currently this is in a very rough state and shouldn't be trusted - Make configuration work soon
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

#include "CanPIDListener.h"

CanPIDListener::CanPIDListener() : Device() {

	prefsHandler = new PrefHandler(PIDLISTENER);

	responseId = 0;
	responseMask = 0x7ff;
	responseExtended = false;

	obd2Handler = OBD2Handler::getInstance();
}

void CanPIDListener::setup() {
	//TickHandler::getInstance()->detach(this);

	loadConfiguration();
	Device::setup();

	//TODO: FIXME Quickly coded as hard coded values. This is naughty. 
	CanHandler::getInstanceEV()->attach(this, 0x7DF, 0x7DF, false);
	CanHandler::getInstanceEV()->attach(this, 0x7E0, 0x7E0, false);
	//TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_CAN_THROTTLE);
}

void CanPIDListener::handleCanFrame(CAN_FRAME *frame) {
	CAN_FRAME outputFrame;
	bool ret;

	if ((frame->id == 0x7E0) || (frame->id == 0x7DF)) {
		//Do some common setup for our output - we won't pull the trigger unless we need to.
		outputFrame.id = 0x7E8; //first ECU replying - TODO: Perhaps allow this to be configured from 0x7E8 - 0x7EF

		ret = false;

		//this assumes that we arent doing custom 16bit pids and that DTC doesn't return more than three faults.
		ret = obd2Handler->processRequest(frame->data.byte[1], frame->data.byte[2], NULL, (char *)frame->data.bytes);
	
		if (ret) {
			CanHandler::getInstanceEV()->sendFrame(outputFrame);
		}
	}
}

DeviceId CanPIDListener::getId() {
	return PIDLISTENER;
}

void CanPIDListener::loadConfiguration() {
	CanPIDConfiguration *config = (CanPIDConfiguration *) getConfiguration();

	if (!config) { // as lowest sub-class make sure we have a config object
		config = new CanPIDConfiguration();
		setConfiguration(config);
	}

	Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED
	if (false) {
#else
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
		Logger::debug(PIDLISTENER, (char *)Constants::validChecksum);
		//prefsHandler->read(EETH_MIN_ONE, &config->minimumLevel1);
		//prefsHandler->read(EETH_MAX_ONE, &config->maximumLevel1);
		//prefsHandler->read(EETH_CAR_TYPE, &config->carType);
	} else { //checksum invalid. Reinitialize values and store to EEPROM
		Logger::warn(PIDLISTENER, (char *)Constants::invalidChecksum);
		//config->minimumLevel1 = Throttle1MinValue;
		//config->maximumLevel1 = Throttle1MaxValue;
		//config->carType = Volvo_S80_Gas;
		saveConfiguration();
	}
	//Logger::debug(CANACCELPEDAL, "T1 MIN: %l MAX: %l Type: %d", config->minimumLevel1, config->maximumLevel1, config->carType);
}

/*
 * Store the current configuration to EEPROM
 */
void CanPIDListener::saveConfiguration() {
	CanPIDConfiguration *config = (CanPIDConfiguration *) getConfiguration();

	Device::saveConfiguration(); // call parent

	//prefsHandler->write(EETH_MIN_ONE, config->minimumLevel1);
	//prefsHandler->write(EETH_MAX_ONE, config->maximumLevel1);
	//prefsHandler->write(EETH_CAR_TYPE, config->carType);
	//prefsHandler->saveChecksum();
}

