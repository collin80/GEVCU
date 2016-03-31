/*
 * WebSocket.h
 *
 *  Created on: 29 Mar 2016
 *      Author: michael
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "config.h"
#include "MotorController.h"
#include "Throttle.h"
#include "Base64.h"
#include "Sha1.h"

/**
 * Cache of param values to avoid sending an update unless changed
 */
struct ParamCache {
    uint32_t timeRunning;
    int16_t torqueRequested;
    int16_t torqueActual;
    int16_t throttle;
    int16_t brake;
    int16_t speedActual;
    int16_t dcVoltage;
    int16_t dcCurrent;
    int16_t acCurrent;
    int16_t nominalVolt;
    int16_t kiloWattHours;
    uint32_t bitfield1;
    uint32_t bitfield2;
    uint32_t bitfield3;
    uint8_t systemState;
    MotorController::Gears gear;
    int16_t temperatureMotor;
    int16_t temperatureController;
    int16_t mechanicalPower;
};

class WebSocket {
public:
    WebSocket();
    void processInput(String &response, char *input);
    String getUpdate();
    bool isConnected();
    void disconnected();

private:
    enum { OPCODE_CONTINUATION = 0x0, OPCODE_TEXT = 0x1, OPCODE_BINARY = 0x2, OPCODE_CLOSE = 0x8, OPCODE_PING = 0x9, OPCODE_PONG = 0xa };
    ParamCache paramCache;
    bool connected;
    bool isFirst;
    char buffer[30];
    String *webSocketKey;

    void processHeader(String &response, char *input);
    void processData(String &response, char *input);
    String prepareWebSocketFrame(uint8_t opcode, String data);
    String getResponseKey(String key);
    void addParam(String &data, const char* key, char *value, bool isNumeric);
    void addParam(String &data, const char *key, float value, int precision);
    void addParam(String &data, const char *key, uint32_t value);
    char *getTimeRunning();
    void initParamCache();
};



#endif /* WEBSOCKET_H_ */
