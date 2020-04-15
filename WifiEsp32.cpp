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
    connected = false;
    inPos = outPos = 0;
    timeStarted = 0;
    dataPointCount = 0;
    psWritePtr = psReadPtr = 0;

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
    connected = false;
    inPos = outPos = 0;
    timeStarted = 0;
    dataPointCount = 0;
    psWritePtr = psReadPtr = 0;

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
    if (logger.isDebug()) {
        logger.debug(this, "Send cmd: %s", cmd.c_str());
    }
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
    if (connected) {
        sendSocketUpdate();
    }

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
        String *params = (String *) message;
        sendLogMessage(params[0], params[1], params[2]);
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

        if (incoming == 13 || inPos > (CFG_WIFI_BUFFER_SIZE - 2)) {
            inBuffer[inPos] = 0;
            inPos = 0;

            if (logger.isDebug()) {
                logger.debug(this, "incoming: '%s'", inBuffer);
            }
            String input = inBuffer;
            if (input.startsWith("cfg:")) {
                processParameterChange(input.substring(4));
            } else if (input.startsWith("cmd:")) {
                processIncomingSocketData(input.substring(4));
            }

            return; // before processing the next line, return to the loop() to allow other devices to process.
        } else { // add more characters
            if (incoming != 10) { // don't add a LF character
                inBuffer[inPos++] = (char) incoming;
            }
        }
    }
}

/**
 * \brief Process incoming data from a socket
 */
void WifiEsp32::processIncomingSocketData(String input)
{
    logger.debug(this, "processing incoming socket data");

    int pos = input.indexOf('=');
    if (pos > 0) {
        String key = input.substring(0, pos);
        String value = input.substring(pos + 1);

        if (key.equals("cruise")) {
            int num = value.toInt();
            if (value.charAt(0) == '-' || value.charAt(0) == '+') {
                deviceManager.getMotorController()->cruiseControlAdjust(num);
            } else {
                deviceManager.getMotorController()->cruiseControlSetSpeed(num);
            }
        } else if (key.equals("regen")) {
            status.enableRegen = value.equals("true");
            logger.info("Regen is now switched %s", (status.enableRegen ? "on" : "off"));
        } else if (key.equals("creep")) {
            status.enableCreep = value.equals("true");;
            logger.info("Creep is now switched %s", (status.enableCreep ? "on" : "off"));
        } else if (key.equals("ehps")) {
            systemIO.setPowerSteering(value.equals("true"));
            logger.info("EHPS is now switched %s", (status.powerSteering ? "on" : "off"));
        } else if (key.equals("heater")) {
            bool flag = value.equals("true");
            systemIO.setEnableHeater(flag);
            systemIO.setHeaterPump(flag);
            logger.info("Heater is now switched %s", (status.enableHeater ? "on" : "off"));
        } else if (key.equals("chargeInput")) {
            logger.info("Setting charge level to %d Amps", value.toInt());
            deviceManager.getCharger()->setMaximumInputCurrent(value.toInt() * 10);
        }
    } else {
        if (input.equals("stopCharge")) {
            status.setSystemState(Status::charged);
        } else if (input.equals("cruiseToggle")) {
            deviceManager.getMotorController()->cruiseControlToggle();
        } else if (input.equals("connected")) {
            valueCache.clear(); // new connection -> clear the cache to have all values sent
            connected = true;
        } else if (input.equals("disconnected")) {
            connected = false;
        } else if (input.equals("loadConfig")) {
            didParamLoad = false;
        }
    }
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
 * \brief send log message as JSON
 *
 * \param logLevel the level of the log entry
 * \param deviceName name of the device which created the log entry
 * \param message the log message
 * \return the prepared log message which can be sent to the socket
 *
 */
void WifiEsp32::sendLogMessage(String logLevel, String deviceName, String message)
{
    String data = "json:{\"logMessage\": {\"level\": \"";
    data.concat(logLevel);
    data.concat("\",\"message\": \"");
    if (deviceName.length() > 0) {
        data.concat(deviceName);
        data.concat(": ");
    }
    data.concat(message);
    data.concat("\"}}");

    serialInterface->println(data);
}

/**
 * \brief Send update to all active sockets
 *
 * The message to be sent is created by the assigned SocketProcessor
 *
 */
void WifiEsp32::sendSocketUpdate()
{
    MotorController* motorController = deviceManager.getMotorController();
    DcDcConverter* dcDcConverter = deviceManager.getDcDcConverter();
    BatteryManager* batteryManager = deviceManager.getBatteryManager();
    outPos = 0;
    dataPointCount = 0;

    processValue(&valueCache.systemState, (uint8_t) status.getSystemState(), systemState);

    if (motorController) {
        processValue(&valueCache.torqueActual, motorController->getTorqueActual(), torqueActual);
        processValue(&valueCache.speedActual, motorController->getSpeedActual(), speedActual);
        processValue(&valueCache.throttle, motorController->getThrottleLevel(), throttle);
        if (batteryManager && batteryManager->hasPackVoltage()) {
            processValue(&valueCache.dcVoltage, batteryManager->getPackVoltage(), dcVoltage);
        } else {
            processValue(&valueCache.dcVoltage, motorController->getDcVoltage(), dcVoltage);
//            processLimits(&dcVoltageMin, &dcVoltageMax, motorController->getDcVoltage(), dcVoltage);
        }
        if (batteryManager && batteryManager->hasPackCurrent()) {
            processValue(&valueCache.dcCurrent, batteryManager->getPackCurrent(), dcCurrent);
        } else {
            processValue(&valueCache.dcCurrent, motorController->getDcCurrent(), dcCurrent);
//            processLimits(&dcCurrentMin, &dcCurrentMax, motorController->getDcCurrent(), dcCurrent);
        }
        processValue(&valueCache.temperatureMotor, motorController->getTemperatureMotor(), temperatureMotor);
//        processLimits(NULL, &temperatureMotorMax, motorController->getTemperatureMotor(), temperatureMotor);
        processValue(&valueCache.temperatureController, motorController->getTemperatureController(), temperatureController);
//        processLimits(NULL, &temperatureControllerMax, motorController->getTemperatureController(), temperatureController);
        processValue(&valueCache.mechanicalPower, motorController->getMechanicalPower(), mechanicalPower);
        processValue(&valueCache.cruiseControlSpeed, motorController->getCruiseControlSpeed(), cruiseControlSpeed);
        processValue(&valueCache.enableCruiseControl, motorController->isCruiseControlEnabled(), enableCruiseControl);
    }

    processValue(&valueCache.bitfieldMotor, status.getBitFieldMotor(), bitfieldMotor);
    processValue(&valueCache.bitfieldBms, status.getBitFieldBms(), bitfieldBms);
    processValue(&valueCache.bitfieldIO, status.getBitFieldIO(), bitfieldIO);

    if (dcDcConverter) {
        processValue(&valueCache.dcDcHvVoltage, dcDcConverter->getHvVoltage(), dcDcHvVoltage);
        processValue(&valueCache.dcDcHvCurrent, dcDcConverter->getHvCurrent(), dcDcHvCurrent);
        processValue(&valueCache.dcDcLvVoltage, dcDcConverter->getLvVoltage(), dcDcLvVoltage);
        processValue(&valueCache.dcDcLvCurrent, dcDcConverter->getLvCurrent(), dcDcLvCurrent);
        processValue(&valueCache.dcDcTemperature, dcDcConverter->getTemperature(), dcDcTemperature);
    }

    if (status.getSystemState() == Status::charging || status.getSystemState() == Status::charged) {
        Charger* charger = deviceManager.getCharger();
        if (charger) {
            processValue(&valueCache.chargerInputVoltage, charger->getInputVoltage(), chargerInputVoltage);
            processValue(&valueCache.chargerInputCurrent, charger->getInputCurrent(), chargerInputCurrent);
            processValue(&valueCache.chargerBatteryVoltage, charger->getBatteryVoltage(), chargerBatteryVoltage);
            processValue(&valueCache.chargerBatteryCurrent, charger->getBatteryCurrent(), chargerBatteryCurrent);
            processValue(&valueCache.chargerTemperature, charger->getTemperature(), chargerTemperature);
            processValue(&valueCache.maximumSolarCurrent, charger->getMaximumSolarCurrent(), maximumSolarCurrent);

            uint16_t secs = millis() / 1000; //TODO calc mins
            processValue(&valueCache.chargeHoursRemain, secs / 60, chargeHoursRemain);
            processValue(&valueCache.chargeMinsRemain, secs % 60, chargeMinsRemain);
            if (batteryManager && batteryManager->hasSoc())
                processValue(&valueCache.chargeLevel, batteryManager->getSoc() * 50, chargeLevel);
            else
                processValue(&valueCache.chargeLevel, map (secs, 0 , 28800, 0, 100), chargeLevel);
        }
    }

    processValue(&valueCache.flowCoolant, status.flowCoolant * 6, flowCoolant);
    processValue(&valueCache.flowHeater, status.flowHeater * 6, flowHeater);
    processValue(&valueCache.heaterPower, status.heaterPower, heaterPower);
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        processValue(&valueCache.temperatureBattery[i], status.temperatureBattery[i], (DataPointCode)(temperatureBattery1 + i));
    }
    processValue(&valueCache.temperatureCoolant, status.temperatureCoolant, temperatureCoolant);
    processValue(&valueCache.temperatureHeater, status.heaterTemperature, temperatureHeater);
    if (status.temperatureExterior != CFG_NO_TEMPERATURE_DATA) {
        processValue(&valueCache.temperatureExterior, status.temperatureExterior, temperatureExterior);
    }

    processValue(&valueCache.powerSteering, status.powerSteering, powerSteering);
    processValue(&valueCache.enableRegen, status.enableRegen, enableRegen);
    processValue(&valueCache.enableHeater, status.enableHeater, enableHeater);
    processValue(&valueCache.enableCreep, status.enableCreep, enableCreep);

    if (batteryManager) {
        if (batteryManager->hasSoc())
            processValue(&valueCache.soc, (uint16_t)(batteryManager->getSoc() * 50), soc);
        if (batteryManager->hasDischargeLimit()) {
            processValue(&valueCache.dischargeLimit, batteryManager->getDischargeLimit(), dischargeLimit);
//            if (batteryManager->getDischargeLimit() != dcCurrentMin || batteryManager->getChargeLimit() != dcCurrentMax) {
//                dcCurrentMax = batteryManager->getDischargeLimit();
//                dcCurrentMin = batteryManager->getChargeLimit() * -1;
//                addLimit((char *)String(dcCurrentMin).c_str(), (char *)String(dcCurrentMax).c_str(), dcCurrent);
//            }
        } else {
            processValue(&valueCache.dischargeAllowed, batteryManager->isDischargeAllowed(), dischargeAllowed);
        }
        if (batteryManager->hasChargeLimit())
            processValue(&valueCache.chargeLimit, batteryManager->getChargeLimit(), chargeLimit);
        else
            processValue(&valueCache.chargeAllowed, batteryManager->isChargeAllowed(), chargeAllowed);
        if (batteryManager->hasCellTemperatures()) {
            processValue(&valueCache.lowestCellTemp, batteryManager->getLowestCellTemp(), lowestCellTemp);
            processValue(&valueCache.highestCellTemp, batteryManager->getHighestCellTemp(), highestCellTemp);
            processValue(&valueCache.lowestCellTempId, batteryManager->getLowestCellTempId(), lowestCellTempId);
            processValue(&valueCache.highestCellTempId, batteryManager->getHighestCellTempId(), highestCellTempId);
        }
        if (batteryManager->hasCellVoltages()) {
            processValue(&valueCache.lowestCellVolts, batteryManager->getLowestCellVolts(), lowestCellVolts);
            processValue(&valueCache.highestCellVolts, batteryManager->getHighestCellVolts(), highestCellVolts);
            processValue(&valueCache.averageCellVolts, batteryManager->getAverageCellVolts(), averageCellVolts);
            processValue(&valueCache.deltaCellVolts, batteryManager->getHighestCellVolts() - batteryManager->getLowestCellVolts(), deltaCellVolts);
            processValue(&valueCache.lowestCellVoltsId, batteryManager->getLowestCellVoltsId(), lowestCellVoltsId);
            processValue(&valueCache.highestCellVoltsId, batteryManager->getHighestCellVoltsId(), highestCellVoltsId);
        }
        if (batteryManager->hasCellResistance()) {
            processValue(&valueCache.lowestCellResistance, batteryManager->getLowestCellResistance(), lowestCellResistance);
            processValue(&valueCache.highestCellResistance, batteryManager->getHighestCellResistance(), highestCellResistance);
            processValue(&valueCache.averageCellResistance, batteryManager->getAverageCellResistance(), averageCellResistance);
            processValue(&valueCache.deltaCellResistance, (batteryManager->getHighestCellResistance() - batteryManager->getLowestCellResistance()), deltaCellResistance);
            processValue(&valueCache.lowestCellResistanceId, batteryManager->getLowestCellResistanceId(), lowestCellResistanceId);
            processValue(&valueCache.highestCellResistanceId, batteryManager->getHighestCellResistanceId(), highestCellResistanceId);
        }
        if (batteryManager->hasPackResistance()) {
            processValue(&valueCache.packResistance, batteryManager->getPackResistance(), packResistance);
        }
        if (batteryManager->hasPackHealth()) {
            processValue(&valueCache.packHealth, batteryManager->getPackHealth(), packHealth);
        }
        if (batteryManager->hasPackCycles()) {
            processValue(&valueCache.packCycles, batteryManager->getPackCycles(), packCycles);
        }
        processValue(&valueCache.bmsTemperature, batteryManager->getSystemTemperature(), bmsTemperature);
    }

    if (outPos > 0) {
        String header = "data:";
        header.concat(dataPointCount);
        serialInterface->print(header); // indicate that we will send a binary data stream of x bytes
        serialInterface->write(13); // CR
        serialInterface->write(outBuffer, outPos); // send the binary data
    }
}

void WifiEsp32::processValue(bool *cacheValue, bool value, DataPointCode code) {
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    outBuffer[outPos++] = DATA_POINT_START;
    outBuffer[outPos++] = code;
    outBuffer[outPos++] = (value ? 1 : 0);
    dataPointCount++;
}

void WifiEsp32::processValue(uint8_t *cacheValue, uint8_t value, DataPointCode code) {
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    outBuffer[outPos++] = DATA_POINT_START;
    outBuffer[outPos++] = code;
    outBuffer[outPos++] = value;
    dataPointCount++;
}

void WifiEsp32::processValue(uint16_t *cacheValue, uint16_t value, DataPointCode code) {
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    outBuffer[outPos++] = DATA_POINT_START;
    outBuffer[outPos++] = code;
    outBuffer[outPos++] = (value & 0xFF00) >> 8;
    outBuffer[outPos++] = value & 0x00FF;
    dataPointCount++;
}

void WifiEsp32::processValue(int16_t *cacheValue, int16_t value, DataPointCode code) {
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    outBuffer[outPos++] = DATA_POINT_START;
    outBuffer[outPos++] = code;
    outBuffer[outPos++] = (value & 0xFF00) >> 8;
    outBuffer[outPos++] = value & 0x00FF;
    dataPointCount++;
}

void WifiEsp32::processValue(uint32_t *cacheValue, uint32_t value, DataPointCode code) {
    if (*cacheValue == value)
        return;
    *cacheValue = value;

    outBuffer[outPos++] = DATA_POINT_START;
    outBuffer[outPos++] = code;
    outBuffer[outPos++] = (value & 0xFF000000) >> 24;
    outBuffer[outPos++] = (value & 0x00FF0000) >> 16;
    outBuffer[outPos++] = (value & 0x0000FF00) >> 8;
    outBuffer[outPos++] = value & 0x000000FF;
    dataPointCount++;
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
