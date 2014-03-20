/*
 *  ELM327_Emu.cpp
 *
 * Class emulates the serial comm of an ELM327 chip - Used to create an OBDII interface
 *
 * Created: 3/18/2014
 *  Author: Collin Kidder
 */

/*
 Copyright (c) 2013-2014 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#include "ELM327_Emu.h"

/*
 * Initialization of hardware and parameters
 */
void ELM327Emu::setup() {

	prefsHandler = new PrefHandler(ELM327EMU);

	TickHandler::getInstance()->detach(this);

	tickCounter = 0;
	ibWritePtr = 0;
	serialInterface->begin(9600);

	//this isn't a wifi link but the timer interval can be the same
	//because it serves a similar function and has similar timing requirements
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);

	bLineFeed = false;
	bHeader = false;

	obd2Handler = OBD2Handler::getInstance();
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 */
void ELM327Emu::sendCmd(String cmd) {
	serialInterface->write("AT");
	serialInterface->print(cmd);
	serialInterface->write(13);
	loop(); // parse the response
}

/*

 */
void ELM327Emu::handleTick() {

}

/*
 * Handle a message sent by the DeviceManager.
 */
void ELM327Emu::handleMessage(uint32_t messageType, void* message) {
	Device::handleMessage(messageType, message);

	switch (messageType) {
	case MSG_SET_PARAM: {
		break;
	}
	case MSG_CONFIG_CHANGE:
		break;
	case MSG_COMMAND:
		sendCmd((char *)message);
		break;
	}
}

/*
 * Constructor. Assign serial interface to use for comm with bluetooth adapter we're emulating with
 */
ELM327Emu::ELM327Emu() {
	prefsHandler = new PrefHandler(ELM327EMU);

	uint8_t sys_type;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
	if (sys_type == 3 || sys_type == 4)
		serialInterface = &Serial2;
	else //older hardware used this instead
		serialInterface = &Serial3; 
}

/*
 * Constructor. Pass serial interface to use
 */
ELM327Emu::ELM327Emu(USARTClass *which) {
	prefsHandler = new PrefHandler(ELM327EMU);
	serialInterface = which;
}

/*
 * Called in the main loop (hopefully) in order to process serial input waiting for us
 * from the wifi module. It should always terminate its answers with 13 so buffer
 * until we get 13 (CR) and then process it.
 * But, for now just echo stuff to our serial port for debugging
 */

void ELM327Emu::loop() {
	int incoming;
	while (serialInterface->available()) {
		incoming = serialInterface->read();
		if (incoming != -1) { //and there is no reason it should be -1
			if (incoming == 13 || ibWritePtr > 126) { // on CR or full buffer, process the line
				incomingBuffer[ibWritePtr] = 0; //null terminate the string
				ibWritePtr = 0; //reset the write pointer
				
				if (Logger::isDebug())
					Logger::debug(ELM327EMU, incomingBuffer);
				processCmd();
					
			} else { // add more characters
				if (incoming != 10 && incoming != ' ') // don't add a LF character or spaces. Strip them right out
					incomingBuffer[ibWritePtr++] = (char)tolower(incoming); //force lowercase to make processing easier
			}
		} else
		return;
	}
}

/*
*	There is no need to pass the string in here because it is local to the class so this function can grab it by default
*	But, for reference, this cmd processes the command in incomingBuffer
*/
void ELM327Emu::processCmd() {
	String retString = String();
	String lineEnding;
	if (bLineFeed) lineEnding = String("\r\n");
		else lineEnding = String("\r");

	if (!strncmp(incomingBuffer, "at", 2)) {

		if (!strcmp(incomingBuffer, "atz")) { //reset hardware
			retString.concat(lineEnding);
	 		retString.concat("ELM327 v1.3a");
		}
		else if (!strncmp(incomingBuffer, "atsh",4)) { //set header address
			//ignore this - just say OK
			retString.concat("OK");
		}
		else if (!strncmp(incomingBuffer, "ate",3)) { //turn echo on/off
			//could support echo but I don't see the need, just ignore this
			retString.concat("OK");
		}
		else if (!strncmp(incomingBuffer, "ath",3)) { //turn headers on/off
			if (incomingBuffer[3] == '1') bHeader = true;
				else bHeader = false;
			retString.concat("OK");
		}
		else if (!strncmp(incomingBuffer, "atl",3)) { //turn linefeeds on/off
			if (incomingBuffer[3] == '1') bLineFeed = true;
				else bLineFeed = false;
			retString.concat("OK");
		}
		else if (!strcmp(incomingBuffer, "at@1")) { //send device description
			retString.concat("ELM327 Emulator");
		}
		else if (!strcmp(incomingBuffer, "ati")) { //send chip ID
			retString.concat("ELM327 v1.3a");
		}
		else if (!strncmp(incomingBuffer, "atat",4)) { //set adaptive timing
			//don't intend to support adaptive timing at all
			retString.concat("OK");
		}
		else if (!strncmp(incomingBuffer, "atsp",4)) { //set protocol 
			//theoretically we can ignore this
			retString.concat("OK");
		}
		else if (!strcmp(incomingBuffer, "atdp")) { //show description of protocol
			retString.concat("can11/500");
		}
		else if (!strcmp(incomingBuffer, "atdpn")) { //show protocol number (same as passed to sp)
			retString.concat("6");
		}
		else if (!strcmp(incomingBuffer, "atd")) { //set to defaults
			retString.concat("OK");
		}
		else if (!strncmp(incomingBuffer, "atm", 3)) { //turn memory on/off
			retString.concat("OK");
		}
		else if (!strcmp(incomingBuffer, "atrv")) { //show 12v rail voltage
			//TODO: the system should actually have this value so it wouldn't hurt to
			//look it up and report the real value.
			retString.concat("14.2V");
		}
		else { //by default respond to anything not specifically handled by just saying OK and pretending.
			retString.concat("OK");
		}
	}
	else { //if no AT then assume it is a PID request. This takes the form of four bytes which form the alpha hex digit encoding for two bytes
		//there should be four or six characters here forming the ascii representation of the PID request. Easiest for now is to turn the ascii into
		//a 16 bit number and mask off to get the bytes
		if (strlen(incomingBuffer) == 4) {
			uint32_t valu = strtol((char *) incomingBuffer, NULL, 16); //the pid format is always in hex
			uint8_t pidnum = (uint8_t)(valu & 0xFF);
			uint8_t mode = (uint8_t)((valu >> 8) & 0xFF);
			Logger::debug(ELM327EMU, "Mode: %i, PID: %i", mode, pidnum);
			char out[6];
			char buff[10];
			if (obd2Handler->processRequest(mode, pidnum, NULL, out)) {
				if (bHeader) {
					retString.concat("7E8");
					out[0] += 2; //not sending only data bits but mode and pid too
					for (int i = 0; i <= out[0]; i++) {
						sprintf(buff, "%02X", out[i]);
						retString.concat(buff);
					}
				}
				else {
					mode += 0x40;
					sprintf(buff, "%02X", mode);
					retString.concat(buff);
					sprintf(buff, "%02X", pidnum);
					retString.concat(buff);
					for (int i = 1; i <= out[0]; i++) {
						sprintf(buff, "%02X", out[i+2]);
						retString.concat(buff);
					}
				}
			}
		}
	}

	retString.concat(lineEnding);
	retString.concat(">"); //prompt to show we're ready to receive again

	serialInterface->print(retString);
	if (Logger::isDebug()) {
		char buff[30];
		retString.toCharArray(buff, 30);
		Logger::debug(ELM327EMU, buff);
	}
	
}

DeviceType ELM327Emu::getType() {
	return DEVICE_MISC;
}

DeviceId ELM327Emu::getId() {
	return (ELM327EMU);
}

void ELM327Emu::loadConfiguration() {
	ELM327Configuration *config = new ELM327Configuration();

	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		Logger::debug(ELM327EMU, "Valid checksum so using stored elm327 emulator config values");
		//TODO: implement processing of config params for WIFI
//		prefsHandler->read(EESYS_WIFI0_SSID, &config->ssid);
	}
}

void ELM327Emu::saveConfiguration() {
	ELM327Configuration *config = (ELM327Configuration *) getConfiguration();

	//TODO: implement processing of config params for WIFI
//	prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
//	prefsHandler->saveChecksum();
}
 