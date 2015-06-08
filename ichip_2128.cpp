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

    lastSendTime = millis();
    lastSentState = IDLE;
    lastSentCmd = String("");

    activeSockets[0] = -1;
    activeSockets[1] = -1;
    activeSockets[2] = -1;
    activeSockets[3] = -1;

    state = IDLE;

    didParamLoad = false;
    didTCPListener = false;

    serialInterface->begin(115200);

    paramCache.timeRunning = 0;

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
    sendCmd(cmd, IDLE);
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
            Logger::debug(ICHIP2128, "Buffer cmd: %s, ", cmd.c_str());
        }
    } else { //otherwise, go ahead and blast away
        serialInterface->write(Constants::ichipCommandPrefix);
        serialInterface->print(cmd);
        serialInterface->write(13);
        state = cmdstate;
        lastSendTime = millis();
        lastSentCmd = String(cmd);
        lastSentState = cmdstate;

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
    static int pollSocket = 0;
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
    for (int c = 0; c < 4; c++) {
        if (activeSockets[c] != -1) {
            char buff[6];
            sprintf(buff, "%03i", activeSockets[c]);
            String temp = "SRCV:" + String(buff) + ",80";
            sendCmd(temp, GET_SOCKET);
        }
    }

    // update main gauges every tick
    if (motorController) {
        if (paramCache.speedActual != motorController->getSpeedActual()) {
            paramCache.speedActual = motorController->getSpeedActual();
            setParam(Constants::speedActual, paramCache.speedActual);
        }
        if (paramCache.torqueActual != motorController->getTorqueActual()) {
            paramCache.torqueActual = motorController->getTorqueActual();
            setParam(Constants::torqueActual, paramCache.torqueActual / 10.0f, 1);
        }
        if (paramCache.dcCurrent != motorController->getDcCurrent()) {
            paramCache.dcCurrent = motorController->getDcCurrent();
            setParam(Constants::dcCurrent, paramCache.dcCurrent / 10.0f, 1);
        }
    }
    if (accelerator) {
        if (paramCache.throttle != accelerator->getLevel()) {
            paramCache.throttle = accelerator->getLevel();
            setParam(Constants::throttle, paramCache.throttle / 10.0f, 1);
        }
    }


    // make small slices so the main loop is not blocked for too long
    if (tickCounter == 1) {
        // just update this every second or so
        if (ms > paramCache.timeRunning + 1000) {
            paramCache.timeRunning = ms;
            setParam(Constants::timeRunning, getTimeRunning());
        }
        if (motorController) {
            if (paramCache.torqueRequested != motorController->getTorqueRequested()) {
                paramCache.torqueRequested = motorController->getTorqueRequested();
                setParam(Constants::torqueRequested, paramCache.torqueRequested / 10.0f, 1);
            }
        }
        if (brake) {
            if (paramCache.brake != brake->getLevel()) {
                paramCache.brake = brake->getLevel();
                setParam(Constants::brake, paramCache.brake / 10.0f, 1);
            }
        }
    } else if (tickCounter == 2) {
        if (motorController) {
            if (paramCache.dcVoltage != motorController->getDcVoltage()) {
                paramCache.dcVoltage = motorController->getDcVoltage();
                setParam(Constants::dcVoltage, paramCache.dcVoltage / 10.0f, 1);
            }
            if (paramCache.kiloWattHours != motorController->getKiloWattHours() / 3600000) {
                paramCache.kiloWattHours = motorController->getKiloWattHours() / 3600000;
                setParam(Constants::kiloWattHours, paramCache.kiloWattHours / 10.0f, 1);
            }
        }
        if (paramCache.systemState != status->getSystemState()) {
            paramCache.systemState = status->getSystemState();
            setParam(Constants::systemState, status->systemStateToStr(status->getSystemState()));
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
        if (motorController) {
            if (paramCache.mechanicalPower != motorController->getMechanicalPower()) {
                paramCache.mechanicalPower = motorController->getMechanicalPower();
                setParam(Constants::mechanicalPower, paramCache.mechanicalPower / 10.0f, 1);
            }
        }
    } else if (tickCounter > 3) {
        if (motorController) {
            if (paramCache.temperatureMotor != motorController->getTemperatureMotor()) {
                paramCache.temperatureMotor = motorController->getTemperatureMotor();
                setParam(Constants::temperatureMotor, paramCache.temperatureMotor / 10.0f, 1);
            }
            if (paramCache.temperatureController != motorController->getTemperatureController()) {
                paramCache.temperatureController = motorController->getTemperatureController();
                setParam(Constants::temperatureController, paramCache.temperatureController / 10.0f, 1);
            }
            if (paramCache.gear != motorController->getSelectedGear()) {
                paramCache.gear = motorController->getSelectedGear();
                setParam(Constants::gear, (uint16_t) paramCache.gear);
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
    int seconds = (int) (ms / 1000) % 60;
    int minutes = (int) ((ms / (1000 * 60)) % 60);
    int hours = (int) ((ms / (1000 * 3600)) % 24);
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
        loop();
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
                    Logger::debug(ICHIP2128, "incoming: '%s', state: %d", incomingBuffer, state);
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
                    if (incomingBuffer[0] == 'I' && incomingBuffer[1] == '/') {
Logger::debug(ICHIP2128, "a, read %d, write %d", psReadPtr, psWritePtr);
                        if (psReadPtr != psWritePtr) { //if there is a parameter in the buffer to send then do it
                            if (Logger::isDebug()) {
                                Logger::debug(ICHIP2128, "Sending buffered cmd: %s", sendBuffer[psReadPtr].cmd.c_str());
                            }
                            state = IDLE;
                            sendCmd(sendBuffer[psReadPtr].cmd, sendBuffer[psReadPtr++].state);
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
        Logger::info(ICHIP2128, "parameter change: %s = %s", key, value);
        getNextParam(); // try to get another one immediately
    }
}

bool ICHIPWIFI::processParameterChangeThrottle(char *key, char *value)
{
    Throttle *throttle = DeviceManager::getInstance()->getAccelerator();

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
                config->positionRegenMaximum = atol(value) * 10;
            } else if (!strcmp(key, Constants::positionRegenMinimum)) {
                config->positionRegenMinimum = atol(value) * 10;
            } else if (!strcmp(key, Constants::positionForwardMotionStart)) {
                config->positionForwardMotionStart = atol(value) * 10;
            } else if (!strcmp(key, Constants::positionHalfPower)) {
                config->positionHalfPower = atol(value) * 10;
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
    Throttle *brake = DeviceManager::getInstance()->getBrake();

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
    MotorController *motorController = DeviceManager::getInstance()->getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::speedMax)) {
                config->speedMax = atol(value);
            } else if (!strcmp(key, Constants::torqueMax)) {
                config->torqueMax = atol(value) * 10;
            } else if (!strcmp(key, Constants::nominalVolt)) {
                config->nominalVolt = atol(value) * 10;
            } else if (!strcmp(key, Constants::motorMode)) {
                config->powerMode = (atol(value) ? modeSpeed : modeTorque);
            } else if (!strcmp(key, Constants::invertDirection)) {
                config->invertDirection = atol(value);
            } else if (!strcmp(key, Constants::torqueSlewRate)) {
                config->torqueSlewRate = atol(value) * 10;
            } else if (!strcmp(key, Constants::speedSlewRate)) {
                config->speedSlewRate = atol(value) * 10;
            } else if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;

                if (!strcmp(key, Constants::maxMechanicalPowerMotor)) {
                    dmc5Config->maxMechanicalPowerMotor = atol(value) / 4;
                } else if (!strcmp(key, Constants::maxMechanicalPowerRegen)) {
                    dmc5Config->maxMechanicalPowerRegen = atol(value) / 4;
                } else if (!strcmp(key, Constants::dcVoltLimitMotor)) {
                    dmc5Config->dcVoltLimitMotor = atol(value) * 10;
                } else if (!strcmp(key, Constants::dcVoltLimitRegen)) {
                    dmc5Config->dcVoltLimitRegen = atol(value) * 10;
                } else if (!strcmp(key, Constants::dcCurrentLimitMotor)) {
                    dmc5Config->dcCurrentLimitMotor = atol(value) * 10;
                } else if (!strcmp(key, Constants::dcCurrentLimitRegen)) {
                    dmc5Config->dcCurrentLimitRegen = atol(value) * 10;
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
    Charger *charger = DeviceManager::getInstance()->getCharger();

    if (charger) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::maximumInputCurrent)) {
                config->maximumInputCurrent = atol(value) * 10;
            } else if (!strcmp(key, Constants::constantCurrent)) {
                config->constantCurrent = atol(value) * 10;
            } else if (!strcmp(key, Constants::constantVoltage)) {
                config->constantVoltage = atol(value) * 10;
            } else if (!strcmp(key, Constants::terminateCurrent)) {
                config->terminateCurrent = atol(value) * 10;
            } else if (!strcmp(key, Constants::minimumBatteryVoltage)) {
                config->minimumBatteryVoltage = atol(value) * 10;
            } else if (!strcmp(key, Constants::maximumBatteryVoltage)) {
                config->maximumBatteryVoltage = atol(value) * 10;
            } else if (!strcmp(key, Constants::minimumTemperature)) {
                config->minimumTemperature = atol(value) * 10;
            } else if (!strcmp(key, Constants::maximumTemperature)) {
                config->maximumTemperature = atol(value) * 10;
            } else if (!strcmp(key, Constants::maximumAmpereHours)) {
                config->maximumAmpereHours = atol(value) * 10;
            } else if (!strcmp(key, Constants::maximumChargeTime)) {
                config->maximumChargeTime = atol(value);
            } else if (!strcmp(key, Constants::deratingRate)) {
                config->deratingRate = atol(value) * 10;
            } else if (!strcmp(key, Constants::deratingReferenceTemperature)) {
                config->deratingReferenceTemperature = atol(value) * 10;
            } else if (!strcmp(key, Constants::hystereseStopTemperature)) {
                config->hystereseStopTemperature = atol(value) * 10;
            } else if (!strcmp(key, Constants::hystereseResumeTemperature)) {
                config->hystereseResumeTemperature = atol(value) * 10;
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
    DcDcConverter *dcDcConverter = DeviceManager::getInstance()->getDcDcConverter();

    if (dcDcConverter) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();

        if (config) {
            if (!strcmp(key, Constants::dcDcMode)) {
                config->mode = atol(value);
            } else if (!strcmp(key, Constants::lowVoltageCommand)) {
                config->lowVoltageCommand = atol(value) * 10;
            } else if (!strcmp(key, Constants::hvUndervoltageLimit)) {
                config->hvUndervoltageLimit = atol(value);
            } else if (!strcmp(key, Constants::lvBuckModeCurrentLimit)) {
                config->lvBuckModeCurrentLimit = atol(value);
            } else if (!strcmp(key, Constants::hvBuckModeCurrentLimit)) {
                config->hvBuckModeCurrentLimit = atol(value) * 10;
            } else if (!strcmp(key, Constants::highVoltageCommand)) {
                config->highVoltageCommand = atol(value);
            } else if (!strcmp(key, Constants::lvUndervoltageLimit)) {
                config->lvUndervoltageLimit = atol(value) * 10;
            } else if (!strcmp(key, Constants::lvBoostModeCurrentLimit)) {
                config->lvBoostModeCurrentLinit = atol(value);
            } else if (!strcmp(key, Constants::hvBoostModeCurrentLimit)) {
                config->hvBoostModeCurrentLimit = atol(value) * 10;
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
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO->getConfiguration();

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
        systemIO->saveConfiguration();
    } else if (!strcmp(key, Constants::warningOutput)) {
        config->warningOutput = atol(value);
    } else if (!strcmp(key, Constants::powerLimitationOutput)) {
        config->powerLimitationOutput = atol(value);
    } else {
        return false;
    }
    return true;
}

bool ICHIPWIFI::processParameterChangeDevices(char *key, char *value)
{
    if (key[0] == 'x' && atol(&key[1]) > 0) {
        long deviceId = strtol(key + 1, 0, 16);
        DeviceManager::getInstance()->sendMessage(DEVICE_ANY, (DeviceId) deviceId, (atol(value) ? MSG_ENABLE : MSG_DISABLE), NULL);
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

    Logger::info("wifi parameters loaded");
}

void ICHIPWIFI::loadParametersThrottle()
{
    Throttle *throttle = DeviceManager::getInstance()->getAccelerator();

    if (throttle) {
        PotThrottleConfiguration *throttleConfig = (PotThrottleConfiguration *) throttle->getConfiguration();

        if (throttleConfig) {
            setParam(Constants::numberPotMeters, throttleConfig->numberPotMeters);
            setParam(Constants::throttleSubType, throttleConfig->throttleSubType);
            setParam(Constants::minimumLevel, throttleConfig->minimumLevel);
            setParam(Constants::minimumLevel2, throttleConfig->minimumLevel2);
            setParam(Constants::maximumLevel, throttleConfig->maximumLevel);
            setParam(Constants::maximumLevel2, throttleConfig->maximumLevel2);
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
    Throttle *brake = DeviceManager::getInstance()->getBrake();

    if (brake) {
        PotThrottleConfiguration *brakeConfig = (PotThrottleConfiguration *) brake->getConfiguration();

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
    MotorController *motorController = DeviceManager::getInstance()->getMotorController();

    if (motorController) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();

        if (config) {
            setParam(Constants::speedMax, config->speedMax);
            setParam(Constants::nominalVolt, config->nominalVolt / 10.0f, 1);
            setParam(Constants::torqueMax, config->torqueMax / 10.0f, 1);
            setParam(Constants::motorMode, (uint8_t) config->powerMode);
            setParam(Constants::invertDirection, (uint8_t)(config->invertDirection ? 1 : 0));
            setParam(Constants::torqueSlewRate, config->torqueSlewRate / 10.0f, 1);
            setParam(Constants::speedSlewRate, config->speedSlewRate / 10.0f, 1);
            if (motorController->getId() == BRUSA_DMC5) {
                BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
                setParam(Constants::maxMechanicalPowerMotor, (uint32_t)(dmc5Config->maxMechanicalPowerMotor * 4));
                setParam(Constants::maxMechanicalPowerRegen, (uint32_t)(dmc5Config->maxMechanicalPowerRegen * 4));
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
    Charger *charger = DeviceManager::getInstance()->getCharger();

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
    DcDcConverter *dcDcConverter = DeviceManager::getInstance()->getDcDcConverter();

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
    SystemIOConfiguration *config = (SystemIOConfiguration *) systemIO->getConfiguration();

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
        device = DeviceManager::getInstance()->getDeviceByID(deviceIds[i]);
        if (device != NULL) {
            sprintf(idHex, "x%x", deviceIds[i]);
            setParam(idHex, (uint8_t)((device->isEnabled() == true) ? 1 : 0));
        }
    }
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
