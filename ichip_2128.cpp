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

/**
 * \brief Constructor
 *
 * Assign serial interface to use for ichip communication
 *
 */
ICHIPWIFI::ICHIPWIFI() : Device()
{
    prefsHandler = new PrefHandler(ICHIP2128);

    if (systemIO.getSystemType() == GEVCU3 || systemIO.getSystemType() == GEVCU4) {
        serialInterface = &Serial2;
    } else { //older hardware used this instead
        serialInterface = &Serial3;
    }
    serialInterface->begin(115200);

    commonName = "WIFI (iChip2128)";

    for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
        socket[i].handle = -1;
        socket[i].processor = NULL;
    }

    didParamLoad = didTCPListener = false;
    tickCounter = watchdogCounter = 0;
    ibWritePtr = psWritePtr = psReadPtr = 0;
    socketListenerHandle = 0;
    lastSendTime = timeStarted = 0;
    lastSendSocket = NULL;
    state = IDLE;
    remainingSocketRead = -1;

    pinMode(CFG_WIFI_RESET, OUTPUT);
    pinMode(CFG_WIFI_ENABLE, OUTPUT);
}

/**
 * \brief Initialization of hardware and parameters
 *
 */
void ICHIPWIFI::setup()
{
    digitalWrite(CFG_WIFI_RESET, HIGH);
    digitalWrite(CFG_WIFI_ENABLE, HIGH);

    for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
        socket[i].handle = -1;
        socket[i].processor = NULL;
    }

    didParamLoad = didTCPListener = false;
    tickCounter = watchdogCounter = 0;
    ibWritePtr = psWritePtr = psReadPtr = 0;
    socketListenerHandle = 0;
    lastSendSocket = NULL;
    remainingSocketRead = -1;
    lastSendTime = timeStarted = millis();
    state = IDLE;
    ready = true;
    running = true;

    tickHandler.attach(this, CFG_TICK_INTERVAL_WIFI);
}

/**
 * \brief Tear down the device in a safe way.
 *
 */
void ICHIPWIFI::tearDown()
{
    Device::tearDown();
    digitalWrite(42, LOW);
}

/**
 * \brief Send a command to ichip
 *
 * A version which defaults to IDLE which is what most of the code used to assume.
 *
 * \param cmd the command string to be sent
 */
void ICHIPWIFI::sendCmd(String cmd)
{
    sendCmd(cmd, IDLE);
}

/**
 * \brief Send a command to ichip.
 *
 * A version used if no socket information is required
 *
 * \param cmd the command to send (without the preceeding AT+i)
 * \param cmdstate the command state the ichip is after sending this command
 */
void ICHIPWIFI::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate)
{
    sendCmd(cmd, cmdstate, NULL);
}

/**
 * \brief Send a command to ichip.
 *
 * The "AT+i" part will be added. If the comm channel is busy it buffers the command
 *
 * \param cmd the command to send (without the preceeding AT+i)
 * \param cmdstate the command state the ichip is after sending this command
 * \param socket the socket to which the command is related
 */
void ICHIPWIFI::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate, Socket *socket)
{
    //if the comm is tied up, then buffer this parameter for sending later
    if (state != IDLE) {
        sendBuffer[psWritePtr].cmd = cmd;
        sendBuffer[psWritePtr].state = cmdstate;
        sendBuffer[psWritePtr++].socket = socket;
        if (psWritePtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psWritePtr = 0;
        }
        if (Logger::isDebug()) {
            Logger::debug(this, "Buffer cmd: %s", cmd.c_str());
        }
    } else { //otherwise, go ahead and blast away
        serialInterface->write(Constants::ichipCommandPrefix);
        serialInterface->print(cmd);
        serialInterface->write(13);
        state = cmdstate;
        lastSendSocket = socket;
        lastSendTime = millis();
        if (Logger::isDebug()) {
            Logger::debug(this, "Send cmd: %s", cmd.c_str());
        }
    }
}

/**
 * \brief Try to send a buffered command to ichip
 */
void ICHIPWIFI::sendBufferedCommand()
{
    state = IDLE;
    if (psReadPtr != psWritePtr) {
  		Logger::debug(this, "sending buffered command");
        //if there is a parameter in the buffer to send then do it
        sendCmd(sendBuffer[psReadPtr].cmd, sendBuffer[psReadPtr].state, sendBuffer[psReadPtr++].socket);
        if (psReadPtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psReadPtr = 0;
        }
    }
}

/**
 * \brief Send data to a specific socket
 *
 * \param socket to send data to
 * \param data to send to client via socket
 */
void ICHIPWIFI::sendToSocket(Socket *socket, String data)
{
    if (data == NULL || data.length() < 1 || socket->handle == -1) {
        return;
    }
    sprintf(buffer, "SSND%%:%i,%i:", socket->handle, data.length());
    String temp = String(buffer);
    temp.concat(data);
    sendCmd(temp, SEND_SOCKET, socket);
}

/**
 * \brief Request the delivery of waiting data from active sockets
 *
 * Only request if we're not already in midst of it
 *
 */
void ICHIPWIFI::requestIncomingSocketData()
{
    if (state != GET_SOCKET) {
        for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
            if (socket[i].handle != -1) {
                sprintf(buffer, "SRCV:%i,1024", socket[i].handle);
                sendCmd(buffer, GET_SOCKET, &socket[i]);
            }
        }
    }
}

/**
 * \brief Request list of open handles for socket
 *
 */
void ICHIPWIFI::requestActiveSocketList()
{
    sprintf(buffer, "LSST:%u", socketListenerHandle);
    sendCmd(buffer, GET_ACTIVE_SOCKETS);
}

/**
 * \brief Start up a listening socket (for WebSockets or OBDII devices)
 *
 */
void ICHIPWIFI::startSocketListener()
{
    sprintf(buffer, "LTCP:2000,%u", CFG_WIFI_NUM_SOCKETS);
    sendCmd(buffer, START_TCP_LISTENER);
}

/**
 * \brief Send update to all active sockets
 *
 * The message to be sent is created by the assigned SocketProcessor
 *
 */
void ICHIPWIFI::sendSocketUpdate()
{
    for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
        if (socket[i].handle != -1 && socket[i].processor != NULL) {
            String data = socket[i].processor->generateUpdate();
            sendToSocket(&socket[i], data);
        }
    }
}

/**
 * \brief Perform regular tasks triggered by a timer
 *
 * Query for changed parameters of the config page, socket status
 * and query for incoming data on sockets
 *
 */
void ICHIPWIFI::handleTick()
{
    if (watchdogCounter++ > 50) { // after 50 * 200ms no reception, reset ichip
        Logger::warn(this, "watchdog: no response from ichip");
        reset();
    }

    if (socketListenerHandle != 0) {
        sendSocketUpdate();

        if (tickCounter++ == 10) {
            requestActiveSocketList();
        }

        if (tickCounter == 20) {
            requestIncomingSocketData();
        }

        // check if params have changed, but only if idle so we don't mess-up other incoming data
        if (tickCounter >  29 && state == IDLE) {
            requestNextParam();
            tickCounter = 0;
        }
    } else { // wait a bit for things to settle before doing a thing
        if (!didParamLoad && millis() > 3000 + timeStarted) {
            loadParameters();
            didParamLoad = true;
        }
        if (!didTCPListener && millis() > 5000 + timeStarted) {
            startSocketListener();
            didTCPListener = true;
        }
    }
}

/**
 * \brief Handle a message sent by the DeviceManager
 *
 * \param messageType the type of the message
 * \param message the message to be processed
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
        char **params = (char **) message;
        for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
            if (socket[i].handle != -1 && socket[i].processor != NULL) {
                String data = socket[i].processor->generateLogEntry(params[0], params[1], params[2]);
                sendToSocket(&socket[i], data);
            }
        }
        break;
    }
}

/**
 * \brief Act on system state changes and send an update to the socket client
 *
 * \param oldState the previous system state
 * \param newState the new system state
 */
void ICHIPWIFI::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);
    sendSocketUpdate();

}

/**
 * \brief Process serial input waiting from the wifi module.
 *
 * The method is called by the main loop
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
            if (state == GET_SOCKET) { // just add the char and ignore nothing (not even CR/LF or 0)
                incomingBuffer[ibWritePtr++] = (char) incoming;
                if (remainingSocketRead > 0 && ibWritePtr < CFG_WIFI_BUFFER_SIZE) {
                    continue;
                }
            }
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
                    processStartSocketListenerRepsonse();
                    break;
                case GET_ACTIVE_SOCKETS: //reply from asking for active connections
                    processActiveSocketListResponse();
                    break;
                case POLL_SOCKET: //reply from asking about state of socket and how much data it has
                    break;
                case SEND_SOCKET: //reply from sending data over a socket
                    break;
                case GET_SOCKET: //reply requesting the data pending on a socket
                    processIncomingSocketData();
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
                watchdogCounter = 0; // the ichip responds --> it's alive
                if(strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
                    Logger::console("ichip responded with error: '%s', state %d", incomingBuffer, state);
                }
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

/**
 * \brief Handle the response of a request to start the socket listener
 *
 */
void ICHIPWIFI::processStartSocketListenerRepsonse()
{
    //reply hopefully has the listening socket handle
    if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
        socketListenerHandle = atoi(&incomingBuffer[2]);
        if (socketListenerHandle < 10 || socketListenerHandle > 11) {
            socketListenerHandle = 0;
        }
        if (Logger::isDebug()) {
            Logger::debug(this, "socket listener handle: %i", socketListenerHandle);
        }
    }
    sendBufferedCommand();
}

/**
 * \brief Handle the response of a request to get the list of active sockets
 *
 */
void ICHIPWIFI::processActiveSocketListResponse()
{
    if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
        if (strncmp(incomingBuffer, "I/(", 3) == 0) {
            socket[0].handle = atoi(strtok(&incomingBuffer[3], ","));
            for (int i = 1; i < CFG_WIFI_NUM_SOCKETS; i++) {
                socket[i].handle = atoi(strtok(NULL, ","));
            }
        }
    } else {
        Logger::error(this, "could not retrieve list of active sockets, closing all open sockets");
        closeAllSockets();
    }
    // as the reply only contains "I/(000,-1,-1,-1)" it won't be recognized in loop() as "I/OK" or "I/ERROR",
    // to proceed with processing the buffer, we need to call it here.
    sendBufferedCommand();
}

/**
 * \brief Extract the size of the socket response data
 *
 */
void ICHIPWIFI::processSocketResponseSize()
{
    incomingBuffer[ibWritePtr] = 0; //null terminate the string
    ibWritePtr = 0; //reset the write pointer
    remainingSocketRead = atoi(&incomingBuffer[2]) - 1;
    Logger::debug(this, "processing remaining socket read: %d", remainingSocketRead);
}

/**
 * \brief Process incoming data from a socket
 *
 * The data is forwarded to the socket's assigned SocketProcessor. The processor prepares a
 * response which is sent back to the client via the socket.
 *
 */
void ICHIPWIFI::processIncomingSocketData()
{
    if (strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
        Logger::error(this, "could not retrieve data from socket %d, closing socket", lastSendSocket->handle);
        closeSocket(lastSendSocket);
    }
    if (strcmp(incomingBuffer, "I/0") == 0) { // nothing to read, move on to next command
        remainingSocketRead = -1;
        sendBufferedCommand();
        return;
    }

    if (lastSendSocket != NULL) {
        if (lastSendSocket->processor == NULL) {
            if (strstr(incomingBuffer, "HTTP") != NULL) {
                Logger::info("connecting to web-socket client");
                lastSendSocket->processor = new WebSocket();
            } else {
                Logger::info("connecting to ELM327 device");
                lastSendSocket->processor = new ELM327Processor();
            }
        }

        String data = lastSendSocket->processor->processInput(incomingBuffer);
        if (data != NULL) {
            if (data.equals(Constants::disconnect)) { // do we have to disconnect ?
                closeSocket(lastSendSocket);
            } else {
                sendToSocket(lastSendSocket, data);
            }
        }
    }

    // empty line marks end of transmission -> switch back to IDLE to be able to send (buffered) messages
    // beware that WebSocket relies on empty lines to indicate the end of transmission
    if (remainingSocketRead == 0) {
        remainingSocketRead = -1;
        sendBufferedCommand();
    }
}

/**
 * \brief Handle the response of a socket send command
 *
 * An eventual error is not sent in a separate line but in the same as the echo
 * of the SSND. It can happen e.g. when the socket was closed by the client
 * (e.g. "AT+iSSND%:000,24:I/ERROR (203)")
 *
 */
void ICHIPWIFI::processSocketSendResponse()
{
    if (strstr(incomingBuffer, Constants::ichipErrorString) != NULL) {
        Logger::info(this, "connection closed by remote client (%s), closing socket", incomingBuffer);
        closeSocket(lastSendSocket);
    }
}

/**
 * \brief Close all open socket
 *
 */
void ICHIPWIFI::closeAllSockets()
{
    for (int c = 0; c < CFG_WIFI_NUM_SOCKETS; c++) {
        closeSocket(&socket[c]);
    }
}

/**
 * \brief Close a specific socket
 *
 * \param socket to close
 */
void ICHIPWIFI::closeSocket(Socket *socket)
{
    if (socket->handle != -1) {
    	Logger::debug(this, "closing socket %i", socket->handle);
        sprintf(buffer, "!SCLS:%i", socket->handle);
        sendCmd(buffer, SEND_SOCKET, socket);
    }
    socket->handle = -1;
    socket->processor = NULL;
}

/**
 * \brief Determine if a parameter has been changed
 *
 * The result will be processed in loop() -> processParameterChange()
 *
 */
void ICHIPWIFI::requestNextParam()
{
    sendCmd("WNXT", GET_PARAM);  //send command to get next changed parameter
}

/**
 * \brief Retrieve the value of a parameter
 *
 * \param paramName the name of the parameter
 */
void ICHIPWIFI::requestParamValue(String paramName)
{
    sendCmd(paramName + "?", GET_PARAM);
}

/**
 * \brief Set a parameter to the given string value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, String value)
{
    sendCmd(paramName + "=\"" + value + "\"", SET_PARAM);
}

/**
 * \brief Set a parameter to the given int32 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, int32_t value)
{
    sprintf(buffer, "%ld", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given uint32 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, uint32_t value)
{
    sprintf(buffer, "%lu", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given int16 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, int16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given uint16 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, uint16_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given int8 value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void ICHIPWIFI::setParam(String paramName, uint8_t value)
{
    sprintf(buffer, "%d", value);
    setParam(paramName, buffer);
}

/**
 * \brief Set a parameter to the given float value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 * \param precision the number of digits after the decimal sign
 */
void ICHIPWIFI::setParam(String paramName, float value, int precision)
{
    char format[10];
    sprintf(format, "%%.%df", precision);
    sprintf(buffer, format, value);
    setParam(paramName, buffer);
}

/**
 * \brief Process the parameter update from ichip we received as a response to AT+iWNXT.
 *
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 *
 * \param key and value pair of changed parameter
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
        requestNextParam(); // try to get another one immediately
    }
}

/**
 * \brief Process a throttle parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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
            } else {
                return false;
            }
            throttle->saveConfiguration();
            return true;
        }
    }
    return false;
}

/**
 * \brief Process a brake parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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

/**
 * \brief Process a motor parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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
            } else if (!strcmp(key, Constants::creepLevel)) {
                config->creepLevel = atol(value);
            } else if (!strcmp(key, Constants::creepSpeed)) {
                config->creepSpeed = atol(value);
            } else if (!strcmp(key, Constants::brakeHold)) {
                config->brakeHold = atol(value);
            } else if (!strcmp(key, Constants::brakeHoldLevel)) {
                config->brakeHold = atol(value);
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

/**
 * \brief Process a charger parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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

/**
 * \brief Process a DCDC converter parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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

/**
 * \brief Process a system I/O parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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
    } else if (!strcmp(key, Constants::absInput)) {
        config->absInput = atol(value);
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

/**
 * \brief Process a device specific parameter change
 *
 * \param key the name of the parameter
 * \param value the value of the parameter
 * \return true if the parameter was processed
 */
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
 * \brief Get parameters from devices and forward them to ichip.
 *
 * This is required to initially set-up the ichip
 *
 */
void ICHIPWIFI::loadParameters()
{
    Logger::info(this, "loading config params to wifi...");

    loadParametersThrottle();
    loadParametersBrake();
    loadParametersMotor();
    loadParametersCharger();
    loadParametersDcDc();
    loadParametersSystemIO();
    loadParametersDevices();
    loadParametersDashboard();

    Logger::info(this, "wifi parameters loaded");
}

/**
 * \brief Load throttle parameters to the ichip
 *
 */
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
        }
    }
}

/**
 * \brief Load brake parameters to the ichip
 *
 */
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
    } else {
        setParam(Constants::brakeMinimumLevel, (uint8_t)0);
        setParam(Constants::brakeMaximumLevel, (uint8_t)100);
        setParam(Constants::brakeMinimumRegen, (uint8_t)0);
        setParam(Constants::brakeMaximumRegen, (uint8_t)0);
    }
}

/**
 * \brief Load motor parameters to the ichip
 *
 */
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
            setParam(Constants::creepLevel, config->creepLevel);
            setParam(Constants::creepSpeed, config->creepSpeed);
            setParam(Constants::brakeHold, config->brakeHold);
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

/**
 * \brief Load charger parameters to the ichip
 *
 */
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

/**
 * \brief Load DCDC converter parameters to the ichip
 *
 */
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

/**
 * \brief Load system I/O parameters to the ichip
 *
 */
void ICHIPWIFI::loadParametersSystemIO()
{
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO.getConfiguration();

    if (config) {
        setParam(Constants::enableInput, config->enableInput);
        setParam(Constants::chargePowerAvailableInput, config->chargePowerAvailableInput);
        setParam(Constants::interlockInput, config->interlockInput);
        setParam(Constants::reverseInput, config->reverseInput);
        setParam(Constants::absInput, config->absInput);

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

/**
 * \brief Load device specific parameters to the ichip
 *
 */
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

/**
 * \brief Load dashboard related parameters to the ichip
 *
 */
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
        setParam(Constants::currentRange, "-200,200");
        setParam(Constants::batteryRangeLow, "297,357,368");
        setParam(Constants::batteryRangeHigh, "402,416,428");
        setParam(Constants::motorTempRange, "0,90,120");
        setParam(Constants::controllerTempRange, "0,60,80");
        setParam(Constants::energyRange, "0,30,38");
    }
}

/**
 * \brief Reset the ichip to overcome a crash
 *
 */
void ICHIPWIFI::reset()
{
    tickHandler.detach(this); // stop other activity
    Logger::info(this, "resetting the ichip");

    // cycle reset pin
    digitalWrite(CFG_WIFI_RESET, LOW);
    delay(2); // according to specs 1ms should be sufficient
    digitalWrite(CFG_WIFI_RESET, HIGH);

    setup();
}


/**
 * \brief Reset to ichip to factory defaults and set-up GEVCU network
 *
 */
void ICHIPWIFI::factoryDefaults() {
    Logger::info(this, "resetting to factory defaults");
    tickHandler.detach(this); // stop other activity
    psReadPtr = psWritePtr = 0; // purge send buffer

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
    sendCmd("AWS=3", IDLE);//turn on web server for 3 concurrent connections
    delay(1000);
    sendCmd("DOWN", IDLE);//cause a reset to allow it to come up with the settings
    delay(5000);// a 5 second delay is required for the chip to come back up ! Otherwise commands will be lost

    tickHandler.attach(this, CFG_TICK_INTERVAL_WIFI);
}

/**
 * \brief Get the device type
 *
 * \return the type of the device
 */
DeviceType ICHIPWIFI::getType()
{
    return DEVICE_WIFI;
}

/**
 * \brief Get the device ID
 *
 * \return the ID of the device
 */
DeviceId ICHIPWIFI::getId()
{
    return (ICHIP2128);
}

/**
 * \brief Load the device configuration from EEPROM
 *
 */
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

/**
 * \brief Save the device configuration to the EEPROM
 *
 */
void ICHIPWIFI::saveConfiguration()
{
    WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

//TODO: implement processing of config params for WIFI
//  prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
    prefsHandler->saveChecksum();
}
