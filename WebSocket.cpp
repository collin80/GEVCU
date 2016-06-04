/*
 * WebSocketProcessor.cpp
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

WebSocket::WebSocket()
{
    paramCache.timeRunning = 0;
    connected = false;
    webSocketKey = NULL;
    isFirst = true;
}

bool WebSocket::isConnected()
{
    return connected;
}

void WebSocket::disconnected()
{
    connected = false;
}

/*
 * Process requests from a web socket client
 */
void WebSocket::processInput(String &response, char *input)
{
    if (connected) {
        if (input[0] != 0) { // don't process empty strings
            processData(response, input);
        }
    } else {
        processHeader(response, input);
    }
}

void WebSocket::initParamCache()
{
    paramCache.timeRunning = 0;
    paramCache.torqueRequested = -1;
    paramCache.torqueActual = -1;
    paramCache.throttle = -1;
    paramCache.brake = -1;
    paramCache.speedActual = -1;
    paramCache.dcVoltage = -1;
    paramCache.dcCurrent = -1;
    paramCache.acCurrent = -1;
    paramCache.nominalVolt = -1;
    paramCache.energyConsumption = -1;
    paramCache.bitfield1 = 0;
    paramCache.bitfield2 = 0;
    paramCache.bitfield3 = 0;
    paramCache.systemState = 0;
    paramCache.gear = MotorController::ERROR;
    paramCache.temperatureMotor = -1;
    paramCache.temperatureController = -1;
    paramCache.mechanicalPower = -1;
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
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        paramCache.temperatureBattery[i] = CFG_NO_TEMPERATURE_DATA;
    }
    paramCache.temperatureCoolant = CFG_NO_TEMPERATURE_DATA;
    paramCache.temperatureHeater = CFG_NO_TEMPERATURE_DATA;
    paramCache.temperatureExterior = CFG_NO_TEMPERATURE_DATA;
}

void WebSocket::processHeader(String &response, char *input)
{
    // grab the key
    char* key = strstr(input, webSocketKeyName);
    if (key != NULL) {
        webSocketKey = new String(key + strlen(webSocketKeyName) + 2);
        Logger::debug("websocket: found key: %s", webSocketKey->c_str());
    }
    // they're done (empty line), send our response
    if (webSocketKey != NULL && strlen(input) == 0) {
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
        connected = true;
        webSocketKey = NULL;
        initParamCache();
    }
}

void WebSocket::processData(String &response, char *input)
{
    bool fin = (input[0] & 0x80) != 0;
    int opcode = (input[0] & 0x0f);
    bool mask = (input[1] & 0x80) != 0;
    long payloadLength = input[1] & 0x7f;
    String data = String();

    Logger::debug("websocket: fin: %B, opcode: %X, mask: %B, length: %d", fin, opcode, mask, payloadLength);

    uint8_t offset = 2;
    if (payloadLength == 0x7e) { // 126 -> use next two bytes as unsigned 16bit length of payload
        payloadLength = input[offset] << 8 + input[offset + 1];
        Logger::debug("websoextended 16-bit lenght: %d", payloadLength);
        offset += 2;
    }
    if (payloadLength == 0x7f) {
        Logger::error("websocket: >64k frames not supported");
        return;
    }

    byte key[] = { input[offset], input[offset + 1], input[offset + 2], input[offset + 3] };
    offset += 4;

    if (mask) {
        for (int i = 0; i < payloadLength; i++) {
            input[offset + i] = (input[offset + i] ^ key[i % 4]);
        }
        input[payloadLength] = 0;
    }

    switch (opcode) {
    case OPCODE_CONTINUATION:
        Logger::error("websocket: continuation frames not supported");
        break;
    case OPCODE_TEXT:
        Logger::info("text frame: '%s'", &input[offset]);

        //TODO move somewhere else
        if (!strcmp("stopCharge", &input[offset])) {
            status.setSystemState(Status::charged);
        }
        break;
    case OPCODE_BINARY:
        Logger::error("websocket: binary frames not supported");
        break;
    case OPCODE_CLOSE:
        Logger::error("websocket: close connection request");
        data.concat((char)(input[offset]));
        data.concat((char)(input[offset + 1]));
        response.concat(prepareWebSocketFrame(OPCODE_CLOSE, data));
        connected = false; // signal to close the connection
        break;
    case OPCODE_PING:
        Logger::error("websocket: ping not supported");
        break;
    case OPCODE_PONG:
        break;
    }
}


/**
 * prepare a JSON object which contains only changed values
 *
 */
String WebSocket::getUpdate()
{
    MotorController* motorController = deviceManager.getMotorController();
    uint32_t ms = millis();
    String data = String();

    isFirst = true;

    if (motorController) {
        if (paramCache.speedActual != motorController->getSpeedActual()) {
            paramCache.speedActual = motorController->getSpeedActual();
            addParam(data, Constants::speedActual, paramCache.speedActual);
        }
        if (paramCache.torqueActual != motorController->getTorqueActual()) {
            paramCache.torqueActual = motorController->getTorqueActual();
            addParam(data, Constants::torqueActual, paramCache.torqueActual / 10.0f, 1);
        }
        if (paramCache.dcCurrent != motorController->getDcCurrent()) {
            paramCache.dcCurrent = motorController->getDcCurrent();
            addParam(data, Constants::dcCurrent, paramCache.dcCurrent / 10.0f, 1);
        }
        if (paramCache.torqueRequested != motorController->getTorqueRequested()) {
            paramCache.torqueRequested = motorController->getTorqueRequested();
            addParam(data, Constants::torqueRequested, paramCache.torqueRequested / 10.0f, 1);
        }
        if (paramCache.dcVoltage != motorController->getDcVoltage()) {
            paramCache.dcVoltage = motorController->getDcVoltage();
            addParam(data, Constants::dcVoltage, paramCache.dcVoltage / 10.0f, 1);
        }
        if (paramCache.energyConsumption != motorController->getEnergyConsumption()) {
            paramCache.energyConsumption = motorController->getEnergyConsumption();
            addParam(data, Constants::energyConsumption, paramCache.energyConsumption / 10.0f, 1);
        }
        if (paramCache.mechanicalPower != motorController->getMechanicalPower()) {
            paramCache.mechanicalPower = motorController->getMechanicalPower();
            addParam(data, Constants::mechanicalPower, paramCache.mechanicalPower / 10.0f, 1);
        }
        if (paramCache.temperatureMotor != motorController->getTemperatureMotor()) {
            paramCache.temperatureMotor = motorController->getTemperatureMotor();
            addParam(data, Constants::temperatureMotor, paramCache.temperatureMotor / 10.0f, 1);
        }
        if (paramCache.temperatureController != motorController->getTemperatureController()) {
            paramCache.temperatureController = motorController->getTemperatureController();
            addParam(data, Constants::temperatureController, paramCache.temperatureController / 10.0f, 1);
        }
        if (paramCache.gear != motorController->getGear()) {
            paramCache.gear = motorController->getGear();
            addParam(data, Constants::gear, (uint16_t) paramCache.gear);
        }
        if (paramCache.throttle != motorController->getThrottleLevel()) {
            paramCache.throttle = motorController->getThrottleLevel();
            addParam(data, Constants::throttle, paramCache.throttle / 10.0f, 1);
        }
    }

    if (paramCache.systemState != status.getSystemState()) {
        paramCache.systemState = status.getSystemState();
        addParam(data, Constants::systemState, status.getSystemState());
    }
    if (paramCache.bitfield1 != status.getBitField1()) {
        paramCache.bitfield1 = status.getBitField1();
        addParam(data, Constants::bitfield1, paramCache.bitfield1);
    }
    if (paramCache.bitfield2 != status.getBitField2()) {
        paramCache.bitfield2 = status.getBitField2();
        addParam(data, Constants::bitfield2, paramCache.bitfield2);
    }
    if (paramCache.bitfield3 != status.getBitField3()) {
        paramCache.bitfield3 = status.getBitField3();
        addParam(data, Constants::bitfield3, paramCache.bitfield3);
    }
    if (ms > paramCache.timeRunning + 900) { // just update this every second or so
        paramCache.timeRunning = ms;
        addParam(data, Constants::timeRunning, getTimeRunning(), false);

        DcDcConverter* dcDcConverter = deviceManager.getDcDcConverter();
        if (dcDcConverter) {
            if (paramCache.dcDcHvVoltage != dcDcConverter->getHvVoltage()) {
                paramCache.dcDcHvVoltage = dcDcConverter->getHvVoltage();
                addParam(data, Constants::dcDcHvVoltage, (uint16_t) paramCache.dcDcHvVoltage / 10.0f);
            }
            if (paramCache.dcDcHvCurrent != dcDcConverter->getHvCurrent()) {
                paramCache.dcDcHvCurrent = dcDcConverter->getHvCurrent();
                addParam(data, Constants::dcDcHvCurrent, (uint16_t) paramCache.dcDcHvCurrent / 10.0f);
            }
            if (paramCache.dcDcLvVoltage != dcDcConverter->getLvVoltage()) {
                paramCache.dcDcLvVoltage = dcDcConverter->getLvVoltage();
                addParam(data, Constants::dcDcLvVoltage, (uint16_t) paramCache.dcDcLvVoltage / 10.0f);
            }
            if (paramCache.dcDcLvCurrent != dcDcConverter->getLvCurrent()) {
                paramCache.dcDcLvCurrent = dcDcConverter->getLvCurrent();
                addParam(data, Constants::dcDcLvCurrent, (uint16_t) paramCache.dcDcLvCurrent / 10.0f);
            }
            if (paramCache.dcDcTemperature != dcDcConverter->getTemperature()) {
                paramCache.dcDcTemperature = dcDcConverter->getTemperature();
                addParam(data, Constants::dcDcTemperature, (uint16_t) paramCache.dcDcTemperature / 10.0f);
            }
        }

        Charger* charger = deviceManager.getCharger();
        if (charger) {
            if (paramCache.chargerInputVoltage != charger->getInputVoltage()) {
                paramCache.chargerInputVoltage = charger->getInputVoltage();
                addParam(data, Constants::chargerInputVoltage, (uint16_t) paramCache.chargerInputVoltage / 10.0f);
            }
            if (paramCache.chargerInputCurrent != charger->getInputCurrent()) {
                paramCache.chargerInputCurrent = charger->getInputCurrent();
                addParam(data, Constants::chargerInputCurrent, (uint16_t) paramCache.chargerInputCurrent / 100.0f);
            }
            if (paramCache.chargerBatteryVoltage != charger->getBatteryVoltage()) {
                paramCache.chargerBatteryVoltage = charger->getBatteryVoltage();
                addParam(data, Constants::chargerBatteryVoltage, (uint16_t) paramCache.chargerBatteryVoltage / 10.0f);
            }
            if (paramCache.chargerBatteryCurrent != charger->getBatteryCurrent()) {
                paramCache.chargerBatteryCurrent = charger->getBatteryCurrent();
                addParam(data, Constants::chargerBatteryCurrent, (uint16_t) paramCache.chargerBatteryCurrent / 100.0f);
            }
            if (paramCache.chargerTemperature != charger->getTemperature()) {
                paramCache.chargerTemperature = charger->getTemperature();
                addParam(data, Constants::chargerTemperature, (uint16_t) paramCache.chargerTemperature / 10.0f);
            }
        }
        if (paramCache.flowCoolant != status.flowCoolant) {
            paramCache.flowCoolant = status.flowCoolant;
            addParam(data, Constants::flowCoolant, (uint16_t) paramCache.flowCoolant * 6 / 100.0f);
        }
        if (paramCache.flowHeater != status.flowHeater) {
            paramCache.flowHeater = status.flowHeater;
            addParam(data, Constants::flowHeater, (uint16_t) paramCache.flowHeater * 6 / 100.0f);
        }
        for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
            if (paramCache.temperatureBattery[i] != status.temperatureBattery[i]) {
                paramCache.temperatureBattery[i] = status.temperatureBattery[i];
                addParam(data, Constants::temperatureBattery[i], (uint16_t) paramCache.temperatureBattery[i] / 10.0f);
            }
        }
        if (paramCache.temperatureCoolant != status.temperatureCoolant) {
            paramCache.temperatureCoolant = status.temperatureCoolant;
            addParam(data, Constants::temperatureCoolant, (uint16_t) paramCache.temperatureCoolant / 10.0f);
        }
        if (paramCache.temperatureHeater != status.temperatureHeater) {
            paramCache.temperatureHeater = status.temperatureHeater;
            addParam(data, Constants::temperatureHeater, (uint16_t) paramCache.temperatureHeater / 10.0f);
        }
        if (paramCache.temperatureExterior != status.temperatureExterior) {
            paramCache.temperatureExterior = status.temperatureExterior;
            addParam(data, Constants::temperatureExterior, (uint16_t) paramCache.temperatureExterior / 10.0f);
        }

    }

    // return empty string -> nothing will be sent, lower resource usage
    if (isFirst) {
        return data;
    }

    data.concat("\r}\r"); // close JSON object
    return prepareWebSocketFrame(OPCODE_TEXT, data);
}

/**
 * Wrap data into a web socket frame with the necessary header information (see Rfc 6455)
 */
String WebSocket::prepareWebSocketFrame(uint8_t opcode, String data)
{
    String frame = String();

    frame.concat((char)(0b10000000 | opcode)); // FIN and opcode
    if (data.length() < 126) {
        frame.concat((char)(data.length() & 0x7f)); // mask = 0, length in one byte
    } else if (data.length() < 0xffff) {
        frame.concat((char)0x7e); // mask = 0, length in following two bytes

        // a dirty trick to prevent 0x00 bytes in the response string
        while ((data.length() >> 8) == 0 || (data.length() & 0xff) == 0) {
            data.concat(" ");
        }

        frame.concat((char)(data.length() >> 8)); // write high byte of length
        frame.concat((char)(data.length() & 0xff)); // write low byte of length
    } else {
        // we won't send frames > 64k
    }
    frame.concat(data);
    return frame;
}

/**
 * add a parameter to the JSON object
 * It's important that the last entry has no trailing ',' sign and the strings
 * are encapsulated with "" - otherwise the JSON parser will fail.
 */
void WebSocket::addParam(String &data, const char* key, char *value, bool isNumeric)
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

/*
 * Add a parameter for the given float value
 */
void WebSocket::addParam(String &data, const char *key, float value, int precision)
{
    char format[10];
    sprintf(format, "%%.%df", precision);
    sprintf(buffer, format, value);
    addParam(data, key, buffer, true);
}

/*
 * Add a parameter for the given uint32 value
 */
void WebSocket::addParam(String &data, const char *key, uint32_t value)
{
    sprintf(buffer, "%lu", value);
    addParam(data, key, buffer, true);
}

/*
 * Calculate the runtime in hh:mm:ss
 This runtime calculation is good for about 50 days of uptime.
 Of course, the sprintf is only good to 99 hours so that's a bit less time.
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

