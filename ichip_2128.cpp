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

/*
 * Constructor. Assign serial interface to use for ichip communication
 */
ICHIPWIFI::ICHIPWIFI() {
	prefsHandler = new PrefHandler(ICHIP2128);

	uint8_t sys_type;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
	if (sys_type == 3 || sys_type == 4)
		serialInterface = &Serial2;
	else //older hardware used this instead
		serialInterface = &Serial3; 

	commonName = "WIFI (iChip2128)";
}

/*
 * Constructor. Pass serial interface to use for ichip communication
 */
ICHIPWIFI::ICHIPWIFI(USARTClass *which) {
	prefsHandler = new PrefHandler(ICHIP2128);
	serialInterface = which;
}

/*
 * Initialization of hardware and parameters
 */
void ICHIPWIFI::setup() {

	Logger::info("add device: iChip 2128 WiFi (id: %X, %X)", ICHIP2128, this);

	TickHandler::getInstance()->detach(this);

	//MSEL pin
	pinMode(18, OUTPUT);
	digitalWrite(18, HIGH);

	//RESET pin
	pinMode(42, OUTPUT);
	digitalWrite(42, HIGH);

	tickCounter = 0;
	ibWritePtr = 0;
	psWritePtr = 0;
	psReadPtr = 0;
	listeningSocket = 0;

	lastSentTime = millis();
	lastSentState = IDLE;
	lastSentCmd = String("");

	activeSockets[0] = -1;
	activeSockets[1] = -1;

	state = IDLE;

	didParamLoad = false;
	didTCPListener = false;

	serialInterface->begin(115200);

	paramCache.brakeNotAvailable = true;

	elmProc = new ELM327Processor();

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);

}

//A version of sendCmd that defaults to SET_PARAM which is what most of the code used to assume.
void ICHIPWIFI::sendCmd(String cmd) {
	sendCmd(cmd, SET_PARAM);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 * If the comm channel is busy it buffers the command
 */
void ICHIPWIFI::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate) {
	if (state != IDLE) { //if the comm is tied up then buffer this parameter for sending later
		if (availableBuffer() > 0) {
			sendingBuffer[psWritePtr].cmd = cmd;
			sendingBuffer[psWritePtr].state = cmdstate;
			psWritePtr = (psWritePtr + 1) & 63;
			if (Logger::isDebug() || localDebug) {
				String temp = "Buffer cmd: " + cmd;
				Logger::debug(ICHIP2128, (char *)temp.c_str());
				Logger::debug(ICHIP2128, "Avail buffer: %i", availableBuffer());
			}
		}
	}
	else { //otherwise, go ahead and blast away
		serialInterface->write(Constants::ichipCommandPrefix);
		serialInterface->print(cmd);
		serialInterface->write(13);
		state = cmdstate;
		lastSentTime = millis();
		lastSentCmd = String(cmd);
		lastSentState = cmdstate;

		if (Logger::isDebug() || localDebug) {
			String temp = "Send to ichip cmd: " + cmd;
			Logger::debug(ICHIP2128, (char *)temp.c_str());
		}
	}
}

void ICHIPWIFI::sendToSocket(int socket, String data) {
	char buff[6];
	sprintf(buff, "%03i", socket);
	String temp = "SSND%%:" + String(buff);
	sprintf(buff, ",%i:", data.length());
	temp = temp + String(buff) + data;
	sendCmd(temp, SEND_SOCKET);
}

/*
 * Periodic updates of parameters to ichip RAM.
 * Also query for changed parameters of the config page.
 */
//TODO: See the processing function below for a more detailed explanation - can't send so many setParam commands in a row
void ICHIPWIFI::handleTick() {
	static int pollListening = 0;
	static int pollSocket = 0;
	uint32_t ms = millis();
	char buff[6];
	uint8_t brklt;
	tickCounter++;

	if (ms < 1000) return; //wait 10 seconds for things to settle before doing a thing

	// Do a delayed parameter load once about a second after startup
	if (!didParamLoad && ms > 5000) {
	    loadParameters();
                  Logger::console("Wifi Parameters loaded...");
            paramCache.bitfield1 = motorController->getStatusBitfield1();
		setParam(Constants::bitfield1, paramCache.bitfield1);
	    paramCache.bitfield2 = motorController->getStatusBitfield2();
	    setParam(Constants::bitfield2, paramCache.bitfield2);
           // DeviceManager::getInstance()->updateWifiByID(BRUSA_DMC5);

	    didParamLoad = true;
	}

	//At 12 seconds start up a listening socket for OBDII
	if (!didTCPListener && ms > 12000) {
		sendCmd("LTCP:2000,2", START_TCP_LISTENER);
		didTCPListener = true;
	}

	if (listeningSocket > 9) {
		pollListening++;
		if (pollListening > 8) {
			pollListening = 0;
			char buff[5];
			sprintf(buff, "%u", listeningSocket);
			String temp = "LSST:" + String(buff);
			sendCmd(temp, GET_ACTIVE_SOCKETS);
		}
	}

	//read any information waiting on active sockets
	for (int c = 0; c < 2; c++) 
		if (activeSockets[c] != -1) {
			sprintf(buff, "%03i", activeSockets[c]);
			String temp = "SRCV:" + String(buff) + ",80";
			sendCmd(temp, GET_SOCKET);
		}

	//TODO:Testing line below. Remove it.
	//return;


	// make small slices so the main loop is not blocked for too long
	if (tickCounter == 1) {
		if (motorController) {
			//Logger::console("Wifi tick counter 1...");

			paramCache.timeRunning = ms;
			setParam(Constants::timeRunning, getTimeRunning());
			
			if ( paramCache.torqueRequested != motorController->getTorqueRequested() ) {
				paramCache.torqueRequested = motorController->getTorqueRequested();
				setParam(Constants::torqueRequested, paramCache.torqueRequested / 10.0f, 1);
			}
			if ( paramCache.torqueActual != motorController->getTorqueActual() ) {
				paramCache.torqueActual = motorController->getTorqueActual();
				setParam(Constants::torqueActual, paramCache.torqueActual / 10.0f, 1);
			}
		}
		if (throttle) {
            RawSignalData *rawSignal = throttle->acquireRawSignal();
			if ( paramCache.throttle !=  rawSignal->input1) {
				paramCache.throttle = rawSignal->input1;
                setParam(Constants::throttle, paramCache.throttle);
	
			/*if ( paramCache.throttle != throttle->getLevel() ) {
				paramCache.throttle = throttle->getLevel();
                                if (paramCache.throttle<-600){paramCache.throttle=-600;}
				setParam(Constants::throttle, paramCache.throttle / 10.0f, 1);*/
			}
		}
		if (brake) {
            RawSignalData *rawSignal = brake->acquireRawSignal();
			if ( paramCache.brake !=  rawSignal->input1) {
				paramCache.brake = rawSignal->input1;
	            paramCache.brakeNotAvailable = false;
	            setParam(Constants::brake, paramCache.brake);
	
			/*if ( paramCache.brake != brake->getLevel() ) {
				paramCache.brake = brake->getLevel();
				paramCache.brakeNotAvailable = false;
				setParam(Constants::brake, paramCache.brake / 10.0f, 1);*/
			}
		} else {
			if ( paramCache.brakeNotAvailable == true ) {
				paramCache.brakeNotAvailable = false; // no need to keep sending this
				setParam(Constants::brake, Constants::notAvailable);
			}
		}
	} else if (tickCounter == 2) {
		if (motorController) {
			//Logger::console("Wifi tick counter 2...");
			if ( paramCache.speedRequested != motorController->getSpeedRequested() ) {
				paramCache.speedRequested = motorController->getSpeedRequested();
				setParam(Constants::speedRequested, paramCache.speedRequested);
			}
			if ( paramCache.speedActual != motorController->getSpeedActual() ) {
				paramCache.speedActual = motorController->getSpeedActual();
                                if (paramCache.speedActual<0) paramCache.speedActual=0;
                                if (paramCache.speedActual>10000) paramCache.speedActual=10000;
				setParam(Constants::speedActual, paramCache.speedActual);
			}
			if ( paramCache.dcVoltage != motorController->getDcVoltage() ) {
				paramCache.dcVoltage = motorController->getDcVoltage();
                                if(paramCache.dcVoltage<1000) paramCache.dcVoltage=1000;  //Limits of the gage display
                                 if(paramCache.dcVoltage>4500) paramCache.dcVoltage=4500;
                               
				setParam(Constants::dcVoltage, paramCache.dcVoltage / 10.0f, 1);
			}
			if ( paramCache.dcCurrent != motorController->getDcCurrent() ) {
				paramCache.dcCurrent = motorController->getDcCurrent();
				setParam(Constants::dcCurrent, paramCache.dcCurrent / 10.0f, 1);
			}
			if ( paramCache.prechargeR != motorController->getprechargeR() ) {
				paramCache.prechargeR = motorController->getprechargeR();
				setParam(Constants::prechargeR, (uint16_t)paramCache.prechargeR);
			}

            if ( paramCache.prechargeRelay != motorController->getprechargeRelay() ) {
				paramCache.prechargeRelay = motorController->getprechargeRelay();
				setParam(Constants::prechargeRelay, (uint8_t) paramCache.prechargeRelay);
				//Logger::console("Precharge Relay %i", paramCache.prechargeRelay);
				//Logger::console("motorController:prechargeRelay:%d, paramCache.prechargeRelay:%d, Constants:prechargeRelay:%s", motorController->getprechargeRelay(),paramCache.prechargeRelay, Constants::prechargeRelay);
			}

            if ( paramCache.mainContactorRelay != motorController->getmainContactorRelay() ) {
				paramCache.mainContactorRelay = motorController->getmainContactorRelay();
				setParam(Constants::mainContactorRelay, (uint8_t) paramCache.mainContactorRelay);
			}
		}
	} else if (tickCounter == 3) {
		if (motorController) {
			//Logger::console("Wifi tick counter 2...");
			if ( paramCache.acCurrent != motorController->getAcCurrent() ) {
				paramCache.acCurrent = motorController->getAcCurrent();
				setParam(Constants::acCurrent, paramCache.acCurrent / 10.0f, 1);
			}

			//if ( paramCache.kiloWattHours != motorController->getkiloWattHours()/3600000 ) {
				paramCache.kiloWattHours = motorController->getKiloWattHours()/3600000;
                                if(paramCache.kiloWattHours<0)paramCache.kiloWattHours = 0;
                                if(paramCache.kiloWattHours>300)paramCache.kiloWattHours = 300;
                        	setParam(Constants::kiloWattHours, paramCache.kiloWattHours / 10.0f, 1);
			//}
                       
            if ( paramCache.nominalVolt != motorController->getnominalVolt()/10 ){
				paramCache.nominalVolt = motorController->getnominalVolt()/10;
				setParam(Constants::nominalVolt, paramCache.nominalVolt);
			}

			if ( paramCache.bitfield1 != motorController->getStatusBitfield1() ) {
				paramCache.bitfield1 = motorController->getStatusBitfield1();
				setParam(Constants::bitfield1, paramCache.bitfield1);
			}
			if ( paramCache.bitfield2 != motorController->getStatusBitfield2() ) {
				paramCache.bitfield2 = motorController->getStatusBitfield2();
				setParam(Constants::bitfield2, paramCache.bitfield2);
			}
			if ( paramCache.bitfield3 != motorController->getStatusBitfield3() ) {
				paramCache.bitfield3 = motorController->getStatusBitfield3();
				setParam(Constants::bitfield3, paramCache.bitfield3);
			}
			if ( paramCache.bitfield4 != motorController->getStatusBitfield4() ) {
				paramCache.bitfield4 = motorController->getStatusBitfield4();
				setParam(Constants::bitfield4, paramCache.bitfield4);
			}
			
		}
	} else if (tickCounter == 4) {
		if (motorController) {
			// Logger::console("Wifi tick counter 4...");
			if ( paramCache.running != motorController->isRunning() ) {
				paramCache.running = motorController->isRunning();
				setParam(Constants::running, (paramCache.running ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.faulted != motorController->isFaulted() ) {
				paramCache.faulted = motorController->isFaulted();
				setParam(Constants::faulted, (paramCache.faulted ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.warning != motorController->isWarning() ) {
				paramCache.warning = motorController->isWarning();
				setParam(Constants::warning, (paramCache.warning ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.gear != motorController->getSelectedGear() ) {
				paramCache.gear = motorController->getSelectedGear();
				setParam(Constants::gear, (uint16_t)paramCache.gear);
			}

			if ( paramCache.coolFan != motorController->getCoolFan() ) {
				paramCache.coolFan = motorController->getCoolFan();
				setParam(Constants::coolFan, (uint8_t) paramCache.coolFan);
			}

            if ( paramCache.coolOn != motorController->getCoolOn() ) {
				paramCache.coolOn = motorController->getCoolOn();
				setParam(Constants::coolOn, (uint8_t) paramCache.coolOn);
			}

            if ( paramCache.coolOff != motorController->getCoolOff() ) {
				paramCache.coolOff = motorController->getCoolOff();
				setParam(Constants::coolOff, (uint8_t) paramCache.coolOff);
			}

			if ( paramCache.brakeLight != motorController->getBrakeLight() ) {
				paramCache.brakeLight = motorController->getBrakeLight();
				setParam(Constants::brakeLight, (uint8_t) paramCache.brakeLight);
			}

			if ( paramCache.revLight != motorController->getRevLight() ) {
				paramCache.revLight = motorController->getRevLight();
				setParam(Constants::revLight, (uint8_t) paramCache.revLight);
			}

			if ( paramCache.enableIn != motorController->getEnableIn() ) {
				paramCache.enableIn = motorController->getEnableIn();
				setParam(Constants::enableIn, (uint8_t) paramCache.enableIn);
			}
			if ( paramCache.reverseIn != motorController->getReverseIn() ) {
				paramCache.reverseIn = motorController->getReverseIn();
				setParam(Constants::reverseIn, (uint8_t) paramCache.reverseIn);
			}

		}
	} else if (tickCounter > 4) {
		if (motorController) {
			// Logger::console("Wifi tick counter 5...");
			if ( paramCache.tempMotor != motorController->getTemperatureMotor() ) {
				paramCache.tempMotor = motorController->getTemperatureMotor();
				setParam(Constants::tempMotor, paramCache.tempMotor / 10.0f, 1);
			}
			if ( paramCache.tempInverter != motorController->getTemperatureInverter() ) {
				paramCache.tempInverter = motorController->getTemperatureInverter();
				setParam(Constants::tempInverter, paramCache.tempInverter / 10.0f, 1);
			}
			if ( paramCache.tempSystem != motorController->getTemperatureSystem() ) {
				paramCache.tempSystem = motorController->getTemperatureSystem();
				setParam(Constants::tempSystem, paramCache.tempSystem / 10.0f, 1);
			}

			if (paramCache.powerMode != motorController->getPowerMode() ) {
				paramCache.powerMode = motorController->getPowerMode();
				setParam(Constants::motorMode, (uint8_t)paramCache.powerMode);
			}

			//if ( paramCache.mechPower != motorController->getMechanicalPower() ) {
				paramCache.mechPower = motorController->getMechanicalPower();
                                if (paramCache.mechPower<-250)paramCache.mechPower=-250;
                                if (paramCache.mechPower>1500)paramCache.mechPower=1500;
				setParam(Constants::mechPower, paramCache.mechPower / 10.0f, 1);
			//}
		}
		tickCounter = 0;
		getNextParam();
	}
}

/*
 * Calculate the runtime in hh:mm:ss
   This runtime calculation is good for about 50 days of uptime.
   Of course, the sprintf is only good to 99 hours so that's a bit less time.
 */
char *ICHIPWIFI::getTimeRunning() {
	uint32_t ms = millis();
	int seconds = (int) (ms / 1000) % 60;
	int minutes = (int) ((ms / (1000 * 60)) % 60);
	int hours = (int) ((ms / (1000 * 3600)) % 24);
	sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
	return buffer;
}

/*
 * Handle a message sent by the DeviceManager.
 * Currently MSG_SET_PARAM is supported. The message should be a two element char pointer array 
 * containing the addresses of a two element char array. char *paramPtr[2] = { &param[0][0], &param[1][0] };
 * Element 0 of the base array (char param [2][20]; )should contain the name of the parameter to be changed
 * Element 1 of the base array should contain the new value to be set.   
 *
 *  sendMessage(DEVICE_WIFI, ICHIP2128, MSG_SET_PARAM,  paramPtr);	
 *    
 */
void ICHIPWIFI::handleMessage(uint32_t messageType, void* message) {
	Device::handleMessage(messageType, message);  //Only matters if message is MSG_STARTUP

	switch (messageType) {
  
	case MSG_SET_PARAM:{   //Sets a single parameter to a single value
  	        char **params = (char **)message;  //recast message as a two element array (params)		
              // Logger::console("Received Device: %s value %s",params[0], params[1]);
		setParam((char *)params[0], (char *)params[1]);
		break;
	}
	case MSG_CONFIG_CHANGE:{  //Loads all parameters to web site
		loadParameters();
		break;
        }
	case MSG_COMMAND:  //Sends a message to the WiReach module in the form of AT+imessage
		sendCmd((char *)message);
		break;
        }
    
}

/*
 * Determine if a parameter has changed
 * The result will be processed in loop() -> processParameterChange()
 */
void ICHIPWIFI::getNextParam() {
	sendCmd("WNXT", GET_PARAM); //send command to get next changed parameter
}

/*
 * Try to retrieve the value of the given parameter.
 */
void ICHIPWIFI::getParamById(String paramName) {
	sendCmd(paramName + "?", GET_PARAM);
}

/*
 * Set a parameter to the given string value
 */
void ICHIPWIFI::setParam(String paramName, String value) {
      sendCmd(paramName + "=\"" + value + "\"", SET_PARAM);
}

/*
 * Set a parameter to the given int32 value
 */
void ICHIPWIFI::setParam(String paramName, int32_t value) {
	sprintf(buffer, "%l", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint32 value
 */
void ICHIPWIFI::setParam(String paramName, uint32_t value) {
	sprintf(buffer, "%lu", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given sint16 value
 */
void ICHIPWIFI::setParam(String paramName, int16_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint16 value
 */
void ICHIPWIFI::setParam(String paramName, uint16_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint8 value
 */
void ICHIPWIFI::setParam(String paramName, uint8_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}
/*
 * Set a parameter to the given float value
 */
void ICHIPWIFI::setParam(String paramName, float value, int precision) {
	char format[10];
	sprintf(format, "%%.%df", precision);
	sprintf(buffer, format, value);
	setParam(paramName, buffer);
}

int ICHIPWIFI::availableBuffer(void)
{
  int head = psWritePtr;
  int tail = psReadPtr;
  if (head >= tail) return 64 - 1 - head + tail;
  return tail - head - 1;
}

/*
 * Called in the main loop (hopefully) in order to process serial input waiting for us
 * from the wifi module. It should always terminate its answers with 13 so buffer
 * until we get 13 (CR) and then process it.
 * 
 */

void ICHIPWIFI::loop() {
	int incoming;
	char buff[6];

	//Helps to determine average latency but it creates an avalanche. Use sparingly.
	//Logger::debug(ICHIP2128, "Entering IChip loop");

	while (serialInterface->available()) {
		incoming = serialInterface->read();
		if (incoming != -1) { //and there is no reason it should be -1
			if (incoming == 13 || ibWritePtr > 126) { // on CR or full buffer, process the line
				incomingBuffer[ibWritePtr] = 0; //null terminate the string
				ibWritePtr = 0; //reset the write pointer
				//what we do with the input depends on what state the ICHIP comm was set to.
				if (Logger::isDebug() || localDebug) {
					sprintf(buff, "%u", state);
					String temp = "In Data, state: " + String(buff);
					Logger::debug(ICHIP2128, (char *)temp.c_str());
					temp = "Data from ichip: " + String(incomingBuffer);
					Logger::debug(ICHIP2128, (char *)temp.c_str());

				}

				//SET_PARAM				
				//ichip echoes the command before replying. This allows for setting the proper state for below automatically
				if (strncmp(incomingBuffer, Constants::ichipCommandPrefix, 4) == 0) //starts with AT+i
				{
					if (strncmp(&incomingBuffer[4], "SSND", 4) == 0) state = SEND_SOCKET;
					if (strncmp(&incomingBuffer[4], "LTCP", 4) == 0) state = START_TCP_LISTENER;
					if (strncmp(&incomingBuffer[4], "LSST", 4) == 0) state = GET_ACTIVE_SOCKETS;
					if (strncmp(&incomingBuffer[4], "SRCV", 4) == 0) state = GET_SOCKET;
					if (strncmp(&incomingBuffer[4], "WNXT", 4) == 0) state = GET_PARAM;
					if (strchr(&incomingBuffer[4], '?') != NULL) state = GET_PARAM;
					if (strchr(&incomingBuffer[4], '=') != NULL) state = SET_PARAM;
				}
				else {
					switch (state) {
					case GET_PARAM: //reply from an attempt to read changed parameters from ichip
						if (strchr(incomingBuffer, '='))
							processParameterChange(incomingBuffer);
						break;	
					case SET_PARAM: //reply from sending parameters to the ichip
						break;
					case START_TCP_LISTENER: //reply from turning on a listening socket
					   //reply hopefully has the listening socket #.
					   if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
						   listeningSocket = atoi(&incomingBuffer[2]);
						   if (listeningSocket < 10) listeningSocket = 0;
						   if (listeningSocket > 11) listeningSocket = 0;
						   if (Logger::isDebug() || localDebug) {
							   sprintf(buff, "%u", listeningSocket);
							   Logger::debug(ICHIP2128, buff);
						   }
					   }
						break;
					case GET_ACTIVE_SOCKETS: //reply from asking for active connections
					    if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
						   activeSockets[0] = atoi(strtok(&incomingBuffer[3], ","));
						   activeSockets[1] = atoi(strtok(NULL, ","));
						   if (Logger::isDebug() || localDebug) {
							   sprintf(buff, "%i", activeSockets[0]);
							   Logger::debug(ICHIP2128, buff);
							   sprintf(buff, "%i", activeSockets[1]);
							   Logger::debug(ICHIP2128, buff);
						   }
					    }
					    break;
					case POLL_SOCKET: //reply from asking about state of socket and how much data it has

						break;
					case SEND_SOCKET: //reply from sending data over a socket
						break;
					case GET_SOCKET: //reply requesting the data pending on a socket
						//reply is I/<size>:data
						int dLen;
						//do not do anything if the socket read returned an error.
						if (strstr(incomingBuffer, "ERROR") == NULL) {
							if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
								dLen = atoi( strtok(&incomingBuffer[2], ":") );
								if (dLen > 0) //if no data waiting then it'd be stupid to process no command
								{
									String datastr = strtok(0, ":"); //get the rest of the string
									datastr.toLowerCase();
									String ret = elmProc->processELMCmd((char *)datastr.c_str());
									sendToSocket(0, ret); //TODO: need to actually track which socket requested this data
								}
							}
						}
						else //if a reply from a get socket request is an error then close the socket 
						{
							sendCmd("SCLS:000", IDLE); //close the socket and don't care about the reply
							sendCmd("SCLS:001", IDLE); //close the socket and don't care about the reply
						}
						break;
					case IDLE: //not sure whether to leave this info or go to debug. The ichip shouldn't be sending things in idle state
					default:
						//Logger::info(ICHIP2128, incomingBuffer);
						break;

					}

					//if we got an I/ reply then the command is done sending data. So, see if there is a buffered cmd to send.
					if (strstr(incomingBuffer, "I/") != NULL) {
						state = IDLE;
						sendBufferedCmd();
					}
				}
			} else { // add more characters
				if (incoming != 10) // don't add a LF character
					incomingBuffer[ibWritePtr++] = (char) incoming;
			}
		} 
		else return;
	}
	
	if (millis() > lastSentTime + 200) { //Absolutely no replies should take even 200ms!
		state = IDLE; //something went wrong so reset state
		sendBufferedCmd(); //skip the command that was ignored and go to the next.
	}
}

void ICHIPWIFI::sendBufferedCmd()
{
	if (psReadPtr != psWritePtr) { //if there is a parameter to send then do it
		String temp = "Sending buffered cmd: " + sendingBuffer[psReadPtr].cmd;
		if (Logger::isDebug() || localDebug) {
			Logger::debug(ICHIP2128, "Avail buffer: %i", availableBuffer());
			Logger::debug(ICHIP2128, (char *)temp.c_str());
		}

		sendCmd(sendingBuffer[psReadPtr].cmd, sendingBuffer[psReadPtr].state);
		psReadPtr = (psReadPtr + 1) & 63;
	}
}


/*
 * Process the parameter update from ichip we received as a response to AT+iWNXT.
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 */
void ICHIPWIFI::processParameterChange(char *key) {
	PotThrottleConfiguration *throttleConfig = NULL;
	PotThrottleConfiguration *brakeConfig = NULL;
	MotorControllerConfiguration *motorConfig = NULL;
	bool parameterFound = true;

	char *value = strchr(key, '=');
	if (!value)
		return;

	if (throttle)
		throttleConfig = (PotThrottleConfiguration *)throttle->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *)brake->getConfiguration();
	if(motorController)
		motorConfig = (MotorControllerConfiguration *)motorController->getConfiguration();

	value[0] = 0; // replace the '=' sign with a 0
	value++;
	if (value[0] == '"')
		value++; // if the value starts with a '"', advance one character
	if (value[strlen(value) - 1] == '"')
		value[strlen(value) - 1] = 0; // if the value ends with a '"' character, replace it with 0

	if (!strcmp(key, Constants::numThrottlePots) && throttleConfig) {
		throttleConfig->numberPotMeters = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleSubType) && throttleConfig) {
		throttleConfig->throttleSubType = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMin1) && throttleConfig) {
		throttleConfig->minimumLevel1 = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMin2) && throttleConfig) {
		throttleConfig->minimumLevel2 = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMax1) && throttleConfig) {
		throttleConfig->maximumLevel1 = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMax2) && throttleConfig) {
		throttleConfig->maximumLevel2 = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleRegenMax) && throttleConfig) {
		throttleConfig->positionRegenMaximum = atol(value) * 10;
	} else if (!strcmp(key, Constants::throttleRegenMin) && throttleConfig) {
		throttleConfig->positionRegenMinimum = atol(value) * 10;
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleFwd) && throttleConfig) {
		throttleConfig->positionForwardMotionStart = atol(value) * 10;
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMap) && throttleConfig) {
		throttleConfig->positionHalfPower = atol(value) * 10;
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMinRegen) && throttleConfig) {
		throttleConfig->minimumRegen = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMaxRegen) && throttleConfig) {
		throttleConfig->maximumRegen = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleCreep) && throttleConfig) {
		throttleConfig->creep = atol(value);
		throttle->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMin) && brakeConfig) {
		brakeConfig->minimumLevel1 = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMax) && brakeConfig) {
		brakeConfig->maximumLevel1 = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMinRegen) && brakeConfig) {
		brakeConfig->minimumRegen = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMaxRegen) && brakeConfig) {
		brakeConfig->maximumRegen = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::speedMax) && motorConfig) {
		motorConfig->speedMax = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::torqueMax) && motorConfig) {
		motorConfig->torqueMax = atol(value) * 10;
		motorController->saveConfiguration();
    } else if (!strcmp(key, Constants::coolFan) && motorConfig) {
		motorConfig->coolFan = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::coolOn) && motorConfig) {
		motorConfig->coolOn = (atol(value));
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::coolOff) && motorConfig) {
		motorConfig->coolOff = (atol(value));
		motorController->saveConfiguration();
      } else if (!strcmp(key, Constants::prechargeR) && motorConfig) {
		motorConfig->prechargeR = atol(value);
		motorController->saveConfiguration(); 
      } else if (!strcmp(key, Constants::prechargeRelay) && motorConfig) {
		motorConfig->prechargeRelay = atol(value);
		motorController->saveConfiguration();
      } else if (!strcmp(key, Constants::nominalVolt) && motorConfig) {
		motorConfig->nominalVolt = (atol(value))*10;
                motorController->saveConfiguration();
    
      } else if (!strcmp(key, Constants::mainContactorRelay) && motorConfig) {
		motorConfig->mainContactorRelay = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeLight) && motorConfig) {
		motorConfig->brakeLight = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::revLight) && motorConfig) {
		motorConfig->revLight = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::enableIn) && motorConfig) {
		motorConfig->enableIn = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::reverseIn) && motorConfig) {
		motorConfig->reverseIn = atol(value);  
		motorController->saveConfiguration();
      /*  } else if (!strcmp(key, Constants::motorMode) && motorConfig) {
		motorConfig->motorMode = (MotorController::PowerMode)atoi(value);
		motorController->saveConfiguration();	
	*/



        } else if (!strcmp(key, "x1000")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16),true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
                // Logger::console("Setting Device: %s ID:%X value %X",key, strtol(key+1, 0, 16),atol(value));
   
            } else if (!strcmp(key, "x1001")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16),true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1002")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16),true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1031")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1032")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1033")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1034")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1010")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
         } else if (!strcmp(key, "x1020")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
        } else if (!strcmp(key, "x1040")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
        } else if (!strcmp(key, "x2000")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
        } else if (!strcmp(key, "x6000")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();
        } else if (!strcmp(key, "x6500")){
                if (255==atol(value)){sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), true);}
                  else {sysPrefs->setDeviceStatus(strtol(key+1, 0, 16), false);}
                sysPrefs->forceCacheWrite();            
	
	} else if (!strcmp(key, Constants::logLevel)) {
		extern PrefHandler *sysPrefs;
		uint8_t loglevel = atoi(value);
		Logger::setLoglevel((Logger::LogLevel)loglevel);
		sysPrefs->write(EESYS_LOG_LEVEL, loglevel);
	} else {
		parameterFound = false;
	}
	if (parameterFound) {
		Logger::info(ICHIP2128, "parameter change: %s", key);
		getNextParam(); // try to get another one immediately
	}
}

/*
 * Get parameters from devices and forward them to ichip.
 * This is required to initially set-up the ichip
 */
void ICHIPWIFI::loadParameters() {
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();
	Throttle *throttle = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	PotThrottleConfiguration *acceleratorConfig = NULL;
	PotThrottleConfiguration *brakeConfig = NULL;
	MotorControllerConfiguration *motorConfig = NULL;

	Logger::info("loading config params to ichip/wifi");

	if (throttle)
		acceleratorConfig = (PotThrottleConfiguration *)throttle->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *)brake->getConfiguration();
	if (motorController)
		motorConfig = (MotorControllerConfiguration *)motorController->getConfiguration();

	if (acceleratorConfig) {
		setParam(Constants::numThrottlePots, acceleratorConfig->numberPotMeters);
		setParam(Constants::throttleSubType, acceleratorConfig->throttleSubType);
		setParam(Constants::throttleMin1, acceleratorConfig->minimumLevel1);
		setParam(Constants::throttleMin2, acceleratorConfig->minimumLevel2);
		setParam(Constants::throttleMax1, acceleratorConfig->maximumLevel1);
		setParam(Constants::throttleMax2, acceleratorConfig->maximumLevel2);
		setParam(Constants::throttleRegenMax, (uint16_t)(acceleratorConfig->positionRegenMaximum / 10));
		setParam(Constants::throttleRegenMin, (uint16_t)(acceleratorConfig->positionRegenMinimum / 10));
		setParam(Constants::throttleFwd, (uint16_t)(acceleratorConfig->positionForwardMotionStart / 10));
		setParam(Constants::throttleMap, (uint16_t)(acceleratorConfig->positionHalfPower / 10));
		setParam(Constants::throttleMinRegen, acceleratorConfig->minimumRegen);
		setParam(Constants::throttleMaxRegen, acceleratorConfig->maximumRegen);
		setParam(Constants::throttleCreep, acceleratorConfig->creep);
	}
	if (brakeConfig) {
		setParam(Constants::brakeMin, brakeConfig->minimumLevel1);
		setParam(Constants::brakeMax, brakeConfig->maximumLevel1);
		setParam(Constants::brakeMinRegen, brakeConfig->minimumRegen);
		setParam(Constants::brakeMaxRegen, brakeConfig->maximumRegen);
	}
	if (motorConfig) {
		setParam(Constants::speedMax, motorConfig->speedMax);
		setParam(Constants::coolFan, motorConfig->coolFan);
        setParam(Constants::coolOn, motorConfig->coolOn);
        setParam(Constants::coolOff, motorConfig->coolOff);
        setParam(Constants::brakeLight, motorConfig->brakeLight);
		setParam(Constants::revLight, motorConfig->revLight);
		setParam(Constants::enableIn, motorConfig->enableIn);
		setParam(Constants::reverseIn, motorConfig->reverseIn);
        setParam(Constants::prechargeR, motorConfig->prechargeR);
        setParam(Constants::prechargeRelay, motorConfig->prechargeRelay);
        setParam(Constants::mainContactorRelay, motorConfig->mainContactorRelay);
        uint16_t nmvlt = motorConfig->nominalVolt/10;
        setParam(Constants::nominalVolt, nmvlt);
		setParam(Constants::torqueMax, (uint16_t)(motorConfig->torqueMax / 10)); // skip the tenth's
	}
	setParam(Constants::logLevel, (uint8_t)Logger::getLogLevel());

	motorController->getSpeedActual()
}

DeviceType ICHIPWIFI::getType() {
	return DEVICE_WIFI;
}

DeviceId ICHIPWIFI::getId() {
	return (ICHIP2128);
}

void ICHIPWIFI::loadConfiguration() {
	WifiConfiguration *config = (WifiConfiguration *)getConfiguration();

	Device::loadConfiguration(); //call parent function

	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		Logger::debug(ICHIP2128, "Valid checksum so using stored wifi config values");

		//TODO: implement processing of config params for WIFI
//		prefsHandler->read(EESYS_WIFI0_SSID, &config->ssid);
	}
}

void ICHIPWIFI::saveConfiguration() {
	WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

	//TODO: implement processing of config params for WIFI
//	prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
//	prefsHandler->saveChecksum();
}
