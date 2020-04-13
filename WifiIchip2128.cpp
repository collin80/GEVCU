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

#include "WifiIchip2128.h"

/**
 * \brief Constructor
 *
 * Assign serial interface to use for ichip communication
 *
 */
WifiIchip2128::WifiIchip2128() : Wifi()
{
    prefsHandler = new PrefHandler(ICHIP2128);

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
void WifiIchip2128::setup()
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

    // don't try to re-attach if called from reset() - to avoid warning message
    if (!tickHandler.isAttached(this, CFG_TICK_INTERVAL_WIFI)) {
        tickHandler.attach(this, CFG_TICK_INTERVAL_WIFI);
    }

    ready = true;
    running = true;
}

/**
 * \brief Tear down the device in a safe way.
 *
 */
void WifiIchip2128::tearDown()
{
    Device::tearDown();
    digitalWrite(CFG_WIFI_ENABLE, LOW);
}

/**
 * \brief Send a command to ichip
 *
 * A version which defaults to IDLE which is what most of the code used to assume.
 *
 * \param cmd the command string to be sent
 */
void WifiIchip2128::sendCmd(String cmd)
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
void WifiIchip2128::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate)
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
void WifiIchip2128::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate, Socket *socket)
{
    //if the comm is tied up, then buffer this parameter for sending later
    if (state != IDLE) {
        sendBuffer[psWritePtr].cmd = cmd;
        sendBuffer[psWritePtr].state = cmdstate;
        sendBuffer[psWritePtr++].socket = socket;
        if (psWritePtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psWritePtr = 0;
        }
        if (logger.isDebug()) {
            logger.debug(this, "Buffer cmd: %s", cmd.c_str());
        }
    } else { //otherwise, go ahead and blast away
        serialInterface->print(ichipCommandPrefix);
        serialInterface->print(cmd);
        serialInterface->write(13);
        state = cmdstate;
        lastSendSocket = socket;
        lastSendTime = millis();
        if (logger.isDebug()) {
            logger.debug(this, "Send cmd: %s", cmd.c_str());
        }
    }
}

/**
 * \brief Try to send a buffered command to ichip
 */
void WifiIchip2128::sendBufferedCommand()
{
    state = IDLE;
    if (psReadPtr != psWritePtr) {
  		logger.debug(this, "sending buffered command");
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
void WifiIchip2128::sendToSocket(Socket *socket, String data)
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
void WifiIchip2128::requestIncomingSocketData()
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
void WifiIchip2128::requestActiveSocketList()
{
    sprintf(buffer, "LSST:%u", socketListenerHandle);
    sendCmd(buffer, GET_ACTIVE_SOCKETS);
}

/**
 * \brief Start up a listening socket (for WebSockets or OBDII devices)
 *
 */
void WifiIchip2128::startSocketListener()
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
void WifiIchip2128::sendSocketUpdate()
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
void WifiIchip2128::handleTick()
{
    if (!running) { // re-init after a reset
        setup();
        return;
    }
    if (watchdogCounter++ > 250) { // after 250 * 100ms no reception, reset ichip
        logger.warn(this, "watchdog: no response from ichip");
        reset();
        return;
    }

    if (socketListenerHandle != 0) {
        sendSocketUpdate();

        if (tickCounter == 5) {
            requestActiveSocketList();
        } else if (tickCounter % 10 == 0) {
            requestIncomingSocketData();
        } else if (tickCounter == 25 && state == IDLE) { // check if params have changed, but only if idle so we don't mess-up other incoming data
            requestNextParam();
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

 	 if (++tickCounter > 30) {
 		 tickCounter = 0;
 	 }
}

/**
 * \brief Handle a message sent by the DeviceManager
 *
 * \param messageType the type of the message
 * \param message the message to be processed
 */
void WifiIchip2128::handleMessage(uint32_t messageType, void* message)
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
        process();
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
void WifiIchip2128::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);
    sendSocketUpdate();

}

/**
 * \brief Process serial input waiting from the wifi module.
 *
 * The method is called by the main loop
 */
void WifiIchip2128::process()
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
        if (state == GET_SOCKET && remainingSocketRead == -1 && ibWritePtr > 2 && (incoming == ':' || incomingBuffer[2] == '0') &&
                incomingBuffer[0] == 'I' && incomingBuffer[1] == '/' && isdigit(incomingBuffer[2])) {
            processSocketResponseSize();
            return;
        }

        if (incoming == 13 || ibWritePtr > (CFG_WIFI_BUFFER_SIZE - 2) || remainingSocketRead == 0) { // on CR or full buffer, process the line
            incomingBuffer[ibWritePtr] = 0; //null terminate the string
            ibWritePtr = 0; //reset the write pointer

            if (logger.isDebug()) {
                logger.debug(this, "incoming: '%s', state: %d", incomingBuffer, state);
            }

            //The ichip echoes our commands back at us. The safer option might be to verify that the command
            //we think we're about to process is really proper by validating the echo here. But, for now
            //just ignore echoes - except for SSND (socket send, where an error might be added at the end)
            if (strncmp(incomingBuffer, ichipCommandPrefix.c_str(), 4) != 0) {
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
                    //logger.info(this, incomingBuffer);
                    break;
                }
            } else {
                // although we ignore all echo's of "AT+i", the "AT+iSSND" has the I/OK or I/ERROR in-line
                if (strstr(incomingBuffer, "AT+iSSND") != NULL) {
                    processSocketSendResponse();
                }
            }
            // if we got an I/OK or I/ERROR in the reply then the command is done sending data. So, see if there is a buffered cmd to send.
            if (strstr(incomingBuffer, "I/OK") != NULL) {
                watchdogCounter = 0; // the ichip responds --> it's alive
                sendBufferedCommand();
            }
            if (strstr(incomingBuffer, ichipErrorString.c_str()) != NULL) {
                logger.warn("ichip responded with error: '%s', state %d", incomingBuffer, state);
                sendBufferedCommand();
            }
            return; // before processing the next line, return to the process() to allow other devices to process.
        } else { // add more characters
            if (incoming != 10) { // don't add a LF character
                incomingBuffer[ibWritePtr++] = (char) incoming;
            }
        }
    }
    if (millis() > lastSendTime + 1000) {
        logger.debug(this, "response timeout");
        lastSendTime = millis();
        sendBufferedCommand(); //something went wrong so reset state
    }
}

/**
 * \brief Handle the response of a request to start the socket listener
 *
 */
void WifiIchip2128::processStartSocketListenerRepsonse()
{
    //reply hopefully has the listening socket handle
    if (strcmp(incomingBuffer, ichipErrorString.c_str())) {
        socketListenerHandle = atoi(&incomingBuffer[2]);
        if (socketListenerHandle < 10 || socketListenerHandle > 11) {
            socketListenerHandle = 0;
        }
        if (logger.isDebug()) {
            logger.debug(this, "socket listener handle: %i", socketListenerHandle);
        }
    }
    sendBufferedCommand();
}

/**
 * \brief Handle the response of a request to get the list of active sockets
 *
 */
void WifiIchip2128::processActiveSocketListResponse()
{
    if (strcmp(incomingBuffer, ichipErrorString.c_str())) {
        if (strncmp(incomingBuffer, "I/(", 3) == 0) {
            socket[0].handle = atoi(strtok(&incomingBuffer[3], ","));
            for (int i = 1; i < CFG_WIFI_NUM_SOCKETS; i++) {
                socket[i].handle = atoi(strtok(NULL, ","));
            }
        }
    } else {
        logger.warn(this, "could not retrieve list of active sockets, closing all open sockets");
        closeAllSockets();
    }
    // as the reply only contains "I/(000,-1,-1,-1)" it won't be recognized in process() as "I/OK" or "I/ERROR",
    // to proceed with processing the buffer, we need to call it here.
    sendBufferedCommand();
}

/**
 * \brief Extract the size of the socket response data
 *
 */
void WifiIchip2128::processSocketResponseSize()
{
    incomingBuffer[ibWritePtr] = 0; //null terminate the string
    ibWritePtr = 0; //reset the write pointer
    remainingSocketRead = constrain(atoi(&incomingBuffer[2]), 0, 1024);
    if(remainingSocketRead == 0) {
        remainingSocketRead--;
        sendBufferedCommand();
    } else {
        logger.debug(this, "processing remaining socket read: %d", remainingSocketRead);
    }
}

/**
 * \brief Process incoming data from a socket
 *
 * The data is forwarded to the socket's assigned SocketProcessor. The processor prepares a
 * response which is sent back to the client via the socket.
 *
 */
void WifiIchip2128::processIncomingSocketData()
{
    logger.debug(this, "processing incoming socket data");

    if (strstr(incomingBuffer, ichipErrorString.c_str()) != NULL) {
        logger.warn(this, "could not retrieve data from socket %d, closing socket", lastSendSocket->handle);
        closeSocket(lastSendSocket);
    }

    if (lastSendSocket != NULL) {
        if (lastSendSocket->processor == NULL) {
            if (strstr(incomingBuffer, "HTTP") != NULL) {
                logger.info("connecting to web-socket client");
                lastSendSocket->processor = new WebSocket();
            } else {
                logger.info("connecting to ELM327 device");
                lastSendSocket->processor = new ELM327Processor();
            }
        }

        String data = lastSendSocket->processor->processInput(incomingBuffer);
        if (data.length() > 0) {
            if (data.equals(disconnect)) { // do we have to disconnect ?
                closeSocket(lastSendSocket);
            } else {
                sendToSocket(lastSendSocket, data);
            }
        }
    }

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
void WifiIchip2128::processSocketSendResponse()
{
    if (strstr(incomingBuffer, ichipErrorString.c_str()) != NULL &&
            strstr(incomingBuffer, "(203)") != NULL) {
        logger.info(this, "connection closed by remote client (%s), closing socket", incomingBuffer);
        closeSocket(lastSendSocket);
    }
}

/**
 * \brief Close all open socket
 *
 */
void WifiIchip2128::closeAllSockets()
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
void WifiIchip2128::closeSocket(Socket *socket)
{
    if (socket->handle != -1) {
    	logger.debug(this, "closing socket %i", socket->handle);
        sprintf(buffer, "!SCLS:%i", socket->handle);
        sendCmd(buffer, SEND_SOCKET, socket);
    }
    socket->handle = -1;
    socket->processor = NULL;
}

/**
 * \brief Determine if a parameter has been changed
 *
 * The result will be processed in process() -> processParameterChange()
 *
 */
void WifiIchip2128::requestNextParam()
{
    sendCmd("WNXT", GET_PARAM);  //send command to get next changed parameter
}

/**
 * \brief Retrieve the value of a parameter
 *
 * \param paramName the name of the parameter
 */
void WifiIchip2128::requestParamValue(String paramName)
{
    sendCmd(paramName + "?", GET_PARAM);
}

/**
 * \brief Set a parameter to the given string value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void WifiIchip2128::setParam(String paramName, String value)
{
    sendCmd(paramName + "=\"" + value + "\"", SET_PARAM);
}

/**
 * \brief Reset the ichip to overcome a crash
 *
 */
void WifiIchip2128::reset()
{
    logger.info(this, "resetting the ichip");

    // cycle reset pin (next tick() will activate it again
    digitalWrite(CFG_WIFI_RESET, LOW);
    running = false;
    ready = false;
}

/**
 * \brief Reset to ichip to factory defaults and set-up GEVCU network
 *
 */
void WifiIchip2128::factoryDefaults() {
    logger.info(this, "resetting to factory defaults");
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
    sendCmd("RPG=*", IDLE);// allow everybody to update the params - no PW-Authentication
    delay(1000);
    sendCmd("WPWD=*", IDLE);// no password required to update params
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
DeviceType WifiIchip2128::getType()
{
    return DEVICE_WIFI;
}

/**
 * \brief Get the device ID
 *
 * \return the ID of the device
 */
DeviceId WifiIchip2128::getId()
{
    return (ICHIP2128);
}
