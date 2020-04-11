/*
 * Esp32Wifi.cpp
 *
 * Class to interface with the Esp32 based wifi adapter
 *
 Copyright (c) 2020 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#include "WifiEsp32.h"

WifiEsp32::WifiEsp32() : Wifi()
{
    prefsHandler = new PrefHandler(ESP32WIFI);

    serialInterface->begin(115200);

    commonName = "WIFI (ESP32)";

    didParamLoad = false;
    tickCounter = watchdogCounter = 0;
    ibWritePtr = psWritePtr = psReadPtr = 0;
    lastSendTime = timeStarted = 0;
    remainingSocketRead = -1;

    pinMode(CFG_WIFI_RESET, OUTPUT);
    pinMode(CFG_WIFI_ENABLE, OUTPUT);
}

/**
 * \brief Initialization of hardware and parameters
 *
 */
void WifiEsp32::setup()
{
    digitalWrite(CFG_WIFI_RESET, HIGH);
    digitalWrite(CFG_WIFI_ENABLE, HIGH);

    didParamLoad = false;
    tickCounter = watchdogCounter = 0;
    ibWritePtr = psWritePtr = psReadPtr = 0;
    remainingSocketRead = -1;
    lastSendTime = timeStarted = millis();

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
void WifiEsp32::tearDown()
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
void WifiEsp32::sendCmd(String cmd)
{
    serialInterface->print(cmd);
    serialInterface->write(13);
    lastSendTime = millis();
    if (logger.isDebug()) {
        logger.debug(this, "Send cmd: %s", cmd.c_str());
    }
}

/**
 * \brief Send update to all active sockets
 *
 * The message to be sent is created by the assigned SocketProcessor
 *
 */
void WifiEsp32::sendSocketUpdate()
{
/*    { 0, 2, true, 0, "systemState" }, // uint16_t
    { 1, 2, false, 10, "torqueActual" }, // int16_t
    { 2, 2, false, 0, "speedActual" }, // int16_t
    { 3, 2, false, 10, "throttle" }, // int16_t

    { 10, 2, true, 10, "dcVoltage" }, // uint16_t
    { 11, 2, false, 10, "dcCurrent" }, // int16_t
    { 12, 2, false, 0, "acCurrent" }, // int16_t
    { 13, 2, false, 10, "temperatureMotor" }, // int16_t
    { 14, 2, false, 10, "temperatureController" }, // int16_t
    { 15, 2, false, 0, "mechanicalPower" }, // int16_t

    { 20, 4, true, 0, "bitfieldMotor" }, // uint32_t
    { 21, 4, true, 0, "bitfieldBms" }, // uint32_t
    { 22, 4, true, 0, "bitfieldIO" }, // uint32_t

    { 30, 2, true, 10, "dcDcHvVoltage" }, // uint16_t
    { 31, 2, true, 10, "dcDcLvVoltage" }, // uint16_t
    { 32, 2, false, 10, "dcDcHvCurrent" }, // int16_t
    { 33, 2, false, 0, "dcDcLvCurrent" }, // int16_t
    { 34, 2, false, 10, "dcDcTemperature" }, // int16_t

    { 40, 2, true, 10, "chargerInputVoltage" }, // uint16_t
    { 41, 2, true, 100, "chargerInputCurrent" }, // uint16_t
    { 42, 2, true, 10, "chargerBatteryVoltage" }, // uint16_t
    { 43, 2, true, 100, "chargerBatteryCurrent" }, // uint16_t
    { 44, 2, false, 10, "chargerTemperature" }, // int16_t
    { 45, 2, false, 10, "maximumSolarCurrent" }, // int16_t
    { 46, 2, true, 10, "chargeHoursRemain" }, // uint16_t
    { 47, 2, true, 10, "chargeMinsRemain" }, // uint16_t
    { 48, 2, true, 100, "chargeLevel" }, // uint16_t

    { 50, 4, true, 100, "flowCoolant" }, // uint32_t
    { 51, 4, true, 100, "flowHeater" }, // uint32_t
    { 52, 2, true, 0, "heaterPower" }, // uint16_t
    { 53, 2, false, 10, "temperatureBattery1" }, // int16_t
    { 54, 2, false, 10, "temperatureBattery2" }, // int16_t
    { 55, 2, false, 10, "temperatureBattery3" }, // int16_t
    { 56, 2, false, 10, "temperatureBattery4" }, // int16_t
    { 57, 2, false, 10, "temperatureBattery5" }, // int16_t
    { 58, 2, false, 10, "temperatureBattery6" }, // int16_t
    { 59, 2, false, 10, "temperatureCoolant" }, // int16_t
    { 60, 2, false, 0, "temperatureHeater" }, // int16_t
    { 61, 2, false, 10, "temperatureExterior" }, // int16_t

    { 70, 1, true, 0, "powerSteering" }, // bool
    { 71, 1, true, 0, "enableRegen" }, // bool
    { 72, 1, true, 0, "enableHeater" }, // bool
    { 73, 1, true, 0, "enableCreep" }, // bool
    { 74, 2, false, 0, "cruiseControlSpeed" }, // int16_t
    { 75, 1, true, 0, "cruiseControlEnable" }, // bool

    { 80, 2, true, 0, "packResistance" }, // uint16_t
    { 81, 2, true, 0, "packHealth" }, // uint8_t
    { 82, 2, true, 0, "packCycles" }, // uint16_t
    { 83, 2, true, 100, "soc" }, // uint16_t
    { 84, 2, true, 0, "dischargeLimit" }, // uint16_t
    { 85, 2, true, 0, "chargeLimit" }, // uint16_t
    { 86, 1, true, 0, "chargeAllowed" }, // bool
    { 87, 1, true, 0, "dischargeAllowed" }, // bool
    { 88, 2, false, 10, "lowestCellTemp" }, // int16_t
    { 89, 2, false, 10, "highestCellTemp" }, // int16_t
    { 90, 2, true, 10000, "lowestCellVolts" }, // uint16_t
    { 91, 2, true, 10000, "highestCellVolts" }, // uint16_t
    { 92, 2, true, 10000, "averageCellVolts" }, // uint16_t
    { 93, 2, true, 10000, "deltaCellVolts" }, // uint16_t
    { 94, 2, true, 100, "lowestCellResistance" }, // uint16_t
    { 95, 2, true, 100, "highestCellResistance" }, // uint16_t
    { 96, 2, true, 100, "averageCellResistance" }, // uint16_t
    { 97, 2, true, 100, "deltaCellResistance" }, // uint16_t
    { 98, 1, true, 0, "lowestCellTempId" }, // uint8_t
    { 99, 1, true, 0, "highestCellTempId" }, // uint8_t
    { 100, 1, true, 0, "lowestCellVoltsId" }, // uint8_t
    { 101, 1, true, 0, "highestCellVoltsId" }, // uint8_t
    { 102, 1, true, 0, "lowestCellResistanceId" }, // uint8_t
    { 103, 1, true, 0, "highestCellResistanceId" }, // uint8_t
    { 104, 1, true, 0, "bmsTemperature" } // uint8_t
*/

/*    for (int i = 0; i < CFG_WIFI_NUM_SOCKETS; i++) {
        if (socket[i].handle != -1 && socket[i].processor != NULL) {
            String data = socket[i].processor->generateUpdate();
            sendToSocket(&socket[i], data);
        }
    }*/
}

/**
 * \brief Perform regular tasks triggered by a timer
 *
 * Query for changed parameters of the config page, socket status
 * and query for incoming data on sockets
 *
 */
void WifiEsp32::handleTick()
{
/*    if (watchdogCounter++ > 250) { // after 250 * 100ms no reception, reset esp
        logger.warn(this, "watchdog: no response from Esp32");
        reset();
        return;
    }
*/
    sendSocketUpdate();

    if (!didParamLoad && millis() > 3000 + timeStarted) {
        loadParameters();
        didParamLoad = true;
    }
}

/**
 * \brief Handle a message sent by the DeviceManager
 *
 * \param messageType the type of the message
 * \param message the message to be processed
 */
void WifiEsp32::handleMessage(uint32_t messageType, void* message)
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

    case MSG_LOG:
        char **params = (char **) message;
//            String data = socket[i].processor->generateLogEntry(params[0], params[1], params[2]);
//            sendToSocket(&socket[i], data);
        break;
    }
}

/**
 * \brief Act on system state changes and send an update to the socket client
 *
 * \param oldState the previous system state
 * \param newState the new system state
 */
void WifiEsp32::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    Device::handleStateChange(oldState, newState);
    sendSocketUpdate();
}

/**
 * \brief Process serial input waiting from the wifi module.
 *
 * The method is called by the main loop
 */
void WifiEsp32::process()
{
    int incoming;
    while (serialInterface->available()) {
        incoming = serialInterface->read();
//SerialUSB.print((char)incoming);
        if (incoming == -1) { //and there is no reason it should be -1
            return;
        }

        if (incoming == 13 || ibWritePtr > (CFG_WIFI_BUFFER_SIZE - 2)) {
            incomingBuffer[ibWritePtr] = 0;
            ibWritePtr = 0;

            if (logger.isDebug()) {
                logger.debug(this, "incoming: '%s'", incomingBuffer);
            }

            return; // before processing the next line, return to the loop() to allow other devices to process.
        } else { // add more characters
            if (incoming != 10) { // don't add a LF character
                incomingBuffer[ibWritePtr++] = (char) incoming;
            }
        }
    }
}

/**
 * \brief Process incoming data from a socket
 *
 * The data is forwarded to the socket's assigned SocketProcessor. The processor prepares a
 * response which is sent back to the client via the socket.
 *
 */
void WifiEsp32::processIncomingSocketData()
{
    logger.debug(this, "processing incoming socket data");

}

/**
 * \brief Set a parameter to the given string value
 *
 * \param paramName the name of the parameter
 * \param value the value to set
 */
void WifiEsp32::setParam(String paramName, String value)
{
    serialInterface->println("cfg:" + paramName + "=" + value);
}

/**
 * \brief Reset the ichip to overcome a crash
 *
 */
void WifiEsp32::reset()
{
    logger.info(this, "resetting the Esp32Wifi");

    // cycle reset pin (next tick() will activate it again
    digitalWrite(CFG_WIFI_RESET, LOW);
    running = false;
    ready = false;
}


/**
 * \brief Get the device type
 *
 * \return the type of the device
 */
DeviceType WifiEsp32::getType()
{
    return DEVICE_WIFI;
}

/**
 * \brief Get the device ID
 *
 * \return the ID of the device
 */
DeviceId WifiEsp32::getId()
{
    return ESP32WIFI;
}
