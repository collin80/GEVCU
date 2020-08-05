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
#include "ValueCache.h"

class WebSocket: public SocketProcessor
{
public:
    WebSocket();
    String generateUpdate();
    String generateLogEntry(String logLevel, String deviceName, String message);
    String processInput(char *input);

private:
    void processConnectionRequest(char *input);
    String prepareConnectionRequestResponse();
    String processData(char *input);
    String prepareWebSocketFrame(uint8_t opcode, String data);
    String getResponseKey(String key);
    void processValue(uint8_t *cacheValue, uint8_t value, const String key);
    void processValue(int16_t *cacheValue, int16_t value, const String key);
    void processValue(uint16_t *cacheValue, uint16_t value, const String key);
    void processValue(int32_t *cacheValue, int32_t value, const String key);
    void processValue(uint32_t *cacheValue, uint32_t value, const String key);
    void processValue(int16_t *cacheValue, int16_t value, const String key, int divisor);
    void processValue(uint16_t *cacheValue, uint16_t value, const String key, int divisor);
    void processValue(int32_t *cacheValue, int32_t value, const String key, int divisor);
    void processValue(uint32_t *cacheValue, uint32_t value, const String key, int divisor);
    void processValue(bool *cacheValue, bool value, const String name);
    void addValue(const String key, char *value, bool isNumeric);
    void processLimits(int16_t *min, int16_t *max, int16_t value, const String key);
    void processLimits(uint16_t *min, uint16_t *max, uint16_t value, const String key);
    void addLimit(char *min, char *max, const String name);
    char* getTimeRunning();
    bool checkTime();

    enum
    {
        OPCODE_CONTINUATION = 0x0,
        OPCODE_TEXT = 0x1,
        OPCODE_BINARY = 0x2,
        OPCODE_CLOSE = 0x8,
        OPCODE_PING = 0x9,
        OPCODE_PONG = 0xa
    };
    bool isFirst;
    uint8_t updateCounter;
    char buffer[30];
    String *webSocketKey;
    String data;
    String limits;
    bool connected;
    // limits
    uint16_t dcVoltageMin;
    uint16_t dcVoltageMax;
    int16_t dcCurrentMin;
    int16_t dcCurrentMax;
    int16_t temperatureMotorMax;
    int16_t temperatureControllerMax;
    uint32_t timeStamp;

    const String disconnect = "_DISCONNECT_";

    // status + dashboard
    const String systemState = "systemState";
    const String timeRunning = "timeRunning";
    const String torqueActual = "torqueActual";
    const String speedActual = "speedActual";
    const String throttle = "throttle";

    const String dcVoltage = "dcVoltage";
    const String dcCurrent = "dcCurrent";
    const String acCurrent = "acCurrent";
    const String temperatureMotor = "temperatureMotor";
    const String temperatureController = "temperatureController";
    const String mechanicalPower = "mechanicalPower";

    const String bitfieldMotor = "bitfieldMotor";
    const String bitfieldBms = "bitfieldBms";
    const String bitfieldIO = "bitfieldIO";

    const String dcDcHvVoltage = "dcDcHvVoltage";
    const String dcDcLvVoltage = "dcDcLvVoltage";
    const String dcDcHvCurrent = "dcDcHvCurrent";
    const String dcDcLvCurrent = "dcDcLvCurrent";
    const String dcDcTemperature = "dcDcTemperature";

    const String chargerInputVoltage = "chargerInputVoltage";
    const String chargerInputCurrent = "chargerInputCurrent";
    const String chargerBatteryVoltage = "chargerBatteryVoltage";
    const String chargerBatteryCurrent = "chargerBatteryCurrent";
    const String chargerTemperature = "chargerTemperature";
    const String maximumSolarCurrent = "maximumSolarCurrent";
    const String chargeHoursRemain = "chargeHoursRemain";
    const String chargeMinsRemain = "chargeMinsRemain";
    const String chargeLevel = "chargeLevel";

    const String flowCoolant = "flowCoolant";
    const String flowHeater = "flowHeater";
    const String heaterPower = "heaterPower";
    const String temperatureBattery[6] = { "temperatureBattery1", "temperatureBattery2", "temperatureBattery3", "temperatureBattery4",
            "temperatureBattery5", "temperatureBattery6" };
    const String temperatureCoolant = "temperatureCoolant";
    const String temperatureHeater = "temperatureHeater";
    const String temperatureExterior = "temperatureExterior";

    const String powerSteering = "powerSteering";
    const String enableRegen = "enableRegen";
    const String enableHeater = "enableHeater";
    const String enableCreep = "enableCreep";
    const String cruiseControlSpeed = "cruiseSpeed";
    const String enableCruiseControl = "enableCruiseControl";

    const String soc = "soc";
    const String dischargeLimit = "dischargeLimit";
    const String chargeLimit = "chargeLimit";
    const String chargeAllowed = "chargeAllowed";
    const String dischargeAllowed = "dischargeAllowed";
    const String lowestCellTemp = "lowestCellTemp";
    const String highestCellTemp = "highestCellTemp";
    const String lowestCellVolts = "lowestCellVolts";
    const String highestCellVolts = "highestCellVolts";
    const String averageCellVolts = "averageCellVolts";
    const String deltaCellVolts = "deltaCellVolts";
    const String lowestCellResistance = "lowestCellResistance";
    const String highestCellResistance = "highestCellResistance";
    const String averageCellResistance = "averageCellResistance";
    const String deltaCellResistance = "deltaCellResistance";
    const String lowestCellTempId = "lowestCellTempId";
    const String highestCellTempId = "highestCellTempId";
    const String lowestCellVoltsId = "lowestCellVoltsId";
    const String highestCellVoltsId = "highestCellVoltsId";
    const String lowestCellResistanceId = "lowestCellResistanceId";
    const String highestCellResistanceId = "highestCellResistanceId";
    const String packResistance = "packResistance";
    const String packHealth = "packHealth";
    const String packCycles = "packCycles";
    const String bmsTemp = "bmsTemp";
};

#endif /* WEBSOCKET_H_ */
