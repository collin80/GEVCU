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
}

void CanThrottle::setup() {
	//TickHandler::getInstance()->detach(this);

	loadConfiguration();
	Device::setup();

	//TODO: FIXME Quickly coded as hard coded values. This is naughty. 
	CanHandler::getInstanceCar()->attach(this, 0x745, 0x7E0, false);
	//TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_CAN_THROTTLE);
}

/*
 * There is no tick handler because this is the only place we do anything is here
*   	
	SAE standard says that this is the format for SAE requests to us:
	byte 0 = # of bytes following
	byte 1 = mode for PID request
	byte 2 = PID requested

	However, the sky is the limit for non-SAE frames (modes over 0x09)
	In that case we'll use two bytes for our custom PIDS (sent MSB first like
	all other PID traffic) MSB = byte 2, LSB = byte 3.

	These are the PIDs I think we should support (mode 1)
	0 = lets the other side know which pids we support. A bitfield that runs from MSb of first byte to lsb of last byte (32 bits)
	1 = Returns 32 bits but we really can only support the first byte which has bit 7 = Malfunction? Bits 0-6 = # of DTCs
	2 = Freeze DTC
	4 = Calculated engine load (A * 100 / 255) - Percentage
	5 = Engine Coolant Temp (A - 40) = Degrees Centigrade
	0x0C = Engine RPM (A * 256 + B) / 4
	0x11 = Throttle position (A * 100 / 255) - Percentage
	0x1C = Standard supported (We return 1 = OBDII)
	0x1F = runtime since engine start (A*256 + B)
	0x20 = pids supported (next 32 pids - formatted just like PID 0)
	0x21 = Distance traveled with fault light lit (A*256 + B) - In km
	0x2F = Fuel level (A * 100 / 255) - Percentage
	0x40 = PIDs supported, next 32
	0x51 = What type of fuel do we use? (We use 8 = electric, presumably.)
	0x60 = PIDs supported, next 32
	0x61 = Driver requested torque (A-125) - Percentage
	0x62 = Actual Torque delivered (A-125) - Percentage
	0x63 = Reference torque for engine - presumably max torque - A*256 + B
	
	Mode 3
	Returns DTC (diag trouble codes) - Three per frame
	bits 6-7 = DTC first character (00 = P = Powertrain, 01=C=Chassis, 10=B=Body, 11=U=Network)
	bits 4-5 = Second char (00 = 0, 01 = 1, 10 = 2, 11 = 3)
	bits 0-3 = third char (stored as normal nibble)
	Then next byte has two nibbles for the next 2 characters (fourth char = bits 4-7, fifth = 0-3)
	
	Mode 9 PIDs
	0x0 = Mode 9 pids supported (same scheme as mode 1)
	0x9 = How long is ECU name returned by 0x0A?
	0xA = ASCII string of ECU name. 20 characters are to be returned, I believe 4 at a time
	
 *
 */
void CanThrottle::handleCanFrame(RX_CAN_FRAME *frame) {
	TX_CAN_FRAME outputFrame;
	if (frame->id == 0x745) {
		//Do some common setup for our output - we won't pull the trigger unless we need to.
		outputFrame.id = frame->id + 8; //always return 8 higher than request
		outputFrame.data[1] = frame->data[1] + 0x40; //to show that this is a response
		outputFrame.data[2] = frame->data[2]; //copy standard PID
		if (frame->data[1] > 0x50) outputFrame.data[3] = frame->data[3]; //if using proprietary PIDs then copy second byte too
		
		switch (frame->data[1]) {
			case 1: //show current data
				break;
			case 2: //show freeze frame data - not sure we'll be supporting this
				break;
			case 3: //show stored diagnostic codes - we can probably map our faults to some existing DTC codes or roll our own
				break;
			case 4: //clear diagnostic trouble codes - If we get this frame we just clear all codes no questions asked.
				break;
			case 6: //test results over CANBus (this replaces mode 5 from non-canbus) - I know nothing of this
				break;
			case 7: //show pending diag codes (current or last driving cycle) - Might just overlap with mode 3
				break;
			case 8: //control operation of on-board systems - this sounds really proprietary and dangerous. Maybe ignore this?
				break;
			case 9: //request vehicle info - We can identify ourselves here but little else
				break;
			case 0x20: //custom PID codes we made up for GEVCU
				break;
		}
	}
}

DeviceId CanThrottle::getId() {
	return PIDLISTENER;
}

void CanThrottle::loadConfiguration() {
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
void CanThrottle::saveConfiguration() {
	CanPIDConfiguration *config = (CanPIDConfiguration *) getConfiguration();

	Device::saveConfiguration(); // call parent

	//prefsHandler->write(EETH_MIN_ONE, config->minimumLevel1);
	//prefsHandler->write(EETH_MAX_ONE, config->maximumLevel1);
	//prefsHandler->write(EETH_CAR_TYPE, config->carType);
	//prefsHandler->saveChecksum();
}

