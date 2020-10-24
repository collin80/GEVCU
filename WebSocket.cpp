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
static const String websocketUid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

/**
 * \brief Constructor
 *
 */
WebSocket::WebSocket()
{
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
    timeStamp = 0;
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
        logger.debug("websocket: found key: %s", webSocketKey->c_str());
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
        logger.debug("websocket: got a key and an empty line, let's go");
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
        valueCache.clear();
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

    logger.debug("websocket: fin: %#x, opcode: %#x, mask: %#x, length: %d", fin, opcode, mask, payloadLength);

    uint8_t offset = 2;
    if (payloadLength == 0x7e) { // 126 -> use next two bytes as unsigned 16bit length of payload
        payloadLength = (input[offset] << 8) + input[offset + 1];
        logger.debug("websocket: extended 16-bit length: %d", payloadLength);
        offset += 2;
    }
    if (payloadLength == 0x7f) {
        logger.warn("websocket: >64k frames not supported");
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
        logger.warn("websocket: continuation frames not supported");
        break;
    case OPCODE_TEXT: {
        char *text = input + offset;
        bool flag = (strstr(text, "true") ? true : false);
        char *equals = strchr(text, '=');
        int16_t value = (equals ? atol(equals + 1) : 0);
        if (strstr(text, "stopCharge")) {
            status.setSystemState(Status::charged);
        } else if (strstr(text, "cruiseToggle")) {
            deviceManager.getMotorController()->cruiseControlToggle();
        } else if (strstr(text, "cruise=")) {
            if (text[7] == '-' || text[7] == '+') {
                deviceManager.getMotorController()->cruiseControlAdjust(value);
            } else {
                deviceManager.getMotorController()->cruiseControlSetSpeed(value);
            }
        } else if (strstr(text, "regen=")) {
            status.enableRegen = flag;
            logger.debug("Regen is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "creep=")) {
            status.enableCreep = flag;
            logger.debug("Creep is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "ehps=")) {
            systemIO.setPowerSteering(flag);
            logger.debug("EHPS is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "heater=")) {
            systemIO.setEnableHeater(flag);
            systemIO.setHeaterPump(flag);
            logger.debug("Heater is now switched %s", (flag ? "on" : "off"));
        } else if (strstr(text, "chargeInput=")) {
            logger.debug("Setting charge level to %d Amps", value);
            deviceManager.getCharger()->overrideMaximumInputCurrent(value * 10);
        }
        break;
    }
    case OPCODE_BINARY:
        logger.warn("websocket: binary frames not supported");
        break;
    case OPCODE_CLOSE:
        logger.info("websocket: close connection request");
        connected = false;
        return disconnect;
        break;
    case OPCODE_PING:
        logger.warn("websocket: ping not supported: %d,%d,%d,%d,%d,%d: %s", input[0], input[1], input[2], input[3], input[4], input[5], input);
        break;
    case OPCODE_PONG:
        break;
    }
    return "";
}

bool WebSocket::checkTime() {
	//TODO with 115200 baud transferring 1kb requires 80ms. try higher baud rate with ichip
	return (data.length() < 250);
}

/**
 * \brief Prepare a JSON object which contains only changed values which are sent to the client
 *
 *  (Processing time between 0-3ms)
 *
 * \return the encoded
 */
String WebSocket::generateUpdate()
{
    MotorController* motorController = deviceManager.getMotorController();
    BatteryManager* batteryManager = deviceManager.getBatteryManager();
    timeStamp = millis(); // init for checkTime()
    data = String();
    limits = String();
    isFirst = true;

    if (motorController) {
        processValue(&valueCache.throttle, motorController->getThrottleLevel(), throttle, 10);
        processValue(&valueCache.torqueActual, motorController->getTorqueActual(), torqueActual, 10);
        if (updateCounter == 0 || updateCounter == 5) { // very fluctuating values which would unnecessarily strain the cpu (of a tablet)
            processValue(&valueCache.speedActual, motorController->getSpeedActual(), speedActual);
            if (batteryManager && batteryManager->hasPackVoltage()) {
                processValue(&valueCache.dcVoltage, batteryManager->getPackVoltage(), dcVoltage, 10);
            } else {
                processValue(&valueCache.dcVoltage, motorController->getDcVoltage(), dcVoltage, 10);
                processLimits(&dcVoltageMin, &dcVoltageMax, motorController->getDcVoltage(), dcVoltage);
            }
            if (batteryManager && batteryManager->hasPackCurrent()) {
                processValue(&valueCache.dcCurrent, batteryManager->getPackCurrent(), dcCurrent, 10);
            } else {
                processValue(&valueCache.dcCurrent, motorController->getDcCurrent(), dcCurrent, 10);
                processLimits(&dcCurrentMin, &dcCurrentMax, motorController->getDcCurrent(), dcCurrent);
            }
            processValue(&valueCache.temperatureMotor, motorController->getTemperatureMotor(), temperatureMotor, 10);
            processLimits(NULL, &temperatureMotorMax, motorController->getTemperatureMotor(), temperatureMotor);
            processValue(&valueCache.temperatureController, motorController->getTemperatureController(), temperatureController, 10);
            processLimits(NULL, &temperatureControllerMax, motorController->getTemperatureController(), temperatureController);
			processValue(&valueCache.cruiseControlSpeed, motorController->getCruiseControlSpeed(), cruiseControlSpeed);
			processValue(&valueCache.enableCruiseControl, motorController->isCruiseControlEnabled(), enableCruiseControl);
        }
    }

    if (checkTime()) {
		processValue(&valueCache.bitfieldMotor, status.getBitFieldMotor(), bitfieldMotor);
		processValue(&valueCache.bitfieldBms, status.getBitFieldBms(), bitfieldBms);
		processValue(&valueCache.bitfieldIO, status.getBitFieldIO(), bitfieldIO);
    }

    if (timeStamp > valueCache.timeRunning + 900) { // just update this every second or so
        valueCache.timeRunning = timeStamp;
        addValue(timeRunning, getTimeRunning(), false);
        processValue(&valueCache.systemState, (int16_t) status.getSystemState(), systemState);

        if (batteryManager && checkTime()) {
            if (batteryManager->hasSoc())
                processValue(&valueCache.soc, (uint16_t)(batteryManager->getSoc() * 50), soc, 100);
            if (batteryManager->hasDischargeLimit()) {
                processValue(&valueCache.dischargeLimit, batteryManager->getDischargeLimit(), dischargeLimit);
                if (batteryManager->getDischargeLimit() != dcCurrentMin || batteryManager->getChargeLimit() != dcCurrentMax) {
                    dcCurrentMax = batteryManager->getDischargeLimit();
                    dcCurrentMin = batteryManager->getChargeLimit() * -1;
                    addLimit((char *)String(dcCurrentMin).c_str(), (char *)String(dcCurrentMax).c_str(), dcCurrent);
                }
            } else {
                processValue(&valueCache.dischargeAllowed, batteryManager->isDischargeAllowed(), dischargeAllowed);
            }
            if (batteryManager->hasChargeLimit())
                processValue(&valueCache.chargeLimit, batteryManager->getChargeLimit(), chargeLimit);
            else
                processValue(&valueCache.chargeAllowed, batteryManager->isChargeAllowed(), chargeAllowed);
            if (batteryManager->hasCellTemperatures() && checkTime()) {
                processValue(&valueCache.lowestCellTemp, batteryManager->getLowestCellTemp(), lowestCellTemp, 10);
                processValue(&valueCache.highestCellTemp, batteryManager->getHighestCellTemp(), highestCellTemp, 10);
                processValue(&valueCache.lowestCellTempId, batteryManager->getLowestCellTempId(), lowestCellTempId);
                processValue(&valueCache.highestCellTempId, batteryManager->getHighestCellTempId(), highestCellTempId);
            }
            if (batteryManager->hasCellVoltages() && checkTime()) {
                processValue(&valueCache.lowestCellVolts, batteryManager->getLowestCellVolts(), lowestCellVolts, 10000);
                processValue(&valueCache.highestCellVolts, batteryManager->getHighestCellVolts(), highestCellVolts, 10000);
                processValue(&valueCache.averageCellVolts, batteryManager->getAverageCellVolts(), averageCellVolts, 10000);
                processValue(&valueCache.deltaCellVolts, batteryManager->getHighestCellVolts() - batteryManager->getLowestCellVolts(), deltaCellVolts, 10000);
                processValue(&valueCache.lowestCellVoltsId, batteryManager->getLowestCellVoltsId(), lowestCellVoltsId);
                processValue(&valueCache.highestCellVoltsId, batteryManager->getHighestCellVoltsId(), highestCellVoltsId);
            }
            if (batteryManager->hasCellResistance() && checkTime()) {
                processValue(&valueCache.lowestCellResistance, batteryManager->getLowestCellResistance(), lowestCellResistance, 100);
                processValue(&valueCache.highestCellResistance, batteryManager->getHighestCellResistance(), highestCellResistance, 100);
                processValue(&valueCache.averageCellResistance, batteryManager->getAverageCellResistance(), averageCellResistance, 100);
                processValue(&valueCache.deltaCellResistance, (batteryManager->getHighestCellResistance() - batteryManager->getLowestCellResistance()), deltaCellResistance, 100);
                processValue(&valueCache.lowestCellResistanceId, batteryManager->getLowestCellResistanceId(), lowestCellResistanceId);
                processValue(&valueCache.highestCellResistanceId, batteryManager->getHighestCellResistanceId(), highestCellResistanceId);
            }
            if (checkTime()) {
				if (batteryManager->hasPackResistance()) {
					processValue(&valueCache.packResistance, batteryManager->getPackResistance(), packResistance);
				}
				if (batteryManager->hasPackHealth()) {
					processValue(&valueCache.packHealth, batteryManager->getPackHealth(), packHealth);
				}
				if (batteryManager->hasPackCycles()) {
					processValue(&valueCache.packCycles, batteryManager->getPackCycles(), packCycles);
				}
            }
            processValue(&valueCache.bmsTemperature, batteryManager->getSystemTemperature(), bmsTemp);
        }

        DcDcConverter* dcDcConverter = deviceManager.getDcDcConverter();
        if (dcDcConverter && checkTime()) {
            processValue(&valueCache.dcDcHvVoltage, dcDcConverter->getHvVoltage(), dcDcHvVoltage, 10);
            processValue(&valueCache.dcDcHvCurrent, dcDcConverter->getHvCurrent(), dcDcHvCurrent, 10);
            processValue(&valueCache.dcDcLvVoltage, dcDcConverter->getLvVoltage(), dcDcLvVoltage, 10);
            processValue(&valueCache.dcDcLvCurrent, dcDcConverter->getLvCurrent(), dcDcLvCurrent);
            processValue(&valueCache.dcDcTemperature, dcDcConverter->getTemperature(), dcDcTemperature, 10);
        }

        if (status.getSystemState() == Status::charging || status.getSystemState() == Status::charged) {
            Charger* charger = deviceManager.getCharger();
            if (charger && checkTime()) {
                processValue(&valueCache.chargerInputVoltage, charger->getInputVoltage(), chargerInputVoltage, 10);
                processValue(&valueCache.chargerInputCurrent, charger->getInputCurrent(), chargerInputCurrent, 100);
                processValue(&valueCache.chargerBatteryVoltage, charger->getBatteryVoltage(), chargerBatteryVoltage, 10);
                processValue(&valueCache.chargerBatteryCurrent, charger->getBatteryCurrent(), chargerBatteryCurrent, 100);
                processValue(&valueCache.chargerTemperature, charger->getTemperature(), chargerTemperature, 10);

                uint16_t minutesRemaining = charger->calculateTimeRemaining();
                processValue(&valueCache.chargeHoursRemain, minutesRemaining / 60, chargeHoursRemain);
                processValue(&valueCache.chargeMinsRemain, minutesRemaining % 60, chargeMinsRemain);
                if (batteryManager && batteryManager->hasSoc())
                    processValue(&valueCache.chargeLevel, batteryManager->getSoc() * 50, chargeLevel, 100);
                processValue(&valueCache.maximumInputCurrent, charger->calculateMaximumInputCurrent(), maximumInputCurrent, 10);
            }
        }

        if (checkTime()) {
			processValue(&valueCache.heaterPower, status.heaterPower, heaterPower);
			processValue(&valueCache.flowCoolant, status.flowCoolant * 6, flowCoolant, 100);
			processValue(&valueCache.flowHeater, status.flowHeater * 6, flowHeater, 100);
			for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
				processValue(&valueCache.temperatureBattery[i], status.temperatureBattery[i], temperatureBattery[i], 10);
			}
			processValue(&valueCache.temperatureCoolant, status.temperatureCoolant, temperatureCoolant, 10);
			processValue(&valueCache.temperatureHeater, status.heaterTemperature, temperatureHeater);
			if (status.temperatureExterior != CFG_NO_TEMPERATURE_DATA) {
				processValue(&valueCache.temperatureExterior, status.temperatureExterior, temperatureExterior, 10);
			}
        }
        if (checkTime()) {
			processValue(&valueCache.enableRegen, status.enableRegen, enableRegen);
			processValue(&valueCache.enableHeater, status.enableHeater, enableHeater);
			processValue(&valueCache.powerSteering, status.powerSteering, powerSteering);
			processValue(&valueCache.enableCreep, status.enableCreep, enableCreep);
        }

        if (limits.length() > 0) {
            String out;
            out.concat("{");
            out.concat(limits.substring(0, limits.length() - 1));
            out.concat("}");
            addValue("limits", (char *)out.c_str(), true);
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
String WebSocket::generateLogEntry(String logLevel, String deviceName, String message)
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
 * \brief Add a value to the JSON data object
 *
 * It's important that the last entry has no trailing ',' sign and the strings
 * are encapsulated with "" - otherwise the JSON parser will fail.
 *
 * \param key the name of the value to be added
 * \param value the value to be added
 * \param isNumeric an indication if the value is to be treated as numeric or string
 */
void WebSocket::addValue(const String key, char *value, bool isNumeric)
{
    if (isFirst) {
        data.concat("{"); // open JSON object
        isFirst = false;
    } else {
        data.concat(",");
    }
    data.concat("\"");
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
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(uint8_t *cacheValue, uint8_t value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    sprintf(buffer, "%u", value);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(int16_t *cacheValue, int16_t value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    sprintf(buffer, "%d", value);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(uint16_t *cacheValue, uint16_t value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    sprintf(buffer, "%u", value);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(int32_t *cacheValue, int32_t value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    sprintf(buffer, "%ld", value);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(uint32_t *cacheValue, uint32_t value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    sprintf(buffer, "%lu", value);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processValue(int16_t *cacheValue, int16_t value, const String name, int divisor)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    char format[10];
    sprintf(format, "%%.%ldf", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processValue(uint16_t *cacheValue, uint16_t value, const String name, int divisor)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    char format[10];
    sprintf(format, "%%.%ldf", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processValue(int32_t *cacheValue, int32_t value, const String name, int divisor)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    char format[10];
    sprintf(format, "%%.%ldf", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 * \param divisor by which the value should be divided to a float value
 */
void WebSocket::processValue(uint32_t *cacheValue, uint32_t value, const String name, int divisor)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    char format[10];
    sprintf(format, "%%.%ldf", round(log10(divisor)));
    sprintf(buffer, format, static_cast<float>(value) / divisor);
    addValue(name, buffer, true);
}

/**
 * \brief Add a value to the response data if it has changed against the cached value
 *
 * \param cacheValue a pointer to the cached value against which the value is compared
 * \param value the current value to be analyzed / added
 * \param name the name of the value
 */
void WebSocket::processValue(bool *cacheValue, bool value, const String name)
{
    if (*cacheValue == value)
        return;
    *cacheValue = value;
    strcpy(buffer, (value ? "true" : "false"));
    addValue(name, buffer, true);
}

void WebSocket::processLimits(int16_t *min, int16_t *max, int16_t value, const String name) {
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

void WebSocket::processLimits(uint16_t *min, uint16_t *max, uint16_t value, const String name) {
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

void WebSocket::addLimit(char *min, char *max, const String name) {
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
