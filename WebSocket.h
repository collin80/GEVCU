/*
 * WebSocket.h
 *
 *  Created on: 29 Mar 2016
 *      Author: michael
 */

#ifndef WEBSOCKET_H_
#define WEBSOCKET_H_

#include "SocketProcessor.h"
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
    int16_t torqueActual;
//    int16_t throttle;
    int16_t speedActual;
    uint16_t dcVoltage;
    int16_t dcCurrent;
    int16_t acCurrent;
    uint16_t energyConsumption;
    uint32_t bitfield1;
    uint32_t bitfield2;
    uint32_t bitfield3;
    uint16_t systemState;
    MotorController::Gears gear;
    int16_t temperatureMotor;
    int16_t temperatureController;
//    int32_t mechanicalPower;
    uint16_t dcDcHvVoltage;
    uint16_t dcDcLvVoltage;
    int16_t dcDcHvCurrent;
    int16_t dcDcLvCurrent;
    int16_t dcDcTemperature;
    uint16_t chargerInputVoltage;
    uint16_t chargerInputCurrent;
    uint16_t chargerBatteryVoltage;
    uint16_t chargerBatteryCurrent;
    int16_t chargerTemperature;
    uint32_t flowCoolant;
    uint32_t flowHeater;
    uint16_t heaterPower;
    int16_t temperatureBattery[CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS];
    int16_t temperatureCoolant;
    int16_t temperatureHeater;
    int16_t temperatureExterior;
    bool powerSteering;
    bool enableRegen;
    bool enableHeater;
    bool enableCreep;
};

class WebSocket : public SocketProcessor {
public:
    WebSocket();
    String generateUpdate();
    String generateLogEntry(char *logLevel, char *deviceName, char *message);
    String processInput(char *input);

private:
    enum { OPCODE_CONTINUATION = 0x0, OPCODE_TEXT = 0x1, OPCODE_BINARY = 0x2, OPCODE_CLOSE = 0x8, OPCODE_PING = 0x9, OPCODE_PONG = 0xa };
    ParamCache paramCache;
    bool isFirst;
    char buffer[30];
    String *webSocketKey;
    String data;
    bool connected;

    void processConnectionRequest(char *input);
    String prepareConnectionRequestResponse();
    String processData(char *input);
    String prepareWebSocketFrame(uint8_t opcode, String data);
    String getResponseKey(String key);
    void processParameter(int16_t *cacheParam, int16_t value, const char *key);
    void processParameter(uint16_t *cacheParam, uint16_t value, const char *key);
    void processParameter(int32_t *cacheParam, int32_t value, const char *key);
    void processParameter(uint32_t *cacheParam, uint32_t value, const char *key);
    void processParameter(int16_t *cacheParam, int16_t value, const char *key, float divisor);
    void processParameter(uint16_t *cacheParam, uint16_t value, const char *key, float divisor);
    void processParameter(int32_t *cacheParam, int32_t value, const char *key, float divisor);
    void processParameter(uint32_t *cacheParam, uint32_t value, const char *key, float divisor);
    void processParameter(bool *cacheParam, bool value, const char *name);
    void addParam(const char *key, char *value, bool isNumeric);
    char *getTimeRunning();
    void initParamCache();
};



#endif /* WEBSOCKET_H_ */
