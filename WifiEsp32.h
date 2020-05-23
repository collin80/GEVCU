/*
 * Esp32Wifi.h
 *
 * Class to interface with the ESP32 based wifi adapter
 *
 * Created: 3/31/2020
 *  Author: Michael Neuweiler

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

#ifndef WIFIESP32_H_
#define WIFIESP32_H_

#include <Arduino.h>

#include "ValueCache.h"
#include "Wifi.h"
#include "ValueCache.h"

class WifiEsp32: public Wifi
{
public:
    WifiEsp32();
    void setup(); //initialization on start up
    void tearDown();
    void handleTick(); //periodic processes
    void handleMessage(uint32_t messageType, void *message);
    void handleStateChange(Status::SystemState, Status::SystemState);
    DeviceType getType();
    DeviceId getId();
    void process();

    enum DataPointCode // must match with DataPoints in ESP32Web's GevcuAdapter
    {
        systemState = 0,
        torqueActual = 1,
        speedActual = 2,
        throttle = 3,
        torqueAvailable = 4,

        dcVoltage = 10,
        dcCurrent = 11,
        acCurrent = 12,
        temperatureMotor = 13,
        temperatureController = 14,
        mechanicalPower = 15,

        bitfieldMotor = 20,
        bitfieldBms = 21,
        bitfieldIO = 22,
        dcCurrentMin = 23,
        dcCurrentMax = 24,
        dcVoltageMin = 25,
        dcVoltageMax = 26,
        tempMotorMax = 27,
        tempControllerMax = 28,

        dcDcHvVoltage = 30,
        dcDcLvVoltage = 31,
        dcDcHvCurrent = 32,
        dcDcLvCurrent = 33,
        dcDcTemperature = 34,

        chargerInputVoltage = 40,
        chargerInputCurrent = 41,
        chargerBatteryVoltage = 42,
        chargerBatteryCurrent = 43,
        chargerTemperature = 44,
        maximumSolarCurrent = 45,
        chargeHoursRemain = 46,
        chargeMinsRemain = 47,
        chargeLevel = 48,

        flowCoolant = 50,
        flowHeater = 51,
        heaterPower = 52,
        temperatureBattery1 = 53,
        temperatureBattery2 = 54,
        temperatureBattery3 = 55,
        temperatureBattery4 = 56,
        temperatureBattery5 = 57,
        temperatureBattery6 = 58,
        temperatureCoolant = 59,
        temperatureHeater = 60,
        temperatureExterior = 61,

        powerSteering = 70,
        enableRegen = 71,
        enableHeater = 72,
        enableCreep = 73,
        cruiseControlSpeed = 74,
        enableCruiseControl = 75,

        soc = 80,
        dischargeLimit = 81,
        chargeLimit = 82,
        chargeAllowed = 83,
        dischargeAllowed = 84,
        lowestCellTemp = 85,
        highestCellTemp = 86,
        lowestCellVolts = 87,
        highestCellVolts = 88,
        averageCellVolts = 89,
        deltaCellVolts = 90,
        lowestCellResistance = 91,
        highestCellResistance = 92,
        averageCellResistance = 93,
        deltaCellResistance = 94,
        lowestCellTempId = 95,
        highestCellTempId = 96,
        lowestCellVoltsId = 97,
        highestCellVoltsId = 98,
        lowestCellResistanceId = 99,
        highestCellResistanceId = 100,
        packResistance = 101,
        packHealth = 102,
        packCycles = 103,
        bmsTemperature = 104
    };

private:
    void requestNextParam(); //get next changed parameter
    void requestParamValue(String paramName);  //try to retrieve the value of the given parameter
    void setParam(String paramName, String value);  //set the given parameter with the given string
    void sendLogMessage(String logLevel, String deviceName, String message);
    void sendCmd(String cmd);
    void sendSocketUpdate();
    void processStartSocketListenerRepsonse();
    void processActiveSocketListResponse();
    void processIncomingSocketData(String data);
    void processSocketSendResponse();
    void prepareMotorControllerData();
    void prepareDcDcConverterData();
    void prepareChargerData();
    void prepareSystemData();
    void prepareBatteryManagerData();
    void processValue(bool *cacheValue, bool value, DataPointCode code);
    void processValue(uint8_t *cacheValue, uint8_t value, DataPointCode code);
    void processValue(uint16_t *cacheValue, uint16_t value, DataPointCode code);
    void processValue(int16_t *cacheValue, int16_t value, DataPointCode code);
    void processValue(uint32_t *cacheValue, uint32_t value, DataPointCode code);
    void processLimits(uint16_t *cacheValue, uint16_t value, DataPointCode code, boolean maximum);
    void processLimits(int16_t *cacheValue, int16_t value, DataPointCode code, boolean maximum);

    char inBuffer[CFG_WIFI_BUFFER_SIZE]; //storage for incoming data
    byte outBuffer[CFG_WIFI_BUFFER_SIZE]; // buffer to compose and send data to ESP32
    uint16_t inPos, outPos; //write position for inBuffer
    uint16_t dataPointCount; // how many data points were added to outBuffer;
    String sendBuffer[CFG_SERIAL_SEND_BUFFER_SIZE];
    int psWritePtr;
    int psReadPtr;
    bool didParamLoad;
    bool connected; // is a client connected via websocket ?
    uint32_t timeStarted;
    uint8_t updateCount;
    static const int DATA_POINT_START = 0xaa;
};

#endif
