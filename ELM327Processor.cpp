#include "ELM327Processor.h"

ELM327Processor::ELM327Processor() {
	obd2Handler = OBD2Handler::getInstance();
}

String ELM327Processor::processELMCmd(char *cmd) {
	String retString = String();
	String lineEnding;
	if (bLineFeed) lineEnding = String("\r\n");
		else lineEnding = String("\r");

	if (!strncmp(cmd, "at", 2)) {

		if (!strcmp(cmd, "atz")) { //reset hardware
			retString.concat(lineEnding);
	 		retString.concat("ELM327 v1.3a");
		}
		else if (!strncmp(cmd, "atsh",4)) { //set header address
			//ignore this - just say OK
			retString.concat("OK");
		}
		else if (!strncmp(cmd, "ate",3)) { //turn echo on/off
			//could support echo but I don't see the need, just ignore this
			retString.concat("OK");
		}
		else if (!strncmp(cmd, "ath",3)) { //turn headers on/off
			if (cmd[3] == '1') bHeader = true;
				else bHeader = false;
			retString.concat("OK");
		}
		else if (!strncmp(cmd, "atl",3)) { //turn linefeeds on/off
			if (cmd[3] == '1') bLineFeed = true;
				else bLineFeed = false;
			retString.concat("OK");
		}
		else if (!strcmp(cmd, "at@1")) { //send device description
			retString.concat("ELM327 Emulator");
		}
		else if (!strcmp(cmd, "ati")) { //send chip ID
			retString.concat("ELM327 v1.3a");
		}
		else if (!strncmp(cmd, "atat",4)) { //set adaptive timing
			//don't intend to support adaptive timing at all
			retString.concat("OK");
		}
		else if (!strncmp(cmd, "atsp",4)) { //set protocol 
			//theoretically we can ignore this
			retString.concat("OK");
		}
		else if (!strcmp(cmd, "atdp")) { //show description of protocol
			retString.concat("can11/500");
		}
		else if (!strcmp(cmd, "atdpn")) { //show protocol number (same as passed to sp)
			retString.concat("6");
		}
		else if (!strcmp(cmd, "atd")) { //set to defaults
			retString.concat("OK");
		}
		else if (!strncmp(cmd, "atm", 3)) { //turn memory on/off
			retString.concat("OK");
		}
		else if (!strcmp(cmd, "atrv")) { //show 12v rail voltage
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
		if (strlen(cmd) == 4) {
			uint32_t valu = strtol((char *) cmd, NULL, 16); //the pid format is always in hex
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

	return retString; 	
}
