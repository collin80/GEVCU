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
String WebSocket::processCmd(char *cmd)
{
    String header = String();

    // grab the key
    char *key = strstr(cmd, webSocketKeyName);
    if (key != NULL) {
        webSocketKey = new String(key + strlen(webSocketKeyName) + 2);
        Logger::debug("websocket: found key: %s", webSocketKey->c_str());
    }

    // they're done (empty line), send our header
    if (webSocketKey != NULL && webSocketKey->length() > 0 && strlen(cmd) == 0) {
        char acceptHash[128];

        Logger::debug("websocket: got a key and an empty line, let's go");
        webSocketKey->concat(websocketUid); // append the UID to the key

        // generate SHA1 hash of new key
        Sha1.init();
        Sha1.print(webSocketKey->c_str());
        uint8_t *hash = Sha1.result();

        // base64 encode the hash of the new key
        base64_encode(acceptHash, (char*)hash, 20);

        header.concat("HTTP/1.1 101 Switching Protocols\r\n");
        header.concat("Upgrade: websocket\r\n");
        header.concat("Connection: Upgrade\r\n");
        header.concat("Sec-WebSocket-Accept: ");
        header.concat(acceptHash);
        header.concat("\r\n\r\n");

        connected = true;
        webSocketKey = NULL;
    }
    return header;
}

/**
 * prepare a JSON object which contains only changed values
 *
 */
String WebSocket::getUpdate()
{
    MotorController* motorController = deviceManager.getMotorController();
    Throttle *accelerator = deviceManager.getAccelerator();
    Throttle *brake = deviceManager.getBrake();
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
        if (paramCache.kiloWattHours != motorController->getKiloWattHours() / 3600000) {
            paramCache.kiloWattHours = motorController->getKiloWattHours() / 3600000;
            addParam(data, Constants::kiloWattHours, paramCache.kiloWattHours / 10.0f, 1);
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
    }
    if (accelerator) {
        if (paramCache.throttle != accelerator->getLevel()) {
            paramCache.throttle = accelerator->getLevel();
            addParam(data, Constants::throttle, paramCache.throttle / 10.0f, 1);
        }
    }
    if (brake) {
        if (paramCache.brake != brake->getLevel()) {
            paramCache.brake = brake->getLevel();
            addParam(data, Constants::brake, paramCache.brake / -10.0f, 1); // divide by negative to get positive values for breaking
        }
    }

    if (paramCache.systemState != status.getSystemState()) {
        paramCache.systemState = status.getSystemState();
        addParam(data, Constants::systemState, status.systemStateToStr(status.getSystemState()), false);
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
    }

    // return empty string -> nothing will be sent, lower resource usage
    if (isFirst) {
        return data;
    }

    data.concat("\r}\r"); // close JSON object
    return prepareWebSocketFrame(data);
}

/**
 * Wrap data into a web socket frame with the necessary header information (see Rfc 6455)
 */
String WebSocket::prepareWebSocketFrame(String data)
{
    String frame = String();

    frame.concat((char)0b10000001); // FIN and opcode = 0x1
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
        // TODO this won't work yet because of the 0x00 -- but do we really need more than 64k of data ?
        frame.concat((char)0x7f); // mask = 0, length in following 8 bytes
        frame.concat((char)(data.length() >> 56));
        frame.concat((char)((data.length() >> 48) & 0xff));
        frame.concat((char)((data.length() >> 40) & 0xff));
        frame.concat((char)((data.length() >> 32) & 0xff));
        frame.concat((char)((data.length() >> 24) & 0xff));
        frame.concat((char)((data.length() >> 16) & 0xff));
        frame.concat((char)((data.length() >> 8) & 0xff));
        frame.concat((char)(data.length() & 0xff));
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

