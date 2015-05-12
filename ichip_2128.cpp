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
ICHIPWIFI::ICHIPWIFI()
{
    prefsHandler = new PrefHandler(ICHIP2128);

    uint8_t sys_type;
    sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);

    if (sys_type == 3 || sys_type == 4) {
        serialInterface = &Serial2;
    } else { //older hardware used this instead
        serialInterface = &Serial3;
    }

    commonName = "WIFI (iChip2128)";
}

/*
 * Constructor. Pass serial interface to use for ichip communication
 */
ICHIPWIFI::ICHIPWIFI(USARTClass *which)
{
    prefsHandler = new PrefHandler(ICHIP2128);
    serialInterface = which;
}

/*
 * Initialization of hardware and parameters
 */
void ICHIPWIFI::setup()
{
    tickHandler->detach(this);

    tickCounter = 0;
    ibWritePtr = 0;
    psWritePtr = 0;
    psReadPtr = 0;
    listeningSocket = 0;

    lastSendTime = millis();

    activeSockets[0] = -1;
    activeSockets[1] = -1;
    activeSockets[2] = -1;
    activeSockets[3] = -1;

    state = IDLE;

    didParamLoad = false;
    didTCPListener = false;

    serialInterface->begin(115200);

    paramCache.timeRunning = 0;
    paramCache.brakeNotAvailable = true;

    elmProc = new ELM327Processor();
    ready = true;
    running = true;

    tickHandler->attach(this, CFG_TICK_INTERVAL_WIFI);
}

/**
 * Tear down the device in a safe way.
 */
void ICHIPWIFI::tearDown()
{
    Device::tearDown();

    //TODO: if there is a way to physically power off the device, do it here. but also power it on during setup()
}


//A version of sendCmd that defaults to SET_PARAM which is what most of the code used to assume.
void ICHIPWIFI::sendCmd(String cmd)
{
    sendCmd(cmd, SET_PARAM);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 * If the comm channel is busy it buffers the command
 */
void ICHIPWIFI::sendCmd(String cmd, ICHIP_COMM_STATE cmdstate)
{
    if (state != IDLE) { //if the comm is tied up then buffer this parameter for sending later
        sendBuffer[psWritePtr].cmd = cmd;
        sendBuffer[psWritePtr++].state = cmdstate;
        if (psWritePtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
            psWritePtr = 0;
        }

        if (Logger::isDebug()) {
            Logger::debug(ICHIP2128, "Buffer cmd: %s", cmd.c_str());
        }
    } else { //otherwise, go ahead and blast away
        serialInterface->write(Constants::ichipCommandPrefix);
        serialInterface->print(cmd);
        serialInterface->write(13);
        state = cmdstate;
        lastSendTime = millis();

        if (Logger::isDebug()) {
            Logger::debug(ICHIP2128, "Send to ichip cmd: %s", cmd.c_str());
        }
    }
}

void ICHIPWIFI::sendToSocket(int socket, String data)
{
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
void ICHIPWIFI::handleTick()
{
    DeviceManager *deviceManager = DeviceManager::getInstance();
    MotorController* motorController = deviceManager->getMotorController();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();
    static int pollListening = 0;
    uint32_t ms = millis();
    tickCounter++;

    if (ms < 1000) {
        return;    //wait 1 seconds for things to settle before doing a thing
    }

    // Do a delayed parameter load once about a second after startup
    if (!didParamLoad) {
        loadParameters();
        didParamLoad = true;
    }

    //At 12 seconds start up a listening socket for OBDII
    if (!didTCPListener && ms > 12000) {
        sendCmd("LTCP:2000,4", START_TCP_LISTENER);
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
    for (int c = 0; c < 4; c++)
        if (activeSockets[c] != -1) {
            char buff[6];
            sprintf(buff, "%03i", activeSockets[c]);
            String temp = "SRCV:" + String(buff) + ",80";
            sendCmd(temp, GET_SOCKET);
        }

    // make small slices so the main loop is not blocked for too long
    if (tickCounter == 1) {
        if (motorController) {
            // just update this every second or so
            if (ms > paramCache.timeRunning + 1000) {
                paramCache.timeRunning = ms;
                setParam(Constants::timeRunning, getTimeRunning());
            }

            if (paramCache.torqueRequested != motorController->getTorqueRequested()) {
                paramCache.torqueRequested = motorController->getTorqueRequested();
                setParam(Constants::torqueRequested, paramCache.torqueRequested / 10.0f, 1);
            }

            if (paramCache.torqueActual != motorController->getTorqueActual()) {
                paramCache.torqueActual = motorController->getTorqueActual();
                setParam(Constants::torqueActual, paramCache.torqueActual / 10.0f, 1);
            }
        }

        if (accelerator) {
            if (paramCache.throttle != accelerator->getLevel()) {
                paramCache.throttle = accelerator->getLevel();
                setParam(Constants::throttle, paramCache.throttle / 10.0f, 1);
            }
        }

        if (brake) {
            if (paramCache.brake != brake->getLevel()) {
                paramCache.brake = brake->getLevel();
                paramCache.brakeNotAvailable = false;
                setParam(Constants::brake, paramCache.brake / 10.0f, 1);
            }
        } else {
            if (paramCache.brakeNotAvailable == true) {
                paramCache.brakeNotAvailable = false; // no need to keep sending this
                setParam(Constants::brake, Constants::notAvailable);
            }
        }
    } else if (tickCounter == 2) {
        if (motorController) {
            if (paramCache.speedRequested != motorController->getSpeedRequested()) {
                paramCache.speedRequested = motorController->getSpeedRequested();
                setParam(Constants::speedRequested, paramCache.speedRequested);
            }

            if (paramCache.speedActual != motorController->getSpeedActual()) {
                paramCache.speedActual = motorController->getSpeedActual();
                setParam(Constants::speedActual, paramCache.speedActual);
            }

            if (paramCache.dcVoltage != motorController->getDcVoltage()) {
                paramCache.dcVoltage = motorController->getDcVoltage();
                setParam(Constants::dcVoltage, paramCache.dcVoltage / 10.0f, 1);
            }

            if (paramCache.dcCurrent != motorController->getDcCurrent()) {
                paramCache.dcCurrent = motorController->getDcCurrent();
                setParam(Constants::dcCurrent, paramCache.dcCurrent / 10.0f, 1);
            }

            if (paramCache.kiloWattHours != motorController->getKiloWattHours() / 3600000 ) {
                paramCache.kiloWattHours = motorController->getKiloWattHours() / 3600000;
                setParam(Constants::kiloWattHours, paramCache.kiloWattHours / 10.0f, 1);
            }

        }
    } else if (tickCounter == 3) {
        if (paramCache.bitfield1 != status->getBitField1()) {
            paramCache.bitfield1 = status->getBitField1();
            setParam(Constants::bitfield1, paramCache.bitfield1);
        }

        if (paramCache.bitfield2 != status->getBitField2()) {
            paramCache.bitfield2 = status->getBitField2();
            setParam(Constants::bitfield2, paramCache.bitfield2);
        }

        if (paramCache.bitfield3 != status->getBitField3()) {
            paramCache.bitfield3 = status->getBitField3();
            setParam(Constants::bitfield3, paramCache.bitfield3);
        }

        if (paramCache.systemState != status->getSystemState()) {
            paramCache.systemState = status->getSystemState();
            setParam(Constants::systemState, status->systemStateToStr(status->getSystemState()));
        }
    } else if (tickCounter > 3) {
        if (motorController) {
            if (paramCache.tempMotor != motorController->getTemperatureMotor()) {
                paramCache.tempMotor = motorController->getTemperatureMotor();
                setParam(Constants::tempMotor, paramCache.tempMotor / 10.0f, 1);
            }

            if (paramCache.tempController != motorController->getTemperatureController()) {
                paramCache.tempController = motorController->getTemperatureController();
                setParam(Constants::tempController, paramCache.tempController / 10.0f, 1);
            }

            if (paramCache.tempSystem != motorController->getTemperatureController()) {
                paramCache.tempSystem = motorController->getTemperatureController();
                setParam(Constants::tempSystem, paramCache.tempSystem / 10.0f, 1);
            }

            if (paramCache.gear != motorController->getSelectedGear()) {
                paramCache.gear = motorController->getSelectedGear();
                setParam(Constants::gear, (uint16_t) paramCache.gear);
            }

            if ( paramCache.mechPower != motorController->getMechanicalPower() ) {
                paramCache.mechPower = motorController->getMechanicalPower();
                setParam(Constants::mechPower, paramCache.mechPower / 10.0f, 1);
            }
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
char *ICHIPWIFI::getTimeRunning()
{
    uint32_t ms = millis();
    int seconds = (int)(ms / 1000) % 60;
    int minutes = (int)((ms / (1000 * 60)) % 60);
    int hours = (int)((ms / (1000 * 3600)) % 24);
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    return buffer;
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
            break;
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

        if (incoming != -1) { //and there is no reason it should be -1
            if (incoming == 13 || ibWritePtr > 126) { // on CR or full buffer, process the line
                incomingBuffer[ibWritePtr] = 0; //null terminate the string
                ibWritePtr = 0; //reset the write pointer

                //what we do with the input depends on what state the ICHIP comm was set to.
                if (Logger::isDebug()) {
                    Logger::debug(ICHIP2128, "In Data, state: %i", state);
                    Logger::debug(ICHIP2128, "Data from ichip: %s", incomingBuffer);
                }

                //The ichip echoes our commands back at us. The safer option might be to verify that the command
                //we think we're about to process is really proper by validating the echo here. But, for now
                //just ignore echoes.
                if (strncmp(incomingBuffer, Constants::ichipCommandPrefix, 4) != 0) {
                    switch (state) {
                        case GET_PARAM: //reply from an attempt to read changed parameters from ichip
                            if (strchr(incomingBuffer, '=')) {
                                processParameterChange(incomingBuffer);
                            }
                            break;

                        case SET_PARAM: //reply from sending parameters to the ichip
                            break;

                        case START_TCP_LISTENER: //reply from turning on a listening socket

                            //reply hopefully has the listening socket #.
                            if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
                                listeningSocket = atoi(&incomingBuffer[2]);
                                if (listeningSocket < 10 || listeningSocket > 11) {
                                    listeningSocket = 0;
                                }
                                if (Logger::isDebug()) {
                                    Logger::debug(ICHIP2128, "%i", listeningSocket);
                                }
                            }
                            break;

                        case GET_ACTIVE_SOCKETS: //reply from asking for active connections
                            if (strcmp(incomingBuffer, Constants::ichipErrorString)) {
                                activeSockets[0] = atoi(strtok(&incomingBuffer[3], ","));
                                activeSockets[1] = atoi(strtok(NULL, ","));
                                activeSockets[2] = atoi(strtok(NULL, ","));
                                activeSockets[3] = atoi(strtok(NULL, ","));

                                if (Logger::isDebug()) {
                                    Logger::debug(ICHIP2128, "%i, %i, %i, %i", activeSockets[0], activeSockets[1], activeSockets[2], activeSockets[3]);
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
                                    dLen = atoi(strtok(&incomingBuffer[2], ":"));
                                    String datastr = strtok(0, ":");  //get the rest of the string
                                    datastr.toLowerCase();
                                    String ret = elmProc->processELMCmd((char *) datastr.c_str());
                                    sendToSocket(0, ret);  //TODO: need to actually track which socket requested this data
                                }
                            }
                            break;

                        case IDLE: //not sure whether to leave this info or go to debug. The ichip shouldn't be sending things in idle state
                        default:
                            //Logger::info(ICHIP2128, incomingBuffer);
                            break;

                    }
                    //if we got an I/ in the reply then the command is done sending data. So, see if there is a buffered cmd to send.
                    if (strstr(incomingBuffer, "I/") != NULL) {
                        if (psReadPtr != psWritePtr) { //if there is a parameter in the buffer to send then do it
                            if (Logger::isDebug()) {
                                Logger::debug(ICHIP2128, "Sending buffered cmd: %s", sendBuffer[psReadPtr].cmd.c_str());
                            }
                            state = IDLE;
                            sendCmd(sendBuffer[psReadPtr].cmd, sendBuffer[psReadPtr].state);
                            psReadPtr++;
                            if (psReadPtr >= CFG_SERIAL_SEND_BUFFER_SIZE) {
                                psReadPtr = 0;
                            }
                        }
                    }
                }
                return; // before processing the next line, return to the loop() to allow other devices to process.
            } else { // add more characters
                if (incoming != 10) { // don't add a LF character
                    incomingBuffer[ibWritePtr++] = (char) incoming;
                }
            }
        } else {
            return;
        }
    }

    if (millis() > lastSendTime + 1000) { //if the last sent command hasn't gotten a reply in 1 second
        state = IDLE; //something went wrong so reset state
        //sendCmd(lastSentCmd, lastSentState); //try to resend it
        //The sendCmd call resets lastSentTime so this will at most be called every second until the iChip interface decides to cooperate.
    }
}

/*
 * Process the parameter update from ichip we received as a response to AT+iWNXT.
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 */
void ICHIPWIFI::processParameterChange(char *key)
{
    PotThrottleConfiguration *acceleratorConfig = NULL;
    PotThrottleConfiguration *brakeConfig = NULL;
    MotorControllerConfiguration *motorConfig = NULL;
    SystemIOConfiguration *systemIOConfig = (SystemIOConfiguration *)systemIO->getConfiguration();
    bool parameterFound = true;

    char *value = strchr(key, '=');

    if (!value) {
        return;
    }

    DeviceManager *deviceManager = DeviceManager::getInstance();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();
    MotorController *motorController = deviceManager->getMotorController();

    if (accelerator) {
        acceleratorConfig = (PotThrottleConfiguration *) accelerator->getConfiguration();
    }

    if (brake) {
        brakeConfig = (PotThrottleConfiguration *) brake->getConfiguration();
    }

    if (motorController) {
        motorConfig = (MotorControllerConfiguration *) motorController->getConfiguration();
    }

    value[0] = 0; // replace the '=' sign with a 0
    value++;

    if (value[0] == '"') {
        value++;    // if the value starts with a '"', advance one character
    }

    if (value[strlen(value) - 1] == '"') {
        value[strlen(value) - 1] = 0;    // if the value ends with a '"' character, replace it with 0
    }

    if (!strcmp(key, Constants::numThrottlePots) && acceleratorConfig) {
        acceleratorConfig->numberPotMeters = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleSubType) && acceleratorConfig) {
        acceleratorConfig->throttleSubType = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMin1) && acceleratorConfig) {
        acceleratorConfig->minimumLevel1 = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMin2) && acceleratorConfig) {
        acceleratorConfig->minimumLevel2 = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMax1) && acceleratorConfig) {
        acceleratorConfig->maximumLevel1 = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMax2) && acceleratorConfig) {
        acceleratorConfig->maximumLevel2 = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleRegenMax) && acceleratorConfig) {
        acceleratorConfig->positionRegenMaximum = atol(value) * 10;
    } else if (!strcmp(key, Constants::throttleRegenMin) && acceleratorConfig) {
        acceleratorConfig->positionRegenMinimum = atol(value) * 10;
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleFwd) && acceleratorConfig) {
        acceleratorConfig->positionForwardMotionStart = atol(value) * 10;
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMap) && acceleratorConfig) {
        acceleratorConfig->positionHalfPower = atol(value) * 10;
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMinRegen) && acceleratorConfig) {
        acceleratorConfig->minimumRegen = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleMaxRegen) && acceleratorConfig) {
        acceleratorConfig->maximumRegen = atol(value);
        accelerator->saveConfiguration();
    } else if (!strcmp(key, Constants::throttleCreep) && acceleratorConfig) {
        acceleratorConfig->creep = atol(value);
        accelerator->saveConfiguration();
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
    } else if (!strcmp(key, Constants::nominalVolt) && motorConfig) {
        motorConfig->nominalVolt = (atol(value)) * 10;
        motorController->saveConfiguration();
    } else if (!strcmp(key, Constants::prechargeMillis)) {
        systemIOConfig->prechargeMillis = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::prechargeRelayOutput)) {
        systemIOConfig->prechargeRelayOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::mainContactorOutput)) {
        systemIOConfig->mainContactorOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::secondaryContactorOutput)) {
        systemIOConfig->secondaryContactorOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::enableMotorOutput)) {
        systemIOConfig->enableMotorOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::coolingFanOutput)) {
        systemIOConfig->coolingFanOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::coolingTempOn)) {
        systemIOConfig->coolingTempOn = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::coolingTempOff)) {
        systemIOConfig->coolingTempOff = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::brakeLightOutput)) {
        systemIOConfig->brakeLightOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::reverseLightOutput)) {
        systemIOConfig->reverseLightOutput = atol(value);
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::logLevel)) {
        extern PrefHandler *sysPrefs;
        uint8_t loglevel = atol(value);
        Logger::setLoglevel((Logger::LogLevel) loglevel);
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
void ICHIPWIFI::loadParameters()
{
    DeviceManager *deviceManager = DeviceManager::getInstance();
    MotorController *motorController = deviceManager->getMotorController();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();
    PotThrottleConfiguration *acceleratorConfig = NULL;
    PotThrottleConfiguration *brakeConfig = NULL;
    MotorControllerConfiguration *motorConfig = NULL;
    SystemIOConfiguration *systemIOConfig = (SystemIOConfiguration *)systemIO->getConfiguration();

    Logger::info("loading config params to ichip/wifi");

    if (accelerator) {
        acceleratorConfig = (PotThrottleConfiguration *) accelerator->getConfiguration();
    }

    if (brake) {
        brakeConfig = (PotThrottleConfiguration *) brake->getConfiguration();
    }

    if (motorController) {
        motorConfig = (MotorControllerConfiguration *) motorController->getConfiguration();
    }

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
        uint16_t nmvlt = motorConfig->nominalVolt / 10;
        setParam(Constants::nominalVolt, nmvlt);
        setParam(Constants::torqueMax, (uint16_t)(motorConfig->torqueMax / 10));   // skip the tenth's
    }

    setParam(Constants::enableInput, systemIOConfig->enableInput);
    setParam(Constants::prechargeMillis, systemIOConfig->prechargeMillis);
    setParam(Constants::prechargeRelayOutput, systemIOConfig->prechargeRelayOutput);
    setParam(Constants::mainContactorOutput, systemIOConfig->mainContactorOutput);
    setParam(Constants::secondaryContactorOutput, systemIOConfig->secondaryContactorOutput);
    setParam(Constants::enableMotorOutput, systemIOConfig->enableMotorOutput);
    setParam(Constants::brakeLightOutput, systemIOConfig->brakeLightOutput);
    setParam(Constants::reverseLightOutput, systemIOConfig->reverseLightOutput);
    setParam(Constants::coolingFanOutput, systemIOConfig->coolingFanOutput);
    setParam(Constants::coolingTempOn, systemIOConfig->coolingTempOn);
    setParam(Constants::coolingTempOff, systemIOConfig->coolingTempOff);

    setParam(Constants::logLevel, (uint8_t) Logger::getLogLevel());
    setParam(Constants::bitfield1, status->getBitField1());
    setParam(Constants::bitfield2, status->getBitField2());
    setParam(Constants::bitfield3, status->getBitField3());
    setParam(Constants::systemState, status->systemStateToStr(status->getSystemState()));

    Logger::info("Wifi Parameters loaded...");
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

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
        Logger::debug(ICHIP2128, "Valid checksum so using stored wifi config values");
        //TODO: implement processing of config params for WIFI
//      prefsHandler->read(EESYS_WIFI0_SSID, &config->ssid);
    }
}

void ICHIPWIFI::saveConfiguration()
{
    WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

    //TODO: implement processing of config params for WIFI
//  prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
//  prefsHandler->saveChecksum();
}
