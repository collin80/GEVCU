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
ICHIPWIFI::ICHIPWIFI() : Device()
{
    prefsHandler = new PrefHandler(ICHIP2128);
    elmProc = new ELM327Processor();
    webSocket = new WebSocket(); //TODO each socket handler should get its own WebSocket class

    uint8_t sys_type;

    if (systemIO.getSystemType() == GEVCU3 || systemIO.getSystemType() == GEVCU4) {
        serialInterface = &Serial2;
    } else { //older hardware used this instead
        serialInterface = &Serial3;
    }
    serialInterface->begin(115200);

    commonName = "WIFI (iChip2128)";

    didParamLoad = false;
    didTCPListener = false;

    tickCounter = 0;
    ibWritePtr = 0;
    psWritePtr = 0;
    psReadPtr = 0;
    listeningSocket = 0;
    lastSendTime = 0;
    lastSentState = IDLE;
    state = IDLE;
    remainingSocketRead = -1;
}

/*
 * Initialization of hardware and parameters
 */
void ICHIPWIFI::setup()
{
    //RESET pin
    pinMode(42, OUTPUT);
    digitalWrite(42, HIGH);

    lastSendTime = millis();
    lastSentState = IDLE;
    state = IDLE;

    activeSockets[0] = -1;
    activeSockets[1] = -1;
    activeSockets[2] = -1;
    activeSockets[3] = -1;

    ready = true;
    running = true;

    tickHandler.attach(this, CFG_TICK_INTERVAL_WIFI);
}

/**
 * Tear down the device in a safe way.
 */
void ICHIPWIFI::tearDown()
{
    Device::tearDown();
    digitalWrite(42, LOW);
}

//A version of sendCmd that defaults to IDLE which is what most of the code used to assume.
void ICHIPWIFI::sendCmd(String cmd)
{
    sendCmd(cmd, IDLE);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 * If the comm channel is busy it buffers the command
 */
void ICHIPWIFI::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate)
{
    //if the comm is tied up, then buffer this parameter for sending later
    if (state != IDLE) {
        sendBuffer[psWritePtr].cmd = cmd;
        sendBuffer[psWritePtr++].state = cmdstate;
        if (psWritePtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psWritePtr = 0;
        }
        if (Logger::isDebug()) {
            Logger::debug(this, "Buffer cmd: %s, ", cmd.c_str());
        }
    } else { //otherwise, go ahead and blast away
        serialInterface->write(Constants::ichipCommandPrefix);
        serialInterface->print(cmd);
        serialInterface->write(13);
        state = cmdstate;
        lastSendTime = millis();
        lastSentState = cmdstate;
        if (Logger::isDebug()) {
            Logger::debug(this, "Send cmd: %s", cmd.c_str());
        }
    }
}

void ICHIPWIFI::sendBufferedCommand()
{
    state = IDLE;
    if (psReadPtr != psWritePtr) {
        //if there is a parameter in the buffer to send then do it
        sendCmd(sendBuffer[psReadPtr].cmd, sendBuffer[psReadPtr++].state);
        if (psReadPtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psReadPtr = 0;
        }
    }
}

void ICHIPWIFI::sendToSocket(int socket, String data)
{
    if (data.length() < 1) {
        return;
    }
    sprintf(buffer, "%03i", socket);
    String temp = "SSND%:" + String(buffer);
    sprintf(buffer, ",%i:", data.length());
    temp.concat(buffer);
    temp.concat(data);
    sendCmd(temp, SEND_SOCKET);
}

/*
 * Query for changed parameters of the config page, socket status
 * and query for incoming data on sockets
 */
void ICHIPWIFI::handleTick()
{
    uint32_t ms = millis();

    if (!didParamLoad) {
        if (ms < 3000) {
            return;    //wait a bit for things to settle before doing a thing
        }
        loadParameters();
        didParamLoad = true;
    }

    // start up a listening socket for OBDII and WebSocket
    if (!didTCPListener && ms > 12000) {
        sendCmd("LTCP:2000,4", START_TCP_LISTENER);
        didTCPListener = true;
    }

    // send updates to a connected WebSocket client
    if (webSocket->isConnected()) {
        sendToSocket(0, webSocket->getUpdate());
    }

    // check for open socket connections
    if (listeningSocket > 9) {
        if (tickCounter++ == 10) {
            sprintf(buffer, "LSST:%u", listeningSocket);
            sendCmd(buffer, GET_ACTIVE_SOCKETS);
        }
    }

    // read any information waiting on active sockets if we're not already in midst of it
    if (state != GET_SOCKET && tickCounter == 20) {
        for (int c = 0; c < 4; c++) {
            if (activeSockets[c] != -1) {
                sprintf(buffer, "SRCV:%03i,1024", activeSockets[c]);
                sendCmd(buffer, GET_SOCKET);
            }
        }
    }

    // check if params have changed, but only if idle so we don't mess-up other incoming data
    if (tickCounter >  29 && state == IDLE) {
        getNextParam();
    	tickCounter = 0;
    }
}

/*
 * Handle a message sent by the DeviceManager.
 * Currently MSG_SET_PARAM is supported. A array of two char * has to be included
 * in the message.
 */
void ICHIPWIFI::handleMessage(uint32_t messageType, void* message)
{
    Device::handleMessage(messageType, message);

    switch (messageType) {
    case MSG_SET_PARAM: {
        char **params = (char **) message;
        setParam((char *) params[0], (char *) params[1]);
        break;
    }

    case MSG_CONFIG_CHANGE:
        loadParameters();
        break;

    case MSG_COMMAND:
        sendCmd((char *) message);
        loop();
        break;

    case MSG_RESET:
        factoryDefaults();
        break;

    case MSG_LOG:
        if (webSocket->isConnected()) {
            char **params = (char **) message;
            sendToSocket(0, webSocket->getLogEntry(params[0], params[1], params[2]));
        }
        break;
    }
}

/**
 * act on state changes and send a (last) update to a websocket client
 */
void ICHIPWIFI::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);

    if (webSocket->isConnected()) {
        sendToSocket(0, webSocket->getUpdate());
    }

}


/*
 * Determine if a parameter has changed
 * The result will be processed in loop() -> processParameterChange()
 */
void ICHIPWIFI::getNextParam()
{
    sendCmd("WNXT", GET_PARAM);  //send command to get next changed parameter
}

/*
 * Try to retrieve the value of the given parameter.
 */
void ICHIPWIFI::getParamById(String paramName)
{
    sendCmd(paramName + "?", GET_PARAM);
}

/*
 * Set a parameter to the given string value
 */
void ICHIPWIFI::setParam(String paramName, String value)
{
    sendCmd(paramName + "=\"" + value + "\"", SET_PARAM);
}

/*
 * Set a parameter to the given int32 value
 */
void ICHIPWIFI::setParam(String paramName, int32_t value)
{
    sprintf(buffer, "%l", value);
    setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint32 value
 */
void ICHIPWIFI::setParam(String paramName, uint32_t value)
{
    sprintf(buffer, "%lu", value);
    setParam(paramName, buffer);
}

/*
 * Set a parameter to the given sint16 value
 */
void ICHIPWIFI::setParam(String paramName, int16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint16 value
 */
void ICHIPWIFI::setParam(String paramName, uint16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint8 value
 */
void ICHIPWIFI::setParam(String paramName, uint8_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/*
 * Set a parameter to the given float value
 */
void ICHIPWIFI::setParam(String paramName, float value, int precision)
{
    char format[10];
    sprintf(format, "%%.%df", precision);
    sprintf(buffer, format, value);
    setParam(paramName, buffer);
}

/*
 * Called in the main loop (hopefully) in order to process serial input waiting for us
 * from the wifi module. It should always terminate its answers with 13 so buffer
 * until we get 13 (CR) and then process it.
 *
 */
void ICHIPWIFI::loop()
{
    int incoming;
    while (serialInterface->available()) {
        incoming = serialInterface->read();
//SerialUSB.print((char)incoming);
        if (incoming == -1) { //and there is no reason it should be -1
            return;
        }
        if (remainingSocketRead > 0) {
            remainingSocketRead--;
        }

        // in GET_SOCKET mode, check if we need to parse the size info from a SRCV command yet
        if (state == GET_SOCKET && remainingSocketRead == -1 && ibWritePtr > 2 && incoming == ':' &&
                incomingBuffer[0] == 'I' && incomingBuffer[1] == '/' && isdigit(incomingBuffer[2])) {
            processSocketResponseSize();
            return;
        }

        if (incoming == 13 || ibWritePtr > (CFG_WIFI_BUFFER_SIZE - 2) || remainingSocketRead == 0) { // on CR or full buffer, process the line
            incomingBuffer[ibWritePtr] = 0; //null terminate the string
            ibWritePtr = 0; //reset the write pointer

            if (Logger::isDebug()) {
                Logger::debug(this, "incoming: '%s', state: %d", incomingBuffer, state);
            }

            //The ichip echoes our commands back at us. The safer option might be to verify that the command
            //we think we're about to process is really proper by validating the echo here. But, for now
            //just ignore echoes - except for SSND (socket send, where an error might be added at the end)
            if (strncmp(incomingBuffer, Constants::ichipCommandPrefix, 4) != 0) {
                switch (state) {
                case GET_PARAM: //reply from an attempt to read changed parameters from ichip
                    processParameterChange(incomingBuffer);
                    break;
                case SET_PARAM: //reply from sending parameters to the ichip
                    break;
                case START_TCP_LISTENER: //reply from turning on a listening socket
                    processStartTcpListenerRepsonse();
                    break;
                case GET_ACTIVE_SOCKETS: //reply from asking for active connections
                    processGetActiveSocketsResponse();
                    break;
                case POLL_SOCKET: //reply from asking about state of socket and how much data it has
                    break;
                case SEND_SOCKET: //reply from sending data over a socket
                    break;
                case GET_SOCKET: //reply requesting the data pending on a socket
                    processSocketGetResponse();
                    break;
                case IDLE:
                default:
                    //Logger::info(this, incomingBuffer);
                    break;
                }
            } else {
                // although we ignore all echo's of "AT+i", the "AT+iSSND" has the I/OK or I/ERROR in-line
                if (strstr(incomingBuffer, "AT+iSSND") != NULL) {
                    processSocketSendResponse();
                }
            }
            // if we got an I/OK or I/ERROR in the reply then the command is done sending data. So, see if there is a buffered cmd to send.
            if (strstr(incomingBuffer, "I/OK") != NULL || strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
                sendBufferedCommand();
            }
            return; // before processing the next line, return to the loop() to allow other devices to process.
        } else { // add more characters
            if (incoming != 10) { // don't add a LF character
                incomingBuffer[ibWritePtr++] = (char) incoming;
            }
        }
    }
    if (millis() > lastSendTime + 1000) {
        state = IDLE; //something went wrong so reset state
    }
}

void ICHIPWIFI::processStartTcpListenerRepsonse()
{
    //reply hopefully has the listening socket #.
    if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
        listeningSocket = atoi(&incomingBuffer[2]);
        if (listeningSocket < 10 || listeningSocket > 11) {
            listeningSocket = 0;
        }
        if (Logger::isDebug()) {
            Logger::debug(this, "socket handle: %i", listeningSocket);
        }
    }
}

void ICHIPWIFI::processGetActiveSocketsResponse()
{
    if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
        if (strncmp(incomingBuffer, "I/(", 3) == 0) {
            activeSockets[0] = atoi(strtok(&incomingBuffer[3], ","));
            activeSockets[1] = atoi(strtok(NULL, ","));
            activeSockets[2] = atoi(strtok(NULL, ","));
            activeSockets[3] = atoi(strtok(NULL, ","));
            if (Logger::isDebug()) {
                Logger::debug(this, "sockets: %i, %i, %i, %i", activeSockets[0], activeSockets[1], activeSockets[2], activeSockets[3]);
            }
        }
    } else {
        closeSockets();
    }
    // as the reply only contains "I/(000,-1,-1,-1)" it won't be recognized in loop() as "I/OK" or "I/ERROR",
    // to proceed with processing the buffer, we need to call it here.
    sendBufferedCommand();
}

void ICHIPWIFI::processSocketResponseSize()
{
    incomingBuffer[ibWritePtr] = 0; //null terminate the string
    ibWritePtr = 0; //reset the write pointer
    remainingSocketRead = atoi(&incomingBuffer[2]) - 1;
}

void ICHIPWIFI::processSocketGetResponse()
{
    if (strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
        closeSockets();
    }
    if (strcmp(incomingBuffer, "I/0") == 0) { // nothing to read, move on to next command
        remainingSocketRead = -1;
        sendBufferedCommand();
        return;
    }

    String ret;
    if (true) {
        // TODO do real differentiation between OBDII and WebSocket messages
        ret = String();
        bool previouslyConnected = webSocket->isConnected();
        webSocket->processInput(ret, incomingBuffer);
        sendToSocket(0, ret); //TODO: need to actually track which socket requested this data
        if (previouslyConnected && !webSocket->isConnected()) { // e.g. by receiving a close request
            closeSockets(); //TODO close only related socket
        }
        if (!previouslyConnected && webSocket->isConnected()) { // established connection but the line was empty
            remainingSocketRead = -1;
            sendBufferedCommand();
            return;
        }
    } else {
        String data = incomingBuffer;
        data.toLowerCase();
        ret = elmProc->processELMCmd((char*) (data.c_str()));
        sendToSocket(0, ret); //TODO: need to actually track which socket requested this data
    }
    // empty line marks end of transmission -> switch back to IDLE to be able to send (buffered) messages
    // beware that WebSocket relies on empty lines to indicate the end of transmission
    if (remainingSocketRead < 1 && strlen(incomingBuffer) != 0) {
        remainingSocketRead = -1;
        sendBufferedCommand();
    }
}

void ICHIPWIFI::processSocketSendResponse()
{
    // we get an error with SSND, when the socket was closed (e.g. "AT+iSSND%:000,24:I/ERROR (203)")
    if (strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
        webSocket->disconnected();
        Logger::info(this, "connection closed by remote client (%s), closing socket", incomingBuffer);
        closeSockets();
    }
}

void ICHIPWIFI::closeSockets() {
    for (int c = 0; c < 4; c++) {
        if (activeSockets[c] != -1) {
            char buff[6];
            sprintf(buff, "%03i", activeSockets[c]);
            String temp = "!SCLS:" + String(buff);
            sendCmd(temp, GET_SOCKET);
            activeSockets[c] = -1;
        }
    }
}
/*
 * Process the parameter update from ichip we received as a response to AT+iWNXT.
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 */
void ICHIPWIFI::processParameterChange(char *key)
{
    char *value = strchr(key, '=');

    if (!value) {
        return;
    }

    value[0] = 0; // replace the '=' sign with a 0
    value++;

    if (value[0] == '"') {
        value++;    // if the value starts with a '"', advance one character
    }
    if (value[strlen(value) - 1] == '"') {
        value[strlen(value) - 1] = 0;    // if the value ends with a '"' character, replace it with 0
    }

    if (!processParameterChangeThrottle(key, value) && !processParameterChangeBrake(key, value) &&
            !processParameterChangeMotor(key, value) && !processParameterChangeCharger(key, value) &&
            !processParameterChangeDcDc(key, value) && !processParameterChangeDevices(key, value) &&
            !processParameterChangeSystemIO(key, value)) {

    } else {
        Logger::info(this, "parameter change: %s = %s", key, value);
        getNextParam(); // try to get another one immediately
    }
}

bool ICHIPWIFI::processParameterChangeThrottle(char *key, char *value)
{
    Throttle *throttle = deviceManager.getAccelerator();

    if (throttle) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) throttle->getConfiguration();

        if(config) {
            if (!strcmp(key, Constants::numberPotMeters)) {
                config->numberPotMeters = atol(value);
            } else if (!strcmp(key, Constants::throttleSubType)) {
                config->throttleSubType = atol(value);
            } else if (!strcmp(key, Constants::minimumLevel)) {
                config->minimumLevel = atol(value);
            } else if (!strcmp(key, Constants::minimumLevel2)) {
                config->minimumLevel2 = atol(value);
            } else if (!strcmp(key, Constants::maximumLevel)) {
                config->maximumLevel = atol(value);
            } else if (!strcmp(key, Constants::maximumLevel2)) {
                config->maximumLevel2 = atol(value);
            } else if (!strcmp(key, Constants::positionRegenMaximum)) {
                config->positionRegenMaximum = atof(value) * 10;
            } else if (!strcmp(key, Constants::positionRegenMinimum)) {
                config->positionRegenMinimum = atof(value) * 10;
            } else if (!strcmp(key, Constants::positionForwardMotionStart)) {
                config->positionForwardMotionStart = atof(value) * 10;
            } else if (!strcmp(key, Constants::positionHalfPower)) {
                config->positionHalfPower = atof(value) * 10;
            } else if (!strcmp(key, Constants::minimumRegen)) {
                config->minimumRegen = atol(value);
            } else if (!strcmp(key, Constants::maximumRegen)) {
                config->maximumRegen = atol(value);
            } else if (!strcmp(key, Constants::creep)) {
                config->creep = atol(value);
            } else {
                return false;
            }
            throttle->saveConfiguration();
            return true;
        }
    }
    return false;
}

bool ICHIPWIFI::processParameterChangeBrake(char *key, char *value)
{
    Throttle *brake = deviceManager.getBrake();

    if (brake) {
        PotThrottleConfiguration *config = (PotThrottleConfiguration *) brake->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::brakeMinimumLevel)) {
                config->minimumLevel = atol(value);
            } else if (!strcmp(key, Constants::brakeMaximumLevel)) {
                config->maximumLevel = atol(value);
            } else if (!strcmp(key, Constants::brakeMinimumRegen)) {
                config->minimumRegen = atol(value);
            } else if (!strcmp(key, Constants::brakeMaximumRegen)) {
                config->maximumRegen = atol(value);
            } else {
                return false;
            }
            brake->saveConfiguration();
            return true;
        }
    }
    return false;
}

bool ICHIPWIFI::processParameterChangeMotor(char *key, char *value)
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::speedMax)) {
                config->speedMax = atol(value);
            } else if (!strcmp(key, Constants::torqueMax)) {
                config->torqueMax = atof(value) * 10;
            } else if (!strcmp(key, Constants::nominalVolt)) {
                config->nominalVolt = atof(value) * 10;
            } else if (!strcmp(key, Constants::motorMode)) {
                config->powerMode = (atol(value) ? modeSpeed : modeTorque);
            } else if (!strcmp(key, Constants::invertDirection)) {
                config->invertDirection = atol(value);
            } else if (!strcmp(key, Constants::slewRate)) {
                config->slewRate = atof(value) * 10;
            } else if (!strcmp(key, Constants::maxMechanicalPowerMotor)) {
                config->maxMechanicalPowerMotor = atof(value) * 10;
            } else if (!strcmp(key, Constants::maxMechanicalPowerRegen)) {
                config->maxMechanicalPowerRegen = atof(value) * 10;
            } else if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;

                if (!strcmp(key, Constants::dcVoltLimitMotor)) {
                    dmc5Config->dcVoltLimitMotor = atof(value) * 10;
                } else if (!strcmp(key, Constants::dcVoltLimitRegen)) {
                    dmc5Config->dcVoltLimitRegen = atof(value) * 10;
                } else if (!strcmp(key, Constants::dcCurrentLimitMotor)) {
                    dmc5Config->dcCurrentLimitMotor = atof(value) * 10;
                } else if (!strcmp(key, Constants::dcCurrentLimitRegen)) {
                    dmc5Config->dcCurrentLimitRegen = atof(value) * 10;
                } else if (!strcmp(key, Constants::enableOscillationLimiter)) {
                    dmc5Config->enableOscillationLimiter = atol(value);
                } else {
                    return false;
                }
            } else {
                return false;
            }
            motorController->saveConfiguration();
            return true;
        }
    }
    return false;
}

bool ICHIPWIFI::processParameterChangeCharger(char *key, char *value)
{
    Charger *charger = deviceManager.getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::maximumInputCurrent)) {
                config->maximumInputCurrent = atof(value) * 10;
            } else if (!strcmp(key, Constants::constantCurrent)) {
                config->constantCurrent = atof(value) * 10;
            } else if (!strcmp(key, Constants::constantVoltage)) {
                config->constantVoltage = atof(value) * 10;
            } else if (!strcmp(key, Constants::terminateCurrent)) {
                config->terminateCurrent = atof(value) * 10;
            } else if (!strcmp(key, Constants::minimumBatteryVoltage)) {
                config->minimumBatteryVoltage = atof(value) * 10;
            } else if (!strcmp(key, Constants::maximumBatteryVoltage)) {
                config->maximumBatteryVoltage = atof(value) * 10;
            } else if (!strcmp(key, Constants::minimumTemperature)) {
                config->minimumTemperature = atof(value) * 10;
            } else if (!strcmp(key, Constants::maximumTemperature)) {
                config->maximumTemperature = atof(value) * 10;
            } else if (!strcmp(key, Constants::maximumAmpereHours)) {
                config->maximumAmpereHours = atof(value) * 10;
            } else if (!strcmp(key, Constants::maximumChargeTime)) {
                config->maximumChargeTime = atol(value);
            } else if (!strcmp(key, Constants::deratingRate)) {
                config->deratingRate = atof(value) * 10;
            } else if (!strcmp(key, Constants::deratingReferenceTemperature)) {
                config->deratingReferenceTemperature = atof(value) * 10;
            } else if (!strcmp(key, Constants::hystereseStopTemperature)) {
                config->hystereseStopTemperature = atof(value) * 10;
            } else if (!strcmp(key, Constants::hystereseResumeTemperature)) {
                config->hystereseResumeTemperature = atof(value) * 10;
            } else {
                return false;
            }
            charger->saveConfiguration();
            return true;
        }
    }
    return false;
}

bool ICHIPWIFI::processParameterChangeDcDc(char *key, char *value)
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::dcDcMode)) {
                config->mode = atol(value);
            } else if (!strcmp(key, Constants::lowVoltageCommand)) {
                config->lowVoltageCommand = atof(value) * 10;
            } else if (!strcmp(key, Constants::hvUndervoltageLimit)) {
                config->hvUndervoltageLimit = atof(value);
            } else if (!strcmp(key, Constants::lvBuckModeCurrentLimit)) {
                config->lvBuckModeCurrentLimit = atol(value);
            } else if (!strcmp(key, Constants::hvBuckModeCurrentLimit)) {
                config->hvBuckModeCurrentLimit = atof(value) * 10;
            } else if (!strcmp(key, Constants::highVoltageCommand)) {
                config->highVoltageCommand = atol(value);
            } else if (!strcmp(key, Constants::lvUndervoltageLimit)) {
                config->lvUndervoltageLimit = atof(value) * 10;
            } else if (!strcmp(key, Constants::lvBoostModeCurrentLimit)) {
                config->lvBoostModeCurrentLinit = atol(value);
            } else if (!strcmp(key, Constants::hvBoostModeCurrentLimit)) {
                config->hvBoostModeCurrentLimit = atof(value) * 10;
            } else {
                return false;
            }
            dcDcConverter->saveConfiguration();
            return true;
        }
    }
    return false;
}

bool ICHIPWIFI::processParameterChangeSystemIO(char *key, char *value)
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (!strcmp(key, Constants::enableInput)) {
        config->enableInput = atol(value);
    } else if (!strcmp(key, Constants::chargePowerAvailableInput)) {
        config->chargePowerAvailableInput = atol(value);
    } else if (!strcmp(key, Constants::interlockInput)) {
        config->interlockInput = atol(value);
    } else if (!strcmp(key, Constants::reverseInput)) {
        config->reverseInput = atol(value);
    } else if (!strcmp(key, Constants::prechargeMillis)) {
        config->prechargeMillis = atol(value);
    } else if (!strcmp(key, Constants::prechargeRelayOutput)) {
        config->prechargeRelayOutput = atol(value);
    } else if (!strcmp(key, Constants::mainContactorOutput)) {
        config->mainContactorOutput = atol(value);
    } else if (!strcmp(key, Constants::secondaryContactorOutput)) {
        config->secondaryContactorOutput = atol(value);
    } else if (!strcmp(key, Constants::fastChargeContactorOutput)) {
        config->fastChargeContactorOutput = atol(value);
    } else if (!strcmp(key, Constants::enableMotorOutput)) {
        config->enableMotorOutput = atol(value);
    } else if (!strcmp(key, Constants::enableChargerOutput)) {
        config->enableChargerOutput = atol(value);
    } else if (!strcmp(key, Constants::enableDcDcOutput)) {
        config->enableDcDcOutput = atol(value);
    } else if (!strcmp(key, Constants::enableHeaterOutput)) {
        config->enableHeaterOutput = atol(value);
    } else if (!strcmp(key, Constants::heaterValveOutput)) {
        config->heaterValveOutput = atol(value);
    } else if (!strcmp(key, Constants::heaterPumpOutput)) {
        config->heaterPumpOutput = atol(value);
    } else if (!strcmp(key, Constants::coolingPumpOutput)) {
        config->coolingPumpOutput = atol(value);
    } else if (!strcmp(key, Constants::coolingFanOutput)) {
        config->coolingFanOutput = atol(value);
    } else if (!strcmp(key, Constants::coolingTempOn)) {
        config->coolingTempOn = atol(value);
    } else if (!strcmp(key, Constants::coolingTempOff)) {
        config->coolingTempOff = atol(value);
    } else if (!strcmp(key, Constants::brakeLightOutput)) {
        config->brakeLightOutput = atol(value);
    } else if (!strcmp(key, Constants::reverseLightOutput)) {
        config->reverseLightOutput = atol(value);
        systemIO.saveConfiguration();
    } else if (!strcmp(key, Constants::warningOutput)) {
        config->warningOutput = atol(value);
    } else if (!strcmp(key, Constants::powerLimitationOutput)) {
        config->powerLimitationOutput = atol(value);
    } else {
        return false;
    }
    systemIO.saveConfiguration();
    return true;
}

bool ICHIPWIFI::processParameterChangeDevices(char *key, char *value)
{
    if (!strcmp(key, Constants::logLevel)) {
        Logger::LogLevel logLevel = (Logger::LogLevel) atoi(value);
        Logger::setLoglevel(logLevel);
        systemIO.setLogLevel(logLevel);
    } else if (key[0] == 'x' && atol(&key[1]) > 0) {
        long deviceId = strtol(key + 1, 0, 16);
        deviceManager.sendMessage(DEVICE_ANY, (DeviceId) deviceId, (atol(value) ? MSG_ENABLE : MSG_DISABLE), NULL);
        return true;
    }
    return false;
}

/*
 * Get parameters from devices and forward them to ichip.
 * This is required to initially set-up the ichip
 */
void ICHIPWIFI::loadParameters()
{
    Logger::info("loading config params to wifi...");

    loadParametersThrottle();
    loadParametersBrake();
    loadParametersMotor();
    loadParametersCharger();
    loadParametersDcDc();
    loadParametersSystemIO();
    loadParametersDevices();
    loadParametersDashboard();

    Logger::info("wifi parameters loaded");
}

void ICHIPWIFI::loadParametersThrottle()
{
    Throttle *throttle = deviceManager.getAccelerator();

    if (throttle) {
        ThrottleConfiguration *throttleConfig = (ThrottleConfiguration *) throttle->getConfiguration();

        if (throttleConfig) {
            if (throttle->getId() == POTACCELPEDAL) {
                PotThrottleConfiguration *potThrottleConfig = (PotThrottleConfiguration *) throttleConfig;
                setParam(Constants::numberPotMeters, potThrottleConfig->numberPotMeters);
                setParam(Constants::throttleSubType, potThrottleConfig->throttleSubType);
                setParam(Constants::minimumLevel2, potThrottleConfig->minimumLevel2);
                setParam(Constants::maximumLevel2, potThrottleConfig->maximumLevel2);
            } else { // set reasonable values so the config page can be saved
                setParam(Constants::numberPotMeters, (uint8_t) 1);
                setParam(Constants::throttleSubType, (uint8_t)0);
                setParam(Constants::minimumLevel2, (uint16_t) 50);
                setParam(Constants::maximumLevel2, (uint16_t) 4095);
            }
            setParam(Constants::minimumLevel, throttleConfig->minimumLevel);
            setParam(Constants::maximumLevel, throttleConfig->maximumLevel);
            setParam(Constants::positionRegenMaximum, (uint16_t) (throttleConfig->positionRegenMaximum / 10));
            setParam(Constants::positionRegenMinimum, (uint16_t) (throttleConfig->positionRegenMinimum / 10));
            setParam(Constants::positionForwardMotionStart, (uint16_t) (throttleConfig->positionForwardMotionStart / 10));
            setParam(Constants::positionHalfPower, (uint16_t) (throttleConfig->positionHalfPower / 10));
            setParam(Constants::minimumRegen, throttleConfig->minimumRegen);
            setParam(Constants::maximumRegen, throttleConfig->maximumRegen);
            setParam(Constants::creep, throttleConfig->creep);
        }
    }
}

void ICHIPWIFI::loadParametersBrake()
{
    Throttle *brake = deviceManager.getBrake();

    if (brake) {
        ThrottleConfiguration *brakeConfig = (ThrottleConfiguration *) brake->getConfiguration();

        if (brakeConfig) {
            setParam(Constants::brakeMinimumLevel, brakeConfig->minimumLevel);
            setParam(Constants::brakeMaximumLevel, brakeConfig->maximumLevel);
            setParam(Constants::brakeMinimumRegen, brakeConfig->minimumRegen);
            setParam(Constants::brakeMaximumRegen, brakeConfig->maximumRegen);
        }
    }
}

void ICHIPWIFI::loadParametersMotor()
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            setParam(Constants::speedMax, config->speedMax);
            setParam(Constants::nominalVolt, config->nominalVolt / 10.0f, 1);
            setParam(Constants::torqueMax, config->torqueMax / 10.0f, 1);
            setParam(Constants::motorMode, (uint8_t) config->powerMode);
            setParam(Constants::invertDirection, (uint8_t)(config->invertDirection ? 1 : 0));
            setParam(Constants::slewRate, config->slewRate / 10.0f, 1);
            setParam(Constants::maxMechanicalPowerMotor, config->maxMechanicalPowerMotor / 10.0f, 1);
            setParam(Constants::maxMechanicalPowerRegen, config->maxMechanicalPowerRegen / 10.0f, 1);
            if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
                setParam(Constants::dcVoltLimitMotor, dmc5Config->dcVoltLimitMotor / 10.0f, 1);
                setParam(Constants::dcVoltLimitRegen, dmc5Config->dcVoltLimitRegen / 10.0f, 1);
                setParam(Constants::dcCurrentLimitMotor, dmc5Config->dcCurrentLimitMotor / 10.0f, 1);
                setParam(Constants::dcCurrentLimitRegen, dmc5Config->dcCurrentLimitRegen / 10.0f, 1);
                setParam(Constants::enableOscillationLimiter, (uint8_t)(dmc5Config->enableOscillationLimiter ? 1 : 0));
            }
        }
    }
}

void ICHIPWIFI::loadParametersCharger()
{
    Charger *charger = deviceManager.getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (config) {
            setParam(Constants::maximumInputCurrent, config->maximumInputCurrent / 10.0f, 1);
            setParam(Constants::constantCurrent, config->constantCurrent / 10.0f, 1);
            setParam(Constants::constantVoltage, config->constantVoltage / 10.0f, 1);
            setParam(Constants::terminateCurrent, config->terminateCurrent / 10.0f, 1);
            setParam(Constants::minimumBatteryVoltage, config->minimumBatteryVoltage / 10.0f, 1);
            setParam(Constants::maximumBatteryVoltage, config->maximumBatteryVoltage / 10.0f, 1);
            setParam(Constants::minimumTemperature, config->minimumTemperature / 10.0f, 1);
            setParam(Constants::maximumTemperature, config->maximumTemperature / 10.0f, 1);
            setParam(Constants::maximumAmpereHours, config->maximumAmpereHours / 10.0f, 1);
            setParam(Constants::maximumChargeTime, config->maximumChargeTime);
            setParam(Constants::deratingRate, config->deratingRate / 10.0f, 1);
            setParam(Constants::deratingReferenceTemperature, config->deratingReferenceTemperature / 10.0f, 1);
            setParam(Constants::hystereseStopTemperature, config->hystereseStopTemperature / 10.0f, 1);
            setParam(Constants::hystereseResumeTemperature, config->hystereseResumeTemperature / 10.0f, 1);
        }
    }
}

void ICHIPWIFI::loadParametersDcDc()
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *dcDcConfig = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (dcDcConfig) {
            setParam(Constants::dcDcMode, dcDcConfig->mode);
            setParam(Constants::lowVoltageCommand, dcDcConfig->lowVoltageCommand / 10.0f, 1);
            setParam(Constants::hvUndervoltageLimit, dcDcConfig->hvUndervoltageLimit);
            setParam(Constants::lvBuckModeCurrentLimit, dcDcConfig->lvBuckModeCurrentLimit);
            setParam(Constants::hvBuckModeCurrentLimit, dcDcConfig->hvBuckModeCurrentLimit / 10.0f, 1);
            setParam(Constants::highVoltageCommand, dcDcConfig->highVoltageCommand);
            setParam(Constants::lvUndervoltageLimit, dcDcConfig->lvUndervoltageLimit / 10.0f, 1);
            setParam(Constants::lvBoostModeCurrentLimit, dcDcConfig->lvBoostModeCurrentLinit);
            setParam(Constants::hvBoostModeCurrentLimit, dcDcConfig->hvBoostModeCurrentLimit / 10.0f, 1);
        }
    }
}

void ICHIPWIFI::loadParametersSystemIO()
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (config) {
        setParam(Constants::enableInput, config->enableInput);
        setParam(Constants::chargePowerAvailableInput, config->chargePowerAvailableInput);
        setParam(Constants::interlockInput, config->interlockInput);
        setParam(Constants::reverseInput, config->reverseInput);

        setParam(Constants::prechargeMillis, config->prechargeMillis);
        setParam(Constants::prechargeRelayOutput, config->prechargeRelayOutput);
        setParam(Constants::mainContactorOutput, config->mainContactorOutput);
        setParam(Constants::secondaryContactorOutput, config->secondaryContactorOutput);
        setParam(Constants::fastChargeContactorOutput, config->fastChargeContactorOutput);

        setParam(Constants::enableMotorOutput, config->enableMotorOutput);
        setParam(Constants::enableChargerOutput, config->enableChargerOutput);
        setParam(Constants::enableDcDcOutput, config->enableDcDcOutput);
        setParam(Constants::enableHeaterOutput, config->enableHeaterOutput);

        setParam(Constants::heaterValveOutput, config->heaterValveOutput);
        setParam(Constants::heaterPumpOutput, config->heaterPumpOutput);
        setParam(Constants::coolingPumpOutput, config->coolingPumpOutput);
        setParam(Constants::coolingFanOutput, config->coolingFanOutput);
        setParam(Constants::coolingTempOn, config->coolingTempOn);
        setParam(Constants::coolingTempOff, config->coolingTempOff);

        setParam(Constants::brakeLightOutput, config->brakeLightOutput);
        setParam(Constants::reverseLightOutput, config->reverseLightOutput);
        setParam(Constants::warningOutput, config->warningOutput);
        setParam(Constants::powerLimitationOutput, config->powerLimitationOutput);
    }
}

void ICHIPWIFI::loadParametersDevices()
{
    Device *device = NULL;
    char idHex[10];
    int size = sizeof(deviceIds) / sizeof(DeviceId);

    setParam(Constants::logLevel, (uint8_t) Logger::getLogLevel());
    for (int i = 0; i < size; i++) {
        sprintf(idHex, "x%x", deviceIds[i]);
        device = deviceManager.getDeviceByID(deviceIds[i]);
        if (device != NULL) {
            setParam(idHex, (uint8_t)((device->isEnabled() == true) ? 1 : 0));
        } else {
            setParam(idHex, (uint8_t)0);
		}
    }
}

void ICHIPWIFI::loadParametersDashboard()
{
    MotorController *motorController = deviceManager.getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            sprintf(buffer, "%d,%d", config->torqueMax / -10, config->torqueMax / 10);
            setParam(Constants::torqueRange, buffer);
            sprintf(buffer, "0,%d", config->speedMax);
            setParam(Constants::rpmRange, buffer);
            sprintf(buffer, "%d,%d", config->maxMechanicalPowerRegen / -10, config->maxMechanicalPowerMotor / 10);
            setParam(Constants::powerRange, buffer);
        } else {
            setParam(Constants::torqueRange, "-300,300");
            setParam(Constants::rpmRange, "0,8000");
            setParam(Constants::powerRange, "-250,250");
        }

        //TODO make params configurable
        setParam(Constants::currentRange, "-400,400");
        setParam(Constants::batteryRangeLow, "297,357,368");
        setParam(Constants::batteryRangeHigh, "402,416,428");
        setParam(Constants::motorTempRange, "0,140,170");
        setParam(Constants::controllerTempRange, "0,80,90");
        setParam(Constants::energyRange, "0,30,38");
        setParam(Constants::chargerInputCurrentRange, "0,32");
        setParam(Constants::chargerInputVoltageRange, "210,230,250");
        setParam(Constants::chargerBatteryCurrentRange, "0,20");
        setParam(Constants::chargerTempRange, "0,90,100");
        setParam(Constants::dcDcHvCurrentRange, "0,10");
        setParam(Constants::dcDcLvVoltageRange, "0,20");
        setParam(Constants::dcDcLvCurrentRange, "0,200");
        setParam(Constants::dcDcTempRange, "0, 50, 70");
    }
}

/*
 * Reset to ichip to factory defaults and set-up GEVCU network
 */
void ICHIPWIFI::factoryDefaults() {
    tickHandler.detach(this); // stop other activity
    psReadPtr = psWritePtr = 0; // purge send buffer

    // pinMode(43,OUTPUT);
    //  digitalWrite(43, HIGH);
    //  delay(3000);
    //  digitalWrite(43, LOW);

    delay(1000);
    sendCmd("", IDLE); // just in case something was still on the line
    delay(1000);
    sendCmd("FD", IDLE);
    delay(3000);
    sendCmd("HIF=1", IDLE);//Set for RS-232 serial.
    delay(1000);
    // set-up specific ad-hoc network
    sendCmd("BDRA", IDLE);
    delay(1000);
    sendCmd("WLCH=1", IDLE);//use whichever channel an AP wants to use
    delay(1000);
    sendCmd("WLSI=!GEVCU", IDLE);//set SSID
    delay(1000);
    sendCmd("DIP=192.168.3.10", IDLE);//enable searching for a proper IP via DHCP
    delay(1000);
    sendCmd("DPSZ=10", IDLE);// enable DHCP server for 10 IPs
    delay(1000);
#ifdef CFG_WIFI_WPA2
    sendCmd("WST0=4", IDLE);// enable WPA2-PSK security
    delay(1000);
    sendCmd("WSEC=1", IDLE);// use WPA2-AES protocol
    delay(1000);
    sendCmd("WPP0=verysecret", IDLE);// WPA2 password
    delay(25000); // it really takes that long to calculate the key !!
#endif
    sendCmd("RPG=secret", IDLE);// set the configuration password for /ichip
    delay(1000);
    sendCmd("WPWD=secret", IDLE);// set the password to update config params
    delay(1000);
    sendCmd("AWS=1", IDLE);//turn on web server for three clients
    delay(1000);
    sendCmd("DOWN", IDLE);//cause a reset to allow it to come up with the settings
    delay(5000);// a 5 second delay is required for the chip to come back up ! Otherwise commands will be lost

    tickHandler.attach(this, CFG_TICK_INTERVAL_WIFI);
}

DeviceType ICHIPWIFI::getType()
{
    return DEVICE_WIFI;
}

DeviceId ICHIPWIFI::getId()
{
    return (ICHIP2128);
}

void ICHIPWIFI::loadConfiguration()
{
    WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new WifiConfiguration();
        setConfiguration(config);
    }

    Device::loadConfiguration();
    Logger::info(this, "WiFi configuration:");

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
        //TODO: implement processing of config params for WIFI
//      prefsHandler->read(EESYS_WIFI0_SSID, &config->ssid);
    } else {
        saveConfiguration();
    }
//    Logger::info(this, "ssid: %s", config->);
}

void ICHIPWIFI::saveConfiguration()
{
    WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

//TODO: implement processing of config params for WIFI
//  prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
    prefsHandler->saveChecksum();
}
