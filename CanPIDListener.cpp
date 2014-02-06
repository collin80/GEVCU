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

void CanPIDListener::setup() {
	//TickHandler::getInstance()->detach(this);

	loadConfiguration();
	Device::setup();

	//TODO: FIXME Quickly coded as hard coded values. This is naughty. 
	CanHandler::getInstanceEV()->attach(this, 0x7DF, 0x7DF, false);
	CanHandler::getInstanceEV()->attach(this, 0x7E0, 0x7E0, false);
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
	0x63 = Reference torque for engine - presumably max torque - A*256 + B - Nm
	
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
void CanPIDListener::handleCanFrame(CAN_FRAME *frame) {
	CAN_FRAME outputFrame;
	bool ret;

	if ((frame->id == 0x7E0) || (frame->id = 0x7DF)) {
		//Do some common setup for our output - we won't pull the trigger unless we need to.
		outputFrame.id = 0x7E8; //first ECU replying - TODO: Perhaps allow this to be configured from 0x7E8 - 0x7EF
		outputFrame.data.bytes[1] = frame->data.bytes[1] + 0x40; //to show that this is a response
		outputFrame.data.bytes[2] = frame->data.bytes[2]; //copy standard PID
		outputFrame.data.bytes[0] = 2;
		if (frame->data.bytes[1] > 0x50) {
			outputFrame.data.bytes[3] = frame->data.bytes[3]; //if using proprietary PIDs then copy second byte too
			outputFrame.data.bytes[0] = 3;
		}

		ret = false;
		
		switch (frame->data.bytes[1]) {
			case 1: //show current data
				ret = processShowData(frame, outputFrame);
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

		//here is where we'd send out response. Right now it sends over canbus but when we support other
		//alteratives they'll be sending here too.
		if (ret) {
			CanHandler::getInstanceEV()->sendFrame(outputFrame);
		}
	}
}


//Process SAE standard PID requests. Function returns whether it handled the request or not.
bool CanPIDListener::processShowData(CAN_FRAME* inFrame, CAN_FRAME& outFrame) {
	MotorController* motorController = DeviceManager::getInstance()->getMotorController();
	int temp;

	switch (inFrame->data.bytes[2]) {
	case 0: //pids 1-0x20 that we support - bitfield
		//returns 4 bytes so immediately indicate that.
		outFrame.data.bytes[0] += 4;
		outFrame.data.bytes[3] = 0b11011000; //pids 1 - 8 - starting with pid 1 in the MSB and going from there
		outFrame.data.bytes[4] = 0b00010000; //pids 9 - 0x10
		outFrame.data.bytes[5] = 0b10000000; //pids 0x11 - 0x18
		outFrame.data.bytes[6] = 0b00010011; //pids 0x19 - 0x20
		return true;
		break;
	case 1: //Returns 32 bits but we really can only support the first byte which has bit 7 = Malfunction? Bits 0-6 = # of DTCs
		outFrame.data.bytes[0] += 4;
		outFrame.data.bytes[3] = 0; //TODO: We aren't properly keeping track of faults yet but when we do fix this. 
		outFrame.data.bytes[3] = 0; //these next three are really related to ICE diagnostics
		outFrame.data.bytes[3] = 0; //so ignore them.
		outFrame.data.bytes[3] = 0;
		return true;
		break;
	case 2: //Freeze DTC
		return false; //don't support freeze framing yet. Might be useful in the future.
		break;
	case 4: //Calculated engine load (A * 100 / 255) - Percentage
		temp = (255 * motorController->getTorqueActual()) / motorController->getTorqueAvailable();
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = (uint8_t)(temp & 0xFF);
		return true;
		break;
	case 5: //Engine Coolant Temp (A - 40) = Degrees Centigrade
		//our code stores temperatures as a signed integer for tenths of a degree so translate
		temp =  motorController->getTemperatureSystem() / 10;
		if (temp < -40) temp = -40;
		if (temp > 215) temp = 215;
		temp += 40;
		outFrame.data.bytes[0] += 1; //returning only one byte
		outFrame.data.bytes[3] = (uint8_t)(temp);
		return true;
		break;
	case 0xC: //Engine RPM (A * 256 + B) / 4
		temp = motorController->getSpeedActual() * 4; //we store in RPM while the PID code wants quarter rpms
		outFrame.data.bytes[0] += 2;
		outFrame.data.bytes[3] = (uint8_t)(temp / 256);
		outFrame.data.bytes[4] = (uint8_t)(temp);
		return true;
		break;
	case 0x11: //Throttle position (A * 100 / 255) - Percentage
		temp = motorController->getThrottle() / 10; //getThrottle returns in 10ths of a percent
		if (temp < 0) temp = 0; //negative throttle can't be shown for OBDII
		temp = (255 * temp) / 100;
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = (uint8_t)(temp);
		return true;
		break;
	case 0x1C: //Standard supported (We return 1 = OBDII)
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = 1;
		return true;
		break;
	case 0x1F: //runtime since engine start (A*256 + B)
		outFrame.data.bytes[0] += 2;
		outFrame.data.bytes[3] = 0; //TODO: Get the actual runtime.
		outFrame.data.bytes[4] = 0;
		return true;
		break;
	case 0x20: //pids supported (next 32 pids - formatted just like PID 0)
		outFrame.data.bytes[0] += 4;
		outFrame.data.bytes[3] = 0b00000000; //pids 0x21 - 0x28 - starting with pid 0x21 in the MSB and going from there
		outFrame.data.bytes[4] = 0b00000000; //pids 0x29 - 0x30
		outFrame.data.bytes[5] = 0b00000000; //pids 0x31 - 0x38
		outFrame.data.bytes[6] = 0b00000001; //pids 0x39 - 0x40
		return true;
		break;
	case 0x21: //Distance traveled with fault light lit (A*256 + B) - In km
		outFrame.data.bytes[0] += 2;
		outFrame.data.bytes[3] = 0; //TODO: Can we get this information?
		outFrame.data.bytes[4] = 0;
		return true;
		break;
	case 0x2F: //Fuel level (A * 100 / 255) - Percentage
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = 0; //TODO: finish BMS interface and get this value
		return true;
		break;
	case 0x40: //PIDs supported, next 32
		outFrame.data.bytes[0] += 4;
		outFrame.data.bytes[3] = 0b00000000; //pids 0x41 - 0x48 - starting with pid 0x41 in the MSB and going from there
		outFrame.data.bytes[4] = 0b00000000; //pids 0x49 - 0x50
		outFrame.data.bytes[5] = 0b10000000; //pids 0x51 - 0x58
		outFrame.data.bytes[6] = 0b00000001; //pids 0x59 - 0x60
		return true;
		break;
	case 0x51: //What type of fuel do we use? (We use 8 = electric, presumably.)
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = 8;
		return true;
		break;
	case 0x60: //PIDs supported, next 32
		outFrame.data.bytes[0] += 4;
		outFrame.data.bytes[3] = 0b11100000; //pids 0x61 - 0x68 - starting with pid 0x61 in the MSB and going from there
		outFrame.data.bytes[4] = 0b00000000; //pids 0x69 - 0x70
		outFrame.data.bytes[5] = 0b00000000; //pids 0x71 - 0x78
		outFrame.data.bytes[6] = 0b00000000; //pids 0x79 - 0x80
		return true;
		break;
	case 0x61: //Driver requested torque (A-125) - Percentage
		temp = (100 * motorController->getTorqueRequested()) / motorController->getTorqueAvailable();
		temp += 125;
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = (uint8_t)temp;
		return true;
		break;
	case 0x62: //Actual Torque delivered (A-125) - Percentage
		temp = (100 * motorController->getTorqueActual()) / motorController->getTorqueAvailable();
		temp += 125;
		outFrame.data.bytes[0] += 1;
		outFrame.data.bytes[3] = (uint8_t)temp;
		return true;
		break;
	case 0x63: //Reference torque for engine - presumably max torque - A*256 + B - Nm
		temp = motorController->getTorqueAvailable();
		outFrame.data.bytes[0] += 2;
		outFrame.data.bytes[3] = (uint8_t)(temp / 256);
		outFrame.data.bytes[4] = (uint8_t)(temp & 0xFF);
		return true;
		break;
	}
	return false;
}

bool CanPIDListener::processShowCustomData(CAN_FRAME* inFrame, CAN_FRAME& outFrame) {
	int pid = inFrame->data.bytes[2] * 256 + inFrame->data.bytes[3];
	switch (pid) {
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

