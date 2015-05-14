/*
 * SerialConsole.cpp
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

#include "SerialConsole.h"

extern PrefHandler *sysPrefs;

SerialConsole::SerialConsole(MemCache* memCache) : memCache(memCache)
{
    init();
}

void SerialConsole::init()
{
    handlingEvent = false;

    //State variables for serial console
    ptrBuffer = 0;
    state = STATE_ROOT_MENU;
}

void SerialConsole::loop()
{
    if (handlingEvent == false) {
        if (SerialUSB.available()) {
            serialEvent();
        }
    }
}

void SerialConsole::printMenu()
{
    DeviceManager *deviceManager = DeviceManager::getInstance();
    Status *status = Status::getInstance();

    //Show build # here as well in case people are using the native port and don't get to see the start up messages
    Logger::console("\nBuild number: %i", CFG_BUILD_NUM);
    Logger::console("System State: %s", status->systemStateToStr(status->getSystemState()));
    Logger::console("System Menu:\n");
    Logger::console("Enable line endings of some sort (LF, CR, CRLF)\n");
    Logger::console("Short Commands:");
    Logger::console("h = help (displays this message)");
    Device *heartbeat = deviceManager->getDeviceByID(HEARTBEAT);
    if (heartbeat != NULL && heartbeat->isEnabled()) {
        Logger::console("L = show raw analog/digital input/output values (toggle)");
    }
    Logger::console("K = set all outputs high");
    Logger::console("J = set all outputs low");
    //Logger::console("U,I = test EEPROM routines");
    Logger::console("E = dump system eeprom values");
    Logger::console("z = detect throttle min/max, num throttles and subtype");
    Logger::console("Z = save throttle values");
    Logger::console("b = detect brake min/max");
    Logger::console("B = save brake values");
    Logger::console("p = enable wifi passthrough (reboot required to resume normal operation)");
    Logger::console("S = show list of devices");
    Logger::console("w = reset wifi to factory defaults, setup GEVCU ad-hoc network");
    Logger::console("W = Set Wifi to WPS mode (try to automatically connect)");
    Logger::console("s = Scan WiFi for nearby access points");

    Logger::console("\nConfig Commands (enter command=newvalue)\n");
    Logger::console("LOGLEVEL=%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", Logger::getLogLevel());
    uint8_t systype;
    sysPrefs->read(EESYS_SYSTEM_TYPE, &systype);
    Logger::console("SYSTYPE=%i - Set board revision (Dued=2, GEVCU3=3, GEVCU4=4)\n", systype);

    deviceManager->printDeviceList();

    printMenuMotorController();
    printMenuThrottle();
    printMenuBrake();
    printMenuSystemIO();
    printMenuCharger();
    printMenuDcDcConverter();

    Logger::console("WLAN - send a AT+i command to the wlan device");
}

void SerialConsole::printMenuMotorController() {
    MotorController* motorController = (MotorController*) DeviceManager::getInstance()->getMotorController();

    if (motorController && motorController->getConfiguration()) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();
        Logger::console("\nMOTOR CONTROLS\n");
        Logger::console("TORQ=%i - Set torque upper limit (in 0.1Nm)", config->torqueMax);
        Logger::console("RPMS=%i - Set maximum speed (in RPMs)", config->speedMax);
        Logger::console("REVLIM=%i - How much torque to allow in reverse (in 0.1%%)", config->reversePercent);
        Logger::console("NOMV=%i - Fully charged pack voltage (in 0.1V)", config->nominalVolt);
        Logger::console("kWh=%d - kiloWatt Hours of energy used", motorController->getKiloWattHours() / 3600000);
    }
}

void SerialConsole::printMenuThrottle() {
    Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();

    if (accelerator && accelerator->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) accelerator->getConfiguration();
        Logger::console("\nTHROTTLE CONTROLS\n");
        if (accelerator->getId() == POTACCELPEDAL) {
            PotThrottleConfiguration *potConfig = (PotThrottleConfiguration *) config;
            Logger::console("TPOT=%i - Number of pots to use (1 or 2)", potConfig->numberPotMeters);
            Logger::console("TTYPE=%i - Set throttle subtype (1=std linear, 2=inverse)", potConfig->throttleSubType);
            Logger::console("T1MN=%i - Set throttle 1 min value", potConfig->minimumLevel);
            Logger::console("T1MX=%i - Set throttle 1 max value", potConfig->maximumLevel);
            Logger::console("T2MN=%i - Set throttle 2 min value", potConfig->minimumLevel2);
            Logger::console("T2MX=%i - Set throttle 2 max value", potConfig->maximumLevel2);
        }
        if (accelerator->getId() == CANACCELPEDAL) {
            CanThrottleConfiguration *canConfig = (CanThrottleConfiguration *) config;
            Logger::console("T1MN=%i - Set throttle 1 min value", canConfig->minimumLevel);
            Logger::console("T1MX=%i - Set throttle 1 max value", canConfig->maximumLevel);
            Logger::console("TCTP=%i - Set car type", canConfig->carType);
        }
        Logger::console("TRGNMAX=%i - Pedal position where regen is at max (in 0.1%%)", config->positionRegenMaximum);
        Logger::console("TRGNMIN=%i - Pedal position where regen is at min (in 0.1%%)", config->positionRegenMinimum);
        Logger::console("TFWD=%i - Pedal position where forward motion starts  (in 0.1%%)", config->positionForwardMotionStart);
        Logger::console("TMAP=%i - Pedal position of 50%% torque (in 0.1%%)", config->positionHalfPower);
        Logger::console("TMINRN=%i - Torque to use for min throttle regen (in 1%%)", config->minimumRegen);
        Logger::console("TMAXRN=%i - Torque to use for max throttle regen (in 1%%)", config->maximumRegen);
        Logger::console("TCREEP=%i - Torque to use for creep (0=disable) (in 1%%)", config->creep);
    }
}

void SerialConsole::printMenuBrake() {
    Throttle *brake = DeviceManager::getInstance()->getBrake();

    if (brake && brake->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) brake->getConfiguration();
        Logger::console("\nBRAKE CONTROLS\n");
        Logger::console("B1MN=%i - Set brake min value", config->minimumLevel);
        Logger::console("B1MX=%i - Set brake max value", config->maximumLevel);
        Logger::console("BMINR=%i - Torque for start of brake regen (in 1%%)", config->minimumRegen);
        Logger::console("BMAXR=%i - Torque for maximum brake regen (in 1%%)", config->maximumRegen);
    }
}

void SerialConsole::printMenuSystemIO() {
    SystemIO *systemIO = SystemIO::getInstance();

    if (systemIO && systemIO->getConfiguration()) {
        SystemIOConfiguration *config = systemIO->getConfiguration();
        Logger::console("\nSYSTEM I/O\n");
        Logger::console("ENABLEI=%i - Digital input to use for enable signal (255 to disable)", config->enableInput);
        Logger::console("CHARGEI=%i - Digital input to use for charger signal (255 to disable)", config->chargePowerAvailableInput);
        Logger::console("INTERLI=%i - Digital input to use for interlock signal (255 to disable)\n", config->interlockInput);

        Logger::console("PREDELAY=%d - Precharge delay time (in milliseconds)", config->prechargeMillis);
        Logger::console("PRELAY=%i - Digital output to use for precharge contactor (255 to disable)", config->prechargeRelayOutput);
        Logger::console("MRELAY=%i - Digital output to use for main contactor (255 to disable)", config->mainContactorOutput);
        Logger::console("NRELAY=%i - Digital output to use for secondary contactor (255 to disable)", config->secondaryContactorOutput);
        Logger::console("FRELAY=%i - Digital output to use for fast charge contactor (255 to disable)\n", config->fastChargeContactorOutput);

        Logger::console("ENABLEM=%i - Digital output to use for enable motor signal (255 to disable)", config->enableMotorOutput);
        Logger::console("ENABLEC=%i - Digital output to use for enable charger signal (255 to disable)", config->enableChargerOutput);
        Logger::console("ENABLED=%i - Digital output to use for enable dc-dc converter signal (255 to disable)", config->enableDcDcOutput);
        Logger::console("ENABLEH=%i - Digital output to use for enable heater signal (255 to disable)\n", config->enableHeaterOutput);

        Logger::console("HEATVALV=%i - Digital output to actuate heater valve (255 to disable)", config->heaterValveOutput);
        Logger::console("HEATPUMP=%i - Digital output to turn on heater pump (255 to disable)", config->heaterPumpOutput);
        Logger::console("COOLPUMP=%i - Digital output to turn on cooling pump (255 to disable)", config->coolingPumpOutput);
        Logger::console("COOLFAN=%i - Digital output to turn on cooling fan (255 to disable)", config->coolingFanOutput);
        Logger::console("COOLON=%i - Inverter temperature to turn cooling on (deg celsius)", config->coolingTempOn);
        Logger::console("COOLOFF=%i - Inverter temperature to turn cooling off (deg celsius)\n", config->coolingTempOff);

        Logger::console("BRAKELT=%i - Digital output to use for brake light (255 to disable)", config->brakeLightOutput);
        Logger::console("REVLT=%i - Digital output to use for reverse light (255 to disable)", config->reverseLightOutput);
        Logger::console("WARNLT=%i - Digital output to use for reverse light (255 to disable)", config->warningOutput);
        Logger::console("LIMITLT=%i - Digital output to use for reverse light (255 to disable)", config->powerLimitationOutput);
    }
}

void SerialConsole::printMenuCharger() {
    Charger *charger = DeviceManager::getInstance()->getCharger();

    if (charger && charger->getConfiguration()) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();
        Logger::console("\nCHARGER CONTROLS\n");
        Logger::console("CHCC=%i - Constant current (in 0.1A)", config->constantCurrent);
        Logger::console("CHCV=%i - Constant voltage (in 0.1V)", config->constantVoltage);
        Logger::console("CHTC=%i - Terminate current (in 0.1A)", config->terminateCurrent);
        Logger::console("CHICMX=%i - Maximum Input current (in 0.1V)", config->maximumInputCurrent);
        Logger::console("CHBVMN=%i - Minimum battery voltage (in 0.1V)", config->minimumBatteryVoltage);
        Logger::console("CHBVMX=%i - Maximum battery voltage (in 0.1V)", config->maximumBatteryVoltage);
        Logger::console("CHTPMN=%i - Minimum battery temperature for charging (in 0.1 deg C)", config->minimumTemperature);
        Logger::console("CHTPMX=%i - Maximum battery temperature for charging (in 0.1 deg C)", config->maximumTemperature);
        Logger::console("CHAHMX=%i - Maximum ampere hours (in 0.1Ah)", config->maximumAmpereHours);
        Logger::console("CHCTMX=%i - Maximum charge time (in 1 min)", config->maximumChargeTime);
        Logger::console("CHTMOD=%i - Temperature control mode (0 = hysterese, 1 = derating", config->useTemperatureDerating);
        Logger::console("CHTDRC=%i - Derating of charge current (in 0.1Ah per deg C)", config->deratingTemperature);
        Logger::console("CHTDRS=%i - Reference temperature for derating (in 1 deg C)", config->deratingReferenceTemperature);
        Logger::console("CHTHYS=%i - Hysterese temperature where charging will be stopped (in 1 deg C)", config->hystereseStopTemperature);
        Logger::console("CHTHYR=%i - Hysterese temperature where charging will resume (in 1 deg C)", config->hystereseResumeTemperature);
    }
}

void SerialConsole::printMenuDcDcConverter() {
    DcDcConverter *dcDcConverter = DeviceManager::getInstance()->getDcDcConverter();

    if (dcDcConverter && dcDcConverter->getConfiguration()) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();
        Logger::console("\nDCDC CONVERTER CONTROLS\n");
        Logger::console("DCMODE=%i - operation mode (0 = buck - default, 1 = boost)", config->boostMode);
        Logger::console("DCBULV=%i - Buck mode LV voltage (in 0.1V)", config->lowVoltageCommand);
        Logger::console("DCBULVC=%i - Buck mode LV current limit (in 1A)", config->lvBuckModeCurrentLimit);
        Logger::console("DCBUHVV=%i - Buck mode HV under voltage limit (in 1 V)", config->hvUndervoltageLimit);
        Logger::console("DCBUHVC=%i - Buck mode HV current limit (in 0.1A)", config->hvBuckModeCurrentLimit);
        Logger::console("DCBOHV=%i - Boost mode HV voltage (in 1V)", config->highVoltageCommand);
        Logger::console("DCBOLVV=%i - Boost mode LV under voltage limit (in 0.1V)", config->lvUndervoltageLimit);
        Logger::console("DCBOLVC=%i - Boost mode LV current limit (in 1A)", config->lvBoostModeCurrentLinit);
        Logger::console("DCBOHVC=%i - Boost mode HV current limit (in 0.1A)", config->hvBoostModeCurrentLimit);
    }
}

/*  There is a help menu (press H or h or ?)

 This is no longer going to be a simple single character console.
 Now the system can handle up to 80 input characters. Commands are submitted
 by sending line ending (LF, CR, or both)
 */
void SerialConsole::serialEvent()
{
    int incoming;
    incoming = SerialUSB.read();

    if (incoming == -1) { //false alarm....
        return;
    }

    if (incoming == 10 || incoming == 13) { //command done. Parse it.
        handleConsoleCmd();
        ptrBuffer = 0; //reset line counter once the line has been processed
    } else {
        cmdBuffer[ptrBuffer++] = (unsigned char) incoming;

        if (ptrBuffer > 79) {
            ptrBuffer = 79;
        }
    }
}

void SerialConsole::handleConsoleCmd()
{
    handlingEvent = true;

    if (state == STATE_ROOT_MENU) {
        if (ptrBuffer == 1) { //command is a single ascii character
            handleShortCmd();
        } else { //if cmd over 1 char then assume (for now) that it is a config line
            handleConfigCmd();
        }
    }

    handlingEvent = false;
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
 comparison purposes.
 */
void SerialConsole::handleConfigCmd()
{
    int i;
    int value;
    bool updateWifi = true;

    //Logger::debug("Cmd size: %i", ptrBuffer);
    if (ptrBuffer < 6) {
        return;    //4 digit command, =, value is at least 6 characters
    }

    cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
    String command = String();
    unsigned char whichEntry = '0';
    i = 0;

    while (cmdBuffer[i] != '=' && i < ptrBuffer) {
        /*if (cmdBuffer[i] >= '0' && cmdBuffer[i] <= '9') {
            whichEntry = cmdBuffer[i++] - '0';
        }
        else */ command.concat(String(cmdBuffer[i++]));
    }

    i++; //skip the =

    if (i >= ptrBuffer) {
        Logger::console("Command needs a value..ie TORQ=3000\n");
        return; //or, we could use this to display the parameter instead of setting
    }

    // strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
    value = strtol((char *)(cmdBuffer + i), NULL, 0);
    command.toUpperCase();

    if (!handleConfigCmdMotorController(command, value) && !handleConfigCmdThrottle(command, value) &&
            !handleConfigCmdBrake(command, value) && !handleConfigCmdSystemIO(command, value) &&
            !handleConfigCmdCharger(command, value) && !handleConfigCmdDcDcConverter(command, value) &&
            !handleConfigCmdSystem(command, value)) {
        if (command == String("WLAN")) {
        DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)(cmdBuffer + i));
        Logger::info("sent \"AT+i%s\" to wlan device", (cmdBuffer + i));
        updateWifi = false;
        } else {
            Logger::error("unknown command: %s", command);
            updateWifi = false;
        }
    }

    // send updates to ichip wifi
    if (updateWifi) {
        DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);
    }
}

bool SerialConsole::handleConfigCmdMotorController(String command, long value)
{
    MotorController *motorController = DeviceManager::getInstance()->getMotorController();
    MotorControllerConfiguration *config = NULL;

    if (!motorController) {
        return false;
    }
    config = (MotorControllerConfiguration *) motorController->getConfiguration();

    if (command == String("TORQ")) {
        Logger::console("Setting torque limit to %fNm", (float) value / 10.0f);
        config->torqueMax = value;
    } else if (command == String("RPMS")) {
        Logger::console("Setting speed limit to %irpm", value);
        config->speedMax = value;
    } else if (command == String("REVLIM")) {
        Logger::console("Setting reverse limit to %f%%", (float) value / 10.0f);
        config->reversePercent = value;
    } else if (command == String("NOMV")) {
        Logger::console("Setting fully charged voltage to %fV", (float) value / 10.0f);
        config->nominalVolt = value;
    } else {
        return false;
    }
    motorController->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdThrottle(String command, long value) {
    Throttle *throttle = DeviceManager::getInstance()->getAccelerator();
    ThrottleConfiguration *config = NULL;

    if (!throttle) {
        return false;
    }
    config = (ThrottleConfiguration *) throttle->getConfiguration();

    if (command == String("TPOT") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting # of throttle pots to %i", value);
        ((PotThrottleConfiguration *)config)->numberPotMeters = value;
    } else if (command == String("TTYPE") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle subtype to %s", (value == 2 ? "inverse" : "std linear"));
        ((PotThrottleConfiguration *)config)->throttleSubType = value;
    } else if (command == String("T2MN") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 min to %i", value);
        ((PotThrottleConfiguration *)config)->minimumLevel2 = value;
    } else if (command == String("T2MX") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 max to %i", value);
        ((PotThrottleConfiguration *)config)->maximumLevel2 = value;
    } else if (command == String("TCTP") && (throttle->getId() == CANACCELPEDAL)) {
        Logger::console("Setting car type to %i", value);
        ((CanThrottleConfiguration *)config)->carType = value;
    } else if (command == String("T1MN")) {
        Logger::console("Setting throttle 1 min to %i", value);
        config->minimumLevel = value;
    } else if (command == String("T1MX")) {
        Logger::console("Setting throttle 1 max to %i", value);
        config->maximumLevel = value;
    } else if (command == String("TRGNMAX")) {
        Logger::console("Setting throttle regen maximum to %f%%", (float) value / 10.0f);
        config->positionRegenMaximum = value;
    } else if (command == String("TRGNMIN")) {
        Logger::console("Setting throttle regen minimum to %f%%", (float) value / 10.0f);
        config->positionRegenMinimum = value;
    } else if (command == String("TFWD")) {
        Logger::console("Setting throttle forward start to %f%%", (float) value / 10.0f);
        config->positionForwardMotionStart = value;
    } else if (command == String("TMAP")) {
        Logger::console("Setting throttle 50%% torque to %f%%", (float) value / 10.0f);
        config->positionHalfPower = value;
    } else if (command == String("TMINRN")) {
        Logger::console("Setting throttle regen minimum strength to %i%%", value);
        config->minimumRegen = value;
    } else if (command == String("TMAXRN")) {
        Logger::console("Setting throttle Regen maximum strength to %i%%", value);
        config->maximumRegen = value;
    } else if (command == String("TCREEP")) {
        Logger::console("Setting throttle creep strength to %i%%", value);
        config->creep = value;
    } else {
        return false;
    }
    throttle->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdBrake(String command, long value) {
    Throttle *brake = DeviceManager::getInstance()->getBrake();
    ThrottleConfiguration *config = NULL;

    if (!brake) {
        return false;
    }
    config = (ThrottleConfiguration *) brake->getConfiguration();

    if (command == String("B1MN")) {
        Logger::console("Setting brake min to %i", value);
        config->minimumLevel = value;
    } else if (command == String("B1MX")) {
        Logger::console("Setting brake max to %i", value);
        config->maximumLevel = value;
    } else if (command == String("BMINR")) {
        Logger::console("Setting min brake regen to %i%%", value);
        config->minimumRegen = value;
    } else if (command == String("BMAXR")) {
        Logger::console("Setting max brake regen to %i%%", value);
        config->maximumRegen = value;
    } else {
        return false;
    }
    brake->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdSystemIO(String command, long value) {
    SystemIO *systemIO = SystemIO::getInstance();
    SystemIOConfiguration *config = systemIO->getConfiguration();

    if (command == String("ENABLEI")) {
        if (value <= CFG_NUMBER_DIGITAL_INPUTS && value >= 0) {
            Logger::console("Setting enable signal to input %i.", value);
            config->enableInput = value;
        } else {
            Logger::console("Invalid enable signal input number. Please enter a value 0 - %d", CFG_NUMBER_DIGITAL_INPUTS - 1);
        }
    } else if (command == String("CHARGEI")) {
        Logger::console("Setting charge signal to input %i", value);
        config->chargePowerAvailableInput = value;
    } else if (command == String("INTERLI")) {
        Logger::console("Setting interlock signal to input %i", value);
        config->interlockInput = value;
    } else if (command == String("PREDELAY")) {
        Logger::console("Setting precharge time to %ims", value);
        config->prechargeMillis = value;
    } else if (command == String("PRELAY")) {
        Logger::console("Setting precharge relay to output %i", value);
        config->prechargeRelayOutput = value;
    } else if (command == String("MRELAY")) {
        Logger::console("Setting main contactor to output %i", value);
        config->mainContactorOutput = value;
    } else if (command == String("NRELAY")) {
        Logger::console("Setting secondary contactor to output %i", value);
        config->secondaryContactorOutput = value;
    } else if (command == String("FRELAY")) {
        Logger::console("Setting fast charge contactor to output %i", value);
        config->fastChargeContactorOutput = value;
    } else if (command == String("ENABLEM")) {
        Logger::console("Setting enable motor signal to output %i", value);
        config->enableMotorOutput = value;
    } else if (command == String("ENABLEC")) {
        Logger::console("Setting enable charger signal to output %i", value);
        config->enableChargerOutput = value;
    } else if (command == String("ENABLED")) {
        Logger::console("Setting enable DC-DC converter signal to output %i", value);
        config->enableDcDcOutput = value;
    } else if (command == String("ENABLEH")) {
        Logger::console("Setting enable heater signal to output %i", value);
        config->enableHeaterOutput = value;
    } else if (command == String("HEATVALV")) {
        Logger::console("Setting heater valve signal output %i", value);
        config->heaterValveOutput = value;
    } else if (command == String("HEATPUMP")) {
        Logger::console("Setting heater pump signal to output %i", value);
        config->heaterPumpOutput = value;
    } else if (command == String("COOLPUMP")) {
        Logger::console("Setting cooling pump signal to output %i", value);
        config->coolingPumpOutput = value;
    } else if (command == String("COOLFAN")) {
        Logger::console("Setting cooling fan signal to output %i", value);
        config->coolingFanOutput = value;
    } else if (command == String("COOLON")) {
        if (value <= 200 && value >= 0) {
            Logger::console("Setting cooling ON temperature to: %i deg C", value);
            config->coolingTempOn = value;
        } else {
            Logger::console("Invalid cooling ON temperature. Please enter a value 0 - 200");
        }
    } else if (command == String("COOLOFF")) {
        if (value <= 200 && value >= 0) {
            Logger::console("Setting cooling OFF temperature to: %i deg C", value);
            config->coolingTempOff = value;
        } else {
            Logger::console("Invalid cooling OFF temperature. Please enter a value 0 - 200");
        }
    } else if (command == String("BRAKELT")) {
        Logger::console("Brake light signal set to output %i.", value);
        config->brakeLightOutput = value;
    } else if (command == String("REVLT")) {
        Logger::console("Reverse light signal set to output %i.", value);
        config->reverseLightOutput = value;
    } else if (command == String("WARNLT")) {
        Logger::console("Warning signal set to output %i.", value);
        config->warningOutput = value;
    } else if (command == String("LIMITLT")) {
        Logger::console("Limit signal set to output %i.", value);
        config->powerLimitationOutput = value;
    } else {
        return false;
    }
    systemIO->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdCharger(String command, long value) {
    Charger *charger = DeviceManager::getInstance()->getCharger();
    ChargerConfiguration *config = NULL;

    if (!charger) {
        return false;
    }
    config = (ChargerConfiguration *) charger->getConfiguration();

    if (command == String("CHCC")) {
        Logger::console("Setting constant current to %fA", (float) value / 10.0f);
        config->constantCurrent = value;
    } else if (command == String("CHCV")) {
        Logger::console("Setting constant voltage to %fV", (float) value / 10.0f);
        config->constantVoltage = value;
    } else if (command == String("CHTC")) {
        Logger::console("Setting terminate current to %fA", (float) value / 10.0f);
        config->terminateCurrent = value;
    } else if (command == String("CHICMX")) {
        Logger::console("Setting max input current to %fV", (float) value / 10.0f);
        config->maximumInputCurrent = value;
    } else if (command == String("CHBVMN")) {
        Logger::console("Setting min battery voltage to %fV", (float) value / 10.0f);
        config->minimumBatteryVoltage = value;
    } else if (command == String("CHBVMX")) {
        Logger::console("Setting max battery voltage to %fV", (float) value / 10.0f);
        config->maximumBatteryVoltage = value;
    } else if (command == String("CHTPMN")) {
        Logger::console("Setting min battery temp to %f deg C", (float) value / 10.0f);
        config->minimumTemperature = value;
    } else if (command == String("CHTPMX")) {
        Logger::console("Setting max battery temp to %f deg C", (float) value / 10.0f);
        config->maximumTemperature = value;
    } else if (command == String("CHAHMX")) {
        Logger::console("Setting max Ampere hours to %fAh", (float) value / 10.0f);
        config->maximumAmpereHours = value;
    } else if (command == String("CHCTMX")) {
        Logger::console("Setting max charge time to %i min", value);
        config->maximumChargeTime = value;
    } else if (command == String("CHTMOD")) {
        Logger::console("Setting temp cotrol mode to %s", (value == 0 ? "hysterese" : "derating"));
        config->useTemperatureDerating = value;
    } else if (command == String("CHTDRC")) {
        Logger::console("Setting derating of charge current to %fAh per deg C", (float) value / 10.0f);
        config->deratingTemperature = value;
    } else if (command == String("CHTDRS")) {
        Logger::console("Setting derating reference temp to %i deg C", value);
        config->deratingReferenceTemperature = value;
    } else if (command == String("CHTHYS")) {
        Logger::console("Setting hysterese temp to stop charging to %i deg C", value);
        config->hystereseStopTemperature = value;
    } else if (command == String("CHTHYR")) {
        Logger::console("Setting hysterese temp to resume charging to %i deg C", value);
        config->hystereseResumeTemperature = value;
    } else {
        return false;
    }
    charger->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdDcDcConverter(String command, long value) {
    DcDcConverter *dcdcConverter = DeviceManager::getInstance()->getDcDcConverter();
    DcDcConverterConfiguration *config = NULL;

    if (!dcdcConverter) {
        return false;
    }
    config = (DcDcConverterConfiguration *) dcdcConverter->getConfiguration();

    if (command == String("DCMODE")) {
        Logger::console("Setting mode to %s", (value == 0 ? "buck" : "boost"));
        config->boostMode = value;
    } else if (command == String("DCBULV")) {
        Logger::console("Setting buck LV voltage to %fV", (float) value / 10.0f);
        config->lowVoltageCommand = value;
    } else if (command == String("DCBULVC")) {
        Logger::console("Setting buck LV current limit to %iA", value);
        config->lvBuckModeCurrentLimit = value;
    } else if (command == String("DCBUHVV")) {
        Logger::console("Setting buck HV under voltage limit to %iV", value);
        config->hvUndervoltageLimit = value;
    } else if (command == String("DCBUHVC")) {
        Logger::console("Setting buck HV current limit to %fA", (float) value / 10.0f);
        config->hvBuckModeCurrentLimit = value;
    } else if (command == String("DCBOHV")) {
        Logger::console("Setting boost HV voltage to %iV", value);
        config->highVoltageCommand = value;
    } else if (command == String("DCBOLVV")) {
        Logger::console("Setting boost LV undervoltage limit to %fV", (float) value / 10.0f);
        config->lvUndervoltageLimit = value;
    } else if (command == String("DCBOLVC")) {
        Logger::console("Setting boost LV current limit to %iA", value);
        config->lvBoostModeCurrentLinit = value;
    } else if (command == String("DCBOHVC")) {
        Logger::console("Setting boost HV current limit to %fA", (float) value / 10.0f);
        config->hvBoostModeCurrentLimit = value;
    } else {
        return false;
    }
    dcdcConverter->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdSystem(String command, long value) {

    if (command == String("ENABLE")) {
        if(!DeviceManager::getInstance()->sendMessage(DEVICE_ANY, (DeviceId) value, MSG_ENABLE, NULL)) {
            Logger::console("Invalid device ID (%X, %d)", value, value);
        }
    } else if (command == String("DISABLE")) {
        if(!DeviceManager::getInstance()->sendMessage(DEVICE_ANY, (DeviceId) value, MSG_DISABLE, NULL)) {
            Logger::console("Invalid device ID (%X, %d)", value, value);
        }
    } else if (command == String("SYSTYPE")) {
        if (value < 5 && value > 0) {
            sysPrefs->write(EESYS_SYSTEM_TYPE, (uint8_t)(value));
            sysPrefs->saveChecksum();
            sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
            Logger::console("System type updated. Power cycle to apply.");
        } else {
            Logger::console("Invalid system type. Please enter a value 1 - 4");
        }
    } else if (command == String("LOGLEVEL")) {
        switch (value) {
            case 0:
                Logger::setLoglevel(Logger::Debug);
                Logger::console("setting loglevel to 'debug'");
                break;

            case 1:
                Logger::setLoglevel(Logger::Info);
                Logger::console("setting loglevel to 'info'");
                break;

            case 2:
                Logger::console("setting loglevel to 'warning'");
                Logger::setLoglevel(Logger::Warn);
                break;

            case 3:
                Logger::console("setting loglevel to 'error'");
                Logger::setLoglevel(Logger::Error);
                break;

            case 4:
                Logger::console("setting loglevel to 'off'");
                Logger::setLoglevel(Logger::Off);
                break;
        }
        sysPrefs->write(EESYS_LOG_LEVEL, (uint8_t) value);
        sysPrefs->saveChecksum();
    } else {
        return false;
    }
    return true;
}

void SerialConsole::handleShortCmd()
{
    uint8_t val;
    DeviceManager *deviceManager = DeviceManager::getInstance();
    MotorController* motorController = (MotorController*) deviceManager->getMotorController();
    Throttle *accelerator = deviceManager->getAccelerator();
    Throttle *brake = deviceManager->getBrake();
    Heartbeat *heartbeat = (Heartbeat *) deviceManager->getDeviceByID(HEARTBEAT);

    switch (cmdBuffer[0]) {
        case 'h':
        case '?':
        case 'H':
            printMenu();
            break;

        case 'L':
            if (heartbeat != NULL) {
                heartbeat->setThrottleDebug(!heartbeat->getThrottleDebug());

                if (heartbeat->getThrottleDebug()) {
                    Logger::console("Output raw throttle");
                } else {
                    Logger::console("Cease raw throttle output");
                }
            }
            break;

        case 'U':
            Logger::console("Adding a sequence of values from 0 to 255 into eeprom");

            for (int i = 0; i < 256; i++) {
                memCache->Write(1000 + i, (uint8_t) i);
            }

            Logger::info("Flushing cache");
            memCache->FlushAllPages(); //write everything to eeprom
            memCache->InvalidateAll(); //remove all data from cache
            Logger::console("Operation complete.");
            break;

        case 'I':
            Logger::console("Retrieving data previously saved");

            for (int i = 0; i < 256; i++) {
                memCache->Read(1000 + i, &val);
                Logger::console("%d: %d", i, val);
            }
            break;

        case 'E':
            Logger::console("Reading System EEPROM values");

            for (int i = 0; i < 256; i++) {
                memCache->Read(EE_SYSTEM_START + i, &val);
                Logger::console("%d: %d", i, val);
            }
            break;

        case 'K': //set all outputs high
            for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
                SystemIO::getInstance()->setDigitalOut(tout, true);
            }

            Logger::console("all outputs: ON");
            break;

        case 'J': //set the four outputs low
            for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
                SystemIO::getInstance()->setDigitalOut(tout, false);
            }

            Logger::console("all outputs: OFF");
            break;

        case 'z': // detect throttle min/max & other details
            if (accelerator) {
                ThrottleDetector *detector = new ThrottleDetector(accelerator);
                detector->detect();
            }
            break;

        case 'Z': // save throttle settings
            if (accelerator) {
                accelerator->saveConfiguration();
            }
            break;

        case 'b':
            if (brake) {
                ThrottleDetector *detector = new ThrottleDetector(brake);
                detector->detect();
            }
            break;

        case 'B':
            if (brake != NULL) {
                brake->saveConfiguration();
            }
            break;

        case 'p':
            Logger::console("PASSTHROUGH MODE - All traffic Serial3 <-> SerialUSB");
            //this never stops so basically everything dies. you will have to reboot.
            int inSerialUSB, inSerial3;

            while (1 == 1) {
                inSerialUSB = SerialUSB.read();
                inSerial3 = Serial3.read();

                if (inSerialUSB > -1) {
                    Serial3.write((char) inSerialUSB);
                }

                if (inSerial3 > -1) {
                    SerialUSB.write((char) inSerial3);
                }
            }
            break;

        case 'S':
            deviceManager->printDeviceList();
            break;

        case 's':
            Logger::console("Finding and listing all nearby WiFi access points");
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "RP20");
            break;

        case 'W':
            Logger::console("Setting Wifi Adapter to WPS mode (make sure you press the WPS button on your router)");
            // restore factory defaults and give it some time
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "AWPS");
            break;

        case 'w':
            Logger::console("Resetting wifi to factory defaults and setting up to auto connect to open APs");
            // restore factory defaults and give it some time
            // pinMode(43,OUTPUT);
            //  digitalWrite(43, HIGH);
            //  delay(3000);
            //  digitalWrite(43, LOW);
            delay(3000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "FD");
            delay(2000);
            // set-up specific ad-hoc network
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "BDRA");
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "WLCH=1");  //use whichever channel an AP wants to use
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "WLSI=!GEVCU");  //set no SSID which enables auto searching for an open hotspot
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "DIP=192.168.3.10");  //enable searching for a proper IP via DHCP
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "DPSZ=10");  //turn off DHCP server
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "RPG=secret");  // set the configuration password for /ichip
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "WPWD=secret");  // set the password to update config params
            delay(1000);
            //  deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WSI1=AndroidAP"); // hotspot SSID
            //  deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WST1=4"); //wpa2
            //  deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WPP1=verysecret"); //wpa2 password

            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "AWS=1");  //turn on web server for three clients
            delay(1000);
            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "DOWN");  //cause a reset to allow it to come up with the settings
            delay(5000);  // a 5 second delay is required for the chip to come back up ! Otherwise commands will be lost

            deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);  // reload configuration params as they were lost
            Logger::console("Wifi initialized");
            break;

        case 'X':
            setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
            break;
    }
}
