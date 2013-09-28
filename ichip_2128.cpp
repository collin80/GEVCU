/*
 * ichip_2128.cpp
 *
 * Class to interface with the ichip 2128 based wifi adapter we're using on our board
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

#include "ichip_2128.h"

//initialization of hardware and parameters
void ICHIPWIFI::setup() {
	TickHandler::getInstance()->detach(this);

	ibWritePtr = 0;
	serialInterface->begin(115200);

	//for now force a specific ad-hoc network to be set up
	sendCmd("WLCH=6"); //use WIFI channel 6
	sendCmd("WLSI=!GEVCU"); //name our ADHOC network GEVCU (the ! indicates a ad-hoc network)
	sendCmd("DIP=192.168.3.10"); //IP of GEVCU is this
	sendCmd("DPSZ=10"); //serve up 10 more addresses (11 - 20)
	enableServer();

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);
}

void ICHIPWIFI::sendCmd(String cmd) {
	serialInterface->write("AT+i");
	serialInterface->print(cmd);
	serialInterface->write(13);
	delay(10); // let the ichip process the command

	//TODO: this is just a hack to keep the input buffer from overflowing
	loop();
}

//periodic processes
void ICHIPWIFI::handleTick() {

}

//turn on the web server
void ICHIPWIFI::enableServer() {
	sendCmd("WWW=3"); //turn on web server for three clients
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
}

//turn off the web server
void ICHIPWIFI::disableServer() {
	sendCmd("WWW=0"); //turn off web server
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
}

//Determine if a parameter has changed, which one, and the new value
String ICHIPWIFI::getNextParam() {
	sendCmd("WNXT"); //send command to get next changed parameter
}

//try to retrieve the value of the given parameter
String ICHIPWIFI::getParamById(String paramName) {
	serialInterface->write("AT+i");
	serialInterface->print(paramName);
	serialInterface->print("?");
	serialInterface->write(13);
}

//set the given parameter with the given string
String ICHIPWIFI::setParam(String paramName, String value) {
	serialInterface->write("AT+i");
	serialInterface->print(paramName);
	serialInterface->print("=");
	serialInterface->print(value);
	serialInterface->write(13);
}

ICHIPWIFI::ICHIPWIFI() {
#ifdef GEVCU3
	serialInterface = &Serial2;
#else
	serialInterface = &Serial3; //default is serial 3 because that should be what our shield really uses
#endif
}

ICHIPWIFI::ICHIPWIFI(USARTClass *which) {
	serialInterface = which;
}

//called in the main loop (hopefully) in order to process serial input waiting for us
//from the wifi module. It should always terminate its answers with 13 so buffer
//until we get 13 (CR) and then process it.
//But, for now just echo stuff to our serial port for debugging
void ICHIPWIFI::loop() {
	int incoming;
	while (serialInterface->available()) {
		incoming = serialInterface->read();
		if (incoming != -1) { //and there is no reason it should be -1
			if (incoming != 13 && ibWritePtr < 127) { //add to the line
				if (incoming != 10)
					incomingBuffer[ibWritePtr++] = (char) incoming;
			} else { //that's the end of the line. Try to figure out what it said.
				incomingBuffer[ibWritePtr] = 0; //null terminate the string
				ibWritePtr = 0; //reset the write pointer
				if (strcmp(incomingBuffer, "I/ERROR") == 0) { //got an error back!
				}
				Logger::debug(ICHIP2128, incomingBuffer);
			}
		} else
			return;
	}
}

DeviceType ICHIPWIFI::getType() {
	return DEVICE_WIFI;
}

DeviceId ICHIPWIFI::getId() {
	return (ICHIP2128);
}
