/*
 * WebSocket.cpp
 *
 * Class to handle web socket data for the dashboard
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

#include "WebSocket.h"

static const char *webSocketKeyName = "Sec-WebSocket-Key";
static const char *websocketUid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/**
 * \brief Constructor
 *
 */
WebSocket::WebSocket()
{
    paramCache.timeRunning = 0;
    webSocketKey = NULL;
    isFirst = true;
    updateCounter = 0;
    connected = false;
    dcVoltageMin = 0;
    dcVoltageMax = 0;
    dcCurrentMin = 0;
    dcCurrentMax = 0;
    temperatureMotorMax = 0;
    temperatureControllerMax = 0;
}

/**
 * \brief Initialize parameter cache so all params are sent when connecting
 *
 */
void WebSocket::initParamCache()
{
    paramCache.timeRunning = millis(); // this way less important data is sent one second later
    paramCache.torqueActual = -1;
    paramCache.throttle = -1;
    paramCache.speedActual = -1;
    paramCache.dcVoltage = -1;
    paramCache.dcCurrent = -1;
    paramCache.acCurrent = -1;
    paramCache.bitfieldMotor = 0;
    paramCache.bitfieldBms = 0;
    paramCache.bitfieldIO = 0;
    paramCache.systemState = 0;
    paramCache.gear = MotorController::ERROR;
    paramCache.temperatureMotor = -1;
    paramCache.temperatureController = -1;
//    paramCache.mechanicalPower = -1;
    paramCache.dcDcHvVoltage = 0;
    paramCache.dcDcLvVoltage = 0;
    paramCache.dcDcHvCurrent = 0;
    paramCache.dcDcLvCurrent = 0;
    paramCache.dcDcTemperature = 0;
    paramCache.chargerInputVoltage = 0;
    paramCache.chargerInputCurrent = 0;
    paramCache.chargerBatteryVoltage = 0;
    paramCache.chargerBatteryCurrent = 0;
    paramCache.chargerTemperature = 0;
    paramCache.flowCoolant = 0;
    paramCache.flowHeater = 0;
    paramCache.heaterPower = 0;
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        paramCache.temperatureBattery[i] = CFG_NO_TEMPERATURE_DATA;
    }
    paramCache.temperatureCoolant = CFG_NO_TEMPERATURE_DATA;
    paramCache.temperatureHeater = CFG_NO_TEMPERATURE_DATA;
    paramCache.temperatureExterior = CFG_NO_TEMPERATURE_DATA;
    paramCache.enableRegen = !status.enableRegen;
    paramCache.powerSteering = !status.powerSteering;
    paramCache.enableHeater = !status.enableHeater;
    paramCache.enableCreep = !status.enableCreep;

    paramCache.soc = 0;
    paramCache.dischargeLimit = 0;
    paramCache.chargeLimit = 0;
    paramCache.chargeAllowed = 0;
    paramCache.dischargeAllowed = 0;
    paramCache.lowestCellTemp = 0;
    paramCache.highestCellTemp = 0;
    paramCache.lowestCellVolts = 0;
    paramCache.highestCellVolts = 0;
    paramCache.averageCellVolts = 0;
    paramCache.deltaCellVolts = 0;
    paramCache.lowestCellResistance = 0;
    paramCache.highestCellResistance = 0;
    paramCache.averageCellResistance = 0;
    paramCache.deltaCellResistance = 0;
    paramCache.lowestCellTempId = 0;
    paramCache.highestCellTempId = 0;
    paramCache.lowestCellVoltsId = 0;
    paramCache.highestCellVoltsId = 0;
    paramCache.lowestCellResistanceId = 0;
    paramCache.highestCellResistanceId = 0;
}

/**
 * \brief Process requests from a web socket client
 *
 * Depending on connection state the header or the
 * incoming data is processed.
 *
 * \param input incoming data
 * \return the encoded socket response
 */
String WebSocket::processInput(char *input)
{
    if (connected) {
        return processData(input);
    } else {
        // split the input into separate lines and process each line
        char *line = strtok(input, "\r\n");
        while (line != NULL) {
            if (line[0] != 0) {
                processConnectionRequest(line);
            }
            line = strtok(NULL, "\r\n");
        }
        return prepareConnectionRequestResponse();
    }
}

/**
 * \brief Process websocket connection request
 *
 * The method is called several times until the key is received.
 *
 * \param input incoming data
 */
void WebSocket::processConnectionRequest(char *input)
{
    // grab the key
    char* key = strstr(input, webSocketKeyName);
    if (key != NULL) {
        webSocketKey = new String(key + strlen(webSocketKeyName) + 2);
        Logger::debug("websocket: found key: %s", webSocketKey->c_str());
    }
}

/**
 * \brief Prepare response to a connection request
 *
 * \return the response which is to be sent back to the browser
 */
String WebSocket::prepareConnectionRequestResponse() {
    String response;

    // they're done (empty line), send our response
    if (webSocketKey != NULL) {
        char acceptHash[128];
        Logger::debug("websocket: got a key and an empty line, let's go");
        webSocketKey->concat(websocketUid); // append the UID to the key
        // generate SHA1 hash of new key
        Sha1.init();
        Sha1.print(webSocketKey->c_str());
        uint8_t* hash = Sha1.result();
        // base64 encode the hash of the new key
        base64_encode(acceptHash, (char*) (hash), 20);

        response.concat("HTTP/1.1 101 Switching Protocols\r\n");
        response.concat("Upgrade: websocket\r\n");
        response.concat("Connection: Upgrade\r\n");
        response.concat("Sec-WebSocket-Accept: ");
        response.concat(acceptHash);
        response.concat("\r\n\r\n");
        initParamCache();
        webSocketKey = NULL;
        connected = true;
    }
    return response;
}

/**
 * \brief Process incoming data from the client
 *
 * \param input the encoded data from the socket
 * \return a response indicating if the connection should remain open
 *
 * Incoming data format:
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 *
 */
String WebSocket::processData(char *input)
{
    if (input == NULL || input[0] == 0) {
        return "";
    }

    bool fin = (input[0] & 0x80) != 0;
    int opcode = (input[0] & 0x0f);
    bool mask = (input[1] & 0x80) != 0;
    long payloadLength = input[1] & 0x7f;

    Logger::debug("websocket: fin: %#x, opcode: %#x, mask: %#x, length: %d", fin, opcode, mask, payloadLength);

    uint8_t offset = 2;
    if (payloadLength == 0x7e) { // 126 -> use next two bytes as unsigned 16bit length of payload
        payloadLength = input[offset] << 8 + input[offset + 1];
        Logger::debug("websocket: extended 16-bit length: %d", payloadLength);
        offset += 2;
    }
    if (payloadLength == 0x7f) {
        Logger::warn("websocket: >64k frames not supported");
        return "";
    }

    byte key[] = { input[offset], input[offset + 1], input[offset + 2], input[offset + 3] };
    offset += 4;

    if (mask) {
        for (int i = 0; i < payloadLength; i++) {
            input[offset + i] = (input[offset + i] ^ key[i % 4]);
        }
        input[offset + payloadLength] = 0;
    }

    switch (opcode) {
    case OPCODE_CONTINUATION:
        Logger::warn("websocket: continuation frames not supported");
        break;
    case OPCODE_TEXT: {
        char *text = input + offset;
        bool flag = (strstr(text, "true") ? true : false);
        if (strstr(text, "stopCharge")) {
            status.setSystemState(Status::charged);
        } else if (strstr(text, "cmdRegen:")) {
            status.enableRegen = flag;
            Logger::info("Regen is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "cmdCreep:")) {
            status.enableCreep = flag;
            Logger::info("Creep is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "cmdEhps:")) {
            systemIO.setPowerSteering(flag);
            Logger::info("EHPS is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "cmdHeater:")) {
            systemIO.setEnableHeater(flag);
            systemIO.setHeaterPump(flag);
            Logger::info("Heater is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "cmdChargeLevel:")) {
            uint8_t inputCurrent = atoi(text);
            Logger::info("Setting charge level to %d A", inputCurrent);
            deviceManager.getCharger()->setMaximumInputCurrent(inputCurrent * 10);
        }
        break;
    }
    case OPCODE_BINARY:
        Logger::warn("websocket: binary frames not supported");
        break;
    case OPCODE_CLOSE:
        Logger::info("websocket: close connection request");
        connected = false;
        return Constants::disconnect;
        break;
    case OPCODE_PING:
        Logger::warn("websocket: ping not supported: %d,%d,%d,%d,%d,%d: %s", input[0], input[1], input[2], input[3], input[4], input[5], input);
        break;
    case OPCODE_PONG:
        break;
    }
    return "";
}

/**
 * \brief Prepare a JSON object which contains only changed values which are sent to the client
 *
 * \return the encoded
 */
String WebSocket::generateUpdate()
{
    MotorController* motorController = deviceManager.getMotorController();
    BatteryManager* batteryManager = deviceManager.getBatteryManager();
    uint32_t ms = millis();
    data = String();
    limits = String();
    isFirst = true;

    if (motorController) {
        processParameter(&paramCache.throttle, motorController->getThrottleLevel(), Constants::throttle, 10);
        processParameter(&paramCache.torqueActual, motorController->getTorqueActual(), Constants::torqueActual, 10);
        processParameter((int16_t *) &paramCache.gear, (int16_t) motorController->getGear(), Constants::gear);
        if (updateCounter == 0 || updateCounter == 5) { // very fluctuating values which would unnecessarily strain the cpu (of a tablet)
            processParameter(&paramCache.speedActual, motorController->getSpeedActual(), Constants::speedActual);
            if (batteryManager && batteryManager->hasPackVoltage()) {
                processParameter(&paramCache.dcVoltage, batteryManager->getPackVoltage(), Constants::dcVoltage, 10);
            } else {
                processParameter(&paramCache.dcVoltage, motorController->getDcVoltage(), Constants::dcVoltage, 10);
                processLimits(&dcVoltageMin, &dcVoltageMax, motorController->getDcVoltage(), Constants::dcVoltage);
            }
            if (batteryManager && batteryManager->hasPackCurrent()) {
                processParameter(&paramCache.dcCurrent, batteryManager->getPackCurrent(), Constants::dcCurrent, 10);
            } else {
                processParameter(&paramCache.dcCurrent, motorController->getDcCurrent(), Constants::dcCurrent, 10);
                processLimits(&dcCurrentMin, &dcCurrentMax, motorController->getDcCurrent(), Constants::dcCurrent);
            }
            processParameter(&paramCache.temperatureMotor, motorController->getTemperatureMotor(), Constants::temperatureMotor, 10);
            processLimits(NULL, &temperatureMotorMax, motorController->getTemperatureMotor(), Constants::temperatureMotor);
            processParameter(&paramCache.temperatureController, motorController->getTemperatureController(), Constants::temperatureController, 10);
            processLimits(NULL, &temperatureControllerMax, motorController->getTemperatureController(), Constants::temperatureController);
        }
    }

    processParameter(&paramCache.bitfieldMotor, status.getBitFieldMotor(), Constants::bitfieldMotor);
    processParameter(&paramCache.bitfieldBms, status.getBitFieldBms(), Constants::bitfieldBms);
    processParameter(&paramCache.bitfieldIO, status.getBitFieldIO(), Constants::bitfieldIO);

    if (ms > paramCache.timeRunning + 900) { // just update this every second or so
        paramCache.timeRunning = ms;
        addParam(Constants::timeRunning, getTimeRunning(), false);
        processParameter(&paramCache.systemState, (int16_t) status.getSystemState(), Constants::systemState);

        if (batteryManager) {
            if (batteryManager->hasSoc())
                processParameter(&paramCache.soc, (uint16_t)(batteryManager->getSoc() * 50), Constants::soc, 100);
            if (batteryManager->hasDischargeLimit()) {
                processParameter(&paramCache.dischargeLimit, batteryManager->getDischargeLimit(), Constants::dischargeLimit);
                if (batteryManager->getDischargeLimit() != dcCurrentMin || batteryManager->getChargeLimit() != dcCurrentMax) {
                    dcCurrentMax = batteryManager->getDischargeLimit();
                    dcCurrentMin = batteryManager->getChargeLimit() * -1;
                    addLimit((char *)String(dcCurrentMin).c_str(), (char *)String(dcCurrentMax).c_str(), Constants::dcCurrent);
                }
            }
            else
                processParameter(&paramCache.dischargeAllowed, batteryManager->isDischargeAllowed(), Constants::dischargeAllowed);
            if (batteryManager->hasChargeLimit())
                processParameter(&paramCache.chargeLimit, batteryManager->getChargeLimit(), Constants::chargeLimit);
            else
                processParameter(&paramCache.chargeAllowed, batteryManager->isChargeAllowed(), Constants::chargeAllowed);
            if (batteryManager->hasCellTemperatures()) {
                processParameter(&paramCache.lowestCellTemp, batteryManager->getLowestCellTemp(), Constants::lowestCellTemp, 10);
                processParameter(&paramCache.highestCellTemp, batteryManager->getHighestCellTemp(), Constants::highestCellTemp, 10);
                processParameter(&paramCache.lowestCellTempId, batteryManager->getLowestCellTempId(), Constants::lowestCellTempId);
                processParameter(&paramCache.highestCellTempId, batteryManager->getHighestCellTempId(), Constants::highestCellTempId);
            }
            if (batteryManager->hasCellVoltages()) {
                processParameter(&paramCache.lowestCellVolts, batteryManager->getLowestCellVolts(), Constants::lowestCellVolts, 10000);
                processParameter(&paramCache.highestCellVolts, batteryManager->getHighestCellVolts(), Constants::highestCellVolts, 10000);
                processParameter(&paramCache.averageCellVolts, batteryManager->getAverageCellVolts(), Constants::averageCellVolts, 10000);
                processParameter(&paramCache.deltaCellVolts, batteryManager->getHighestCellVolts() - batteryManager->getLowestCellVolts(), Constants::deltaCellVolts, 10000);
                processParameter(&paramCache.lowestCellVoltsId, batteryManager->getLowestCellVoltsId(), Constants::lowestCellVoltsId);
                processParameter(&paramCache.highestCellVoltsId, batteryManager->getHighestCellVoltsId(), Constants::highestCellVoltsId);
            }
            if (batteryManager->hasCellResistance()) {
                processParameter(&paramCache.lowestCellResistance, batteryManager->getLowestCellResistance(), Constants::lowestCellResistance, 100);
                processParameter(&paramCache.highestCellResistance, batteryManager->getHighestCellResistance(), Constants::highestCellResistance, 100);
                processParameter(&paramCache.averageCellResistance, batteryManager->getAverageCellResistance(), Constants::averageCellResistance, 100);
                processParameter(&paramCache.deltaCellResistance, (batteryManager->getHighestCellResistance() - batteryManager->getLowestCellResistance()), Constants::deltaCellResistance, 100);
                processParameter(&paramCache.lowestCellResistanceId, batteryManager->getLowestCellResistanceId(), Constants::lowestCellResistanceId);
                processParameter(&paramCache.highestCellResistanceId, batteryManager->getHighestCellResistanceId(), Constants::highestCellResistanceId);
            }
            if (batteryManager->hasPackResistance()) {
                processParameter(&paramCache.packResistance, batteryManager->getPackResistance(), Constants::packResistance);
            }
            if (batteryManager->hasPackHealth()) {
                processParameter(&paramCache.packHealth, batteryManager->getPackHealth(), Constants::packHealth);
            }
            if (batteryManager->hasPackCycles()) {
                processParameter(&paramCache.packCycles, batteryManager->getPackCycles(), Constants::packCycles);
            }
        }

        DcDcConverter* dcDcConverter = deviceManager.getDcDcConverter();
        if (dcDcConverter) {
            processParameter(&paramCache.dcDcHvVoltage, dcDcConverter->getHvVoltage(), Constants::dcDcHvVoltage, 10);
            processParameter(&paramCache.dcDcHvCurrent, dcDcConverter->getHvCurrent(), Constants::dcDcHvCurrent, 10);
            processParameter(&paramCache.dcDcLvVoltage, dcDcConverter->getLvVoltage(), Constants::dcDcLvVoltage, 10);
            processParameter(&paramCache.dcDcLvCurrent, dcDcConverter->getLvCurrent(), Constants::dcDcLvCurrent);
            processParameter(&paramCache.dcDcTemperature, dcDcConverter->getTemperature(), Constants::dcDcTemperature, 10);
        }

        if (status.getSystemState() == Status::charging || status.getSystemState() == Status::charged) {
            Charger* charger = deviceManager.getCharger();
            if (charger) {
                processParameter(&paramCache.chargerInputVoltage, charger->getInputVoltage(), Constants::chargerInputVoltage, 10);
                processParameter(&paramCache.chargerInputCurrent, charger->getInputCurrent(), Constants::chargerInputCurrent, 100);
                processParameter(&paramCache.chargerBatteryVoltage, charger->getBatteryVoltage(), Constants::chargerBatteryVoltage, 10);
                processParameter(&paramCache.chargerBatteryCurrent, charger->getBatteryCurrent(), Constants::chargerBatteryCurrent, 100);
                processParameter(&paramCache.chargerTemperature, charger->getTemperature(), Constants::chargerTemperature, 10);

                uint16_t secs = millis() / 1000; //TODO calc mins
                processParameter(&paramCache.chargeHoursRemain, secs / 60, Constants::chargeHoursRemain);
                processParameter(&paramCache.chargeMinsRemain, secs % 60, Constants::chargeMinsRemain);
                if (batteryManager && batteryManager->hasSoc())
                    processParameter(&paramCache.chargeLevel, batteryManager->getSoc() * 50, Constants::chargeLevel, 100);
                else
                    processParameter(&paramCache.chargeLevel, map (secs, 0 , 28800, 0, 100), Constants::chargeLevel);
            }
        }

        processParameter(&paramCache.heaterPower, status.heaterPower, Constants::heaterPower);
        processParameter(&paramCache.flowCoolant, status.flowCoolant * 6, Constants::flowCoolant, 100);
        processParameter(&paramCache.flowHeater, status.flowHeater * 6, Constants::flowHeater, 100);
        for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
            processParameter(&paramCache.temperatureBattery[i], status.temperatureBattery[i], Constants::temperatureBattery[i], 10);
        }
        processParameter(&paramCache.temperatureCoolant, status.temperatureCoolant, Constants::temperatureCoolant, 10);
        processParameter(&paramCache.temperatureHeater, status.heaterTemperature, Constants::temperatureHeater);
        if (status.temperatureExterior != CFG_NO_TEMPERATURE_DATA) {
            processParameter(&paramCache.temperatureExterior, status.temperatureExterior, Constants::temperatureExterior, 10);
        }
        processParameter(&paramCache.enableRegen, status.enableRegen, Constants::enableRegen);
        processParameter(&paramCache.enableHeater, status.enableHeater, Constants::enableHeater);
        processParameter(&paramCache.powerSteering, status.powerSteering, Constants::powerSteering);
        processParameter(&paramCache.enableCreep, status.enableCreep, Constants::enableCreep);

        if (limits.length() > 0) {
            String out;
            out.concat("{");
            out.concat(limits.substring(0, limits.length() - 1));
            out.concat("}");
            addParam("limits", (char *)out.c_str(), true);
        }
    }

    if (++updateCounter > 9) {
        updateCounter = 0;
    }

    if (isFirst) {    // return empty string -> nothing will be sent, lower resource usage
        return data;
    }
    data.concat("\r}\r"); // close JSON object

    return prepareWebSocketFrame(OPCODE_TEXT, data);
}

/**
 * \brief Prepare JSON message for log message
 *
 * \param logLevel the level of the log entry
 * \param deviceName name of the device which created the log entry
 * \param message the log message
 * \return the prepared log message which can be sent to the socket
 *
 */
String WebSocket::generateLogEntry(char *logLevel, char *deviceName, char *message)
{
    data = String();

    if (!connected) {
        return data;
    }
    data.concat("{\"logMessage\": {\"level\": \"");
    data.concat(logLevel);
    data.concat("\",\"message\": \"");
    if (deviceName != NULL) {
        data.concat(deviceName);
        data.concat(": ");
    }
    data.concat(message);
    data.concat("\"}}");

    return prepareWebSocketFrame(OPCODE_TEXT, data);
}

/**
 * \brief Wrap data into a web socket frame with the necessary header information (see Rfc 6455)
 *
 * \param opcode the opcode of the message (usually OPCODE_TEXT or OPCODE_BINARY)
 * \param data the date to be encapsulated
 * \return the web socket frame to be sent to the client
 */
String WebSocket::prepareWebSocketFrame(uint8_t opcode, String data)
{
    String frame = String();

    frame.concat((char) (0b10000000 | opcode)); // FIN and opcode
    if (data.length() < 126) {
        frame.concat((char) (data.length() & 0x7f)); // mask = 0, length in one byte
    } else if (data.length() < 0xffff) {
        frame.concat((char) 0x7e); // mask = 0, length in following two bytes

        // a dirty trick to prevent 0x00 bytes in the response string
        while ((data.length() >> 8) == 0 || (data.length() & 0xff) == 0) {
            data.concat(" ");
        }

        frame.concat((char) (data.length() >> 8)); // write high byte of length
        frame.concat((char) (data.length() & 0xff)); // write low byte of length
    } else {
        // we won't send frames > 64k
    }
    frame.concat(data);
    return frame;
}

/**
 * \brief Add a parameter to the JSON data object
 *
 * It's important that the last entry has no trailing ',' sign and the strings
 * are encapsulated with "" - otherwise the JSON parser will fail.
 *
 * \param key the name of the value to be added
 * \param value the value to be added
 * \param isNumeric an indication if the value is to be treated as numeric or string
 */
void WebSocket::addParam(const char* key, char *value, bool isNumeric)
{
    if (isFirst) {
        data.concat("{\r"); // open JSON object
        isFirst = false;
    } else {
        data.concat(",");
    }
    data.concat("\r\t\"");
    data.concat(key);
    data.concat("\": ");
    if (!isNumeric) {
        data.concat("\"");
    }
    data.concat(value);
    if (!isNumeric) {
        data.concat("\"");
    }
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(uint8_t *cacheParam, uint8_t value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    sprintf(buffer, "%u", value);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(int16_t *cacheParam, int16_t value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    sprintf(buffer, "%d", value);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(uint16_t *cacheParam, uint16_t value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    sprintf(buffer, "%u", value);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(int32_t *cacheParam, int32_t value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    sprintf(buffer, "%ld", value);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(uint32_t *cacheParam, uint32_t value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    sprintf(buffer, "%lu", value);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processParameter(int16_t *cacheParam, int16_t value, const char *name, int divisor)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    char format[10];
    sprintf(format, "%%.%df", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processParameter(uint16_t *cacheParam, uint16_t value, const char *name, int divisor)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    char format[10];
    sprintf(format, "%%.%df", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processParameter(int32_t *cacheParam, int32_t value, const char *name, int divisor)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    char format[10];
    sprintf(format, "%%.%df", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processParameter(uint32_t *cacheParam, uint32_t value, const char *name, int divisor)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;

    char format[10];
    sprintf(format, "%%.%df", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addParam(name, buffer, true);
}

/**
 * \brief Add a parameter to the response data if it has changed against the cached parameter
 *
 * \param cacheParam a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the parameter
 */
void WebSocket::processParameter(bool *cacheParam, bool value, const char *name)
{
    if (*cacheParam == value)
        return;
    *cacheParam = value;
    strcpy(buffer, (value ? "true" : "false"));
    addParam(name, buffer, true);
}

void WebSocket::processLimits(int16_t *min, int16_t *max, int16_t value, const char *name) {
    if (min && *min > value) {
        *min = value;
        sprintf(buffer, "%d", value);
        addLimit(buffer, NULL, name);
    }
    if (max && value > *max) {
        *max = value;
        sprintf(buffer, "%d", value);
        addLimit(NULL, buffer, name);
    }
}

void WebSocket::processLimits(uint16_t *min, uint16_t *max, uint16_t value, const char *name) {
    if (min && *min > value) {
        *min = value;
        sprintf(buffer, "%u", value);
        addLimit(buffer, NULL, name);
    }
    if (max && value > *max) {
        *max = value;
        sprintf(buffer, "%u", value);
        addLimit(NULL, buffer, name);
    }
}

void WebSocket::addLimit(char *min, char *max, const char *name) {
    if (!min && !max)
        return;

    limits.concat("\"");
    limits.concat(name);
    limits.concat("\": {");
    if (min) {
        limits.concat("\"min\": ");
        limits.concat(min);
        if (max)
            limits.concat(",");
    }
    if (max) {
        limits.concat("\"max\": ");
        limits.concat(max);
    }
    limits.concat("},");
}

/**
 * \brief Calculate the runtime in hh:mm:ss
 *
 * This runtime calculation is good for about 50 days of uptime.
 * Of course, the sprintf is only good to 99 hours so that's a bit less time.
 *
 * \return the current time running in hh:mm:ss
 */
char *WebSocket::getTimeRunning()
{
    uint32_t ms = millis();
    int seconds = (int) (ms / 1000) % 60;
    int minutes = (int) ((ms / (1000 * 60)) % 60);
    int hours = (int) ((ms / (1000 * 3600)) % 24);
    sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
    return buffer;
}
