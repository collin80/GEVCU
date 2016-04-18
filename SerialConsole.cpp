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

SerialConsole serialConsole;

SerialConsole::SerialConsole()
{
    handlingEvent = false;
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
    //Show build # here as well in case people are using the native port and don't get to see the start up messages
    Logger::console("\nBuild number: %i", CFG_BUILD_NUM);
    Logger::console("System State: %s", status.systemStateToStr(status.getSystemState()));
    Logger::console("System Menu:\n");
    Logger::console("Enable line endings of some sort (LF, CR, CRLF)\n");
    Logger::console("Short Commands:");
    Logger::console("h = help (displays this message)");
    Device *heartbeat = deviceManager.getDeviceByID(HEARTBEAT);
    if (heartbeat != NULL && heartbeat->isEnabled()) {
        Logger::console("L = show raw analog/digital input/output values (toggle)");
    }
    Logger::console("K = set all outputs high");
    Logger::console("J = set all outputs low");
    //Logger::console("U,I = test EEPROM routines");
    Logger::console("z = detect throttle min/max, num throttles and subtype");
    Logger::console("Z = save throttle values");
    Logger::console("b = detect brake min/max");
    Logger::console("B = save brake values");
    Logger::console("p = enable wifi passthrough (reboot required to resume normal operation)");
    Logger::console("S = show list of devices");
    Logger::console("w = GEVCU 4.2 reset wifi to factory defaults, setup GEVCU ad-hoc network");
    Logger::console("W = GEVCU 5.2 reset wifi to factory defaults, setup GEVCU as 10.0.0.1 Access Point");
    Logger::console("s = Scan WiFi for nearby access points");

    Logger::console("\nConfig Commands (enter command=newvalue)\n");
    Logger::console("LOGLEVEL=[deviceId,]%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", Logger::getLogLevel());
    Logger::console("SYSTYPE=%i - Set board revision (Dued=2, GEVCU3=3, GEVCU4=4)\n", systemIO.getSystemType());
    Logger::console("WLAN - send a AT+i command to the wlan device");
    Logger::console("NUKE=1 - Resets all device settings in EEPROM. You have been warned.");

    deviceManager.printDeviceList();

    printMenuMotorController();
    printMenuThrottle();
    printMenuBrake();
    printMenuSystemIO();
    printMenuCharger();
    printMenuDcDcConverter();
}

void SerialConsole::printMenuMotorController()
{
    MotorController* motorController = (MotorController*) deviceManager.getMotorController();

    if (motorController && motorController->getConfiguration()) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();
        Logger::console("\nMOTOR CONTROLS\n");
        Logger::console("TORQ=%i - Set torque upper limit (in 0.1Nm)", config->torqueMax);
        Logger::console("RPMS=%i - Set maximum speed (in RPMs)", config->speedMax);
        Logger::console("REVLIM=%i - How much torque to allow in reverse (in 0.1%%)", config->reversePercent);
        Logger::console("MOINVD=%i - invert the direction of the motor (0=normal, 1=invert)", config->invertDirection);
        Logger::console("MOSLEW=%i - slew rate (in 0.1 percent/sec, 0=disabled)", config->slewRate);
        Logger::console("MOMWMX=%i - maximal mechanical power of motor (in 100W steps)", config->maxMechanicalPowerMotor);
        Logger::console("MORWMX=%i - maximal mechanical power of regen (in 100W steps)", config->maxMechanicalPowerRegen);
        Logger::console("NOMV=%i - Fully charged pack voltage that automatically resets the kWh counter (in 0.1V)", config->nominalVolt);
        Logger::console("kWh=%d - kiloWatt Hours of energy used", motorController->getKiloWattHours() / 3600000);
        if (motorController->getId() == BRUSA_DMC5) {
            BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
            Logger::console("MOMVMN=%i - minimum DC voltage limit for motoring (in 0.1V)", dmc5Config->dcVoltLimitMotor);
            Logger::console("MOMCMX=%i - current limit for motoring (in 0.1A)", dmc5Config->dcCurrentLimitMotor);
            Logger::console("MORVMX=%i - maximum DC voltage limit for regen (in 0.1V)", dmc5Config->dcVoltLimitRegen);
            Logger::console("MORCMX=%i - current limit for regen (in 0.1A)", dmc5Config->dcCurrentLimitRegen);
            Logger::console("MOOSC=%i - enable the DMC5 oscillation limiter (1=enable, 0=disable, also set DMC parameter!)",
                    dmc5Config->enableOscillationLimiter);
        }
    }
}

void SerialConsole::printMenuThrottle()
{
    Throttle *accelerator = deviceManager.getAccelerator();

    if (accelerator && accelerator->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) accelerator->getConfiguration();
        Logger::console("\nTHROTTLE CONTROLS\n");
        if (accelerator->getId() == POTACCELPEDAL) {
            PotThrottleConfiguration *potConfig = (PotThrottleConfiguration *) config;
            Logger::console("TPOT=%i - Number of pots to use (1 or 2)", potConfig->numberPotMeters);
            Logger::console("TTYPE=%i - Set throttle subtype (1=std linear, 2=inverse)", potConfig->throttleSubType);
            Logger::console("T1MN=%i - Set throttle 1 min value", potConfig->minimumLevel);
            Logger::console("T1MX=%i - Set throttle 1 max value", potConfig->maximumLevel);
            Logger::console("T1ADC=%i - Set throttle 1 ADC pin", potConfig->AdcPin1);
            Logger::console("T2MN=%i - Set throttle 2 min value", potConfig->minimumLevel2);
            Logger::console("T2MX=%i - Set throttle 2 max value", potConfig->maximumLevel2);
            Logger::console("T2ADC=%i - Set throttle 2 ADC pin", potConfig->AdcPin2);
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

void SerialConsole::printMenuBrake()
{
    Throttle *brake = deviceManager.getBrake();

    if (brake && brake->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) brake->getConfiguration();
        Logger::console("\nBRAKE CONTROLS\n");
        if (brake->getId() == POTBRAKEPEDAL) {
            PotBrakeConfiguration *potConfig = (PotBrakeConfiguration *) config;
            Logger::console("B1ADC=%i - Set brake ADC pin", potConfig->AdcPin1);
        }
        Logger::console("B1MN=%i - Set brake min value", config->minimumLevel);
        Logger::console("B1MX=%i - Set brake max value", config->maximumLevel);
        Logger::console("BMINR=%i - Torque for start of brake regen (in 1%%)", config->minimumRegen);
        Logger::console("BMAXR=%i - Torque for maximum brake regen (in 1%%)", config->maximumRegen);
    }
}

void SerialConsole::printMenuSystemIO()
{
    SystemIOConfiguration *config = systemIO.getConfiguration();

    if (config) {
        Logger::console("\nSYSTEM I/O\n");
        Logger::console("ENABLEI=%i - Digital input to use for enable signal (255 to disable)", config->enableInput);
        Logger::console("CHARGEI=%i - Digital input to use for charger signal (255 to disable)", config->chargePowerAvailableInput);
        Logger::console("INTERLI=%i - Digital input to use for interlock signal (255 to disable)", config->interlockInput);
        Logger::console("REVIN=%i - Digital input to reverse motor rotation (255 to disable)\n", config->reverseInput);

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
        Logger::console("COOLON=%i - Controller temperature to turn cooling on (deg celsius)", config->coolingTempOn);
        Logger::console("COOLOFF=%i - Controller temperature to turn cooling off (deg celsius)\n", config->coolingTempOff);

        Logger::console("BRAKELT=%i - Digital output to use for brake light (255 to disable)", config->brakeLightOutput);
        Logger::console("REVLT=%i - Digital output to use for reverse light (255 to disable)", config->reverseLightOutput);
        Logger::console("PWRSTR=%i - Digital output to use to enable power steering (255 to disable)", config->powerSteeringOutput);
//        Logger::console("TBD=%i - Digital output to use to xxxxxx (255 to disable)", config->unusedOutput);

        Logger::console("WARNLT=%i - Digital output to use for reverse light (255 to disable)", config->warningOutput);
        Logger::console("LIMITLT=%i - Digital output to use for limitation indicator (255 to disable)", config->powerLimitationOutput);
        Logger::console("SOCHG=%i - Analog output to use to indicate state of charge (255 to disable)", config->stateOfChargeOutput);
        Logger::console("OUTPUT=<0-7> - toggles state of specified digital output");
    }
}

void SerialConsole::printMenuCharger()
{
    Charger *charger = deviceManager.getCharger();

    if (charger && charger->getConfiguration()) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();
        Logger::console("\nCHARGER CONTROLS\n");
        Logger::console("CHCC=%i - Constant current (in 0.1A)", config->constantCurrent);
        Logger::console("CHCV=%i - Constant voltage (in 0.1V)", config->constantVoltage);
        Logger::console("CHTC=%i - Terminate current (in 0.1A)", config->terminateCurrent);
        Logger::console("CHICMX=%i - Maximum Input current (in 0.1A)", config->maximumInputCurrent);
        Logger::console("CHBVMN=%i - Minimum battery voltage (in 0.1V)", config->minimumBatteryVoltage);
        Logger::console("CHBVMX=%i - Maximum battery voltage (in 0.1V)", config->maximumBatteryVoltage);
        Logger::console("CHTPMN=%i - Minimum battery temperature for charging (in 0.1 deg C)", config->minimumTemperature);
        Logger::console("CHTPMX=%i - Maximum battery temperature for charging (in 0.1 deg C)", config->maximumTemperature);
        Logger::console("CHAHMX=%i - Maximum ampere hours (in 0.1Ah)", config->maximumAmpereHours);
        Logger::console("CHCTMX=%i - Maximum charge time (in 1 min)", config->maximumChargeTime);
        Logger::console("CHTDRC=%i - Derating of charge current (in 0.1Ah per deg C)", config->deratingRate);
        Logger::console("CHTDRS=%i - Reference temperature for derating (in 0.1 deg C)", config->deratingReferenceTemperature);
        Logger::console("CHTHYS=%i - Hysterese temperature where charging will be stopped (in 0.1 deg C)", config->hystereseStopTemperature);
        Logger::console("CHTHYR=%i - Hysterese temperature where charging will resume (in 0.1 deg C)", config->hystereseResumeTemperature);
    }
}

void SerialConsole::printMenuDcDcConverter()
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter && dcDcConverter->getConfiguration()) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();
        Logger::console("\nDCDC CONVERTER CONTROLS\n");
        Logger::console("DCMODE=%i - operation mode (0 = buck/default, 1 = boost)", config->mode);
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
         else */command.concat(String(cmdBuffer[i++]));
    }

    i++; //skip the =

    if (i >= ptrBuffer) {
        Logger::console("Command needs a value..ie TORQ=3000\n");
        return; //or, we could use this to display the parameter instead of setting
    }

    // strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
    value = strtol((char *) (cmdBuffer + i), NULL, 0);
    command.toUpperCase();

    if (!handleConfigCmdMotorController(command, value) && !handleConfigCmdThrottle(command, value) && !handleConfigCmdBrake(command, value)
            && !handleConfigCmdSystemIO(command, value) && !handleConfigCmdCharger(command, value) && !handleConfigCmdDcDcConverter(command, value)
            && !handleConfigCmdSystem(command, value, (cmdBuffer + i))) {
        if (handleConfigCmdWifi(command, (cmdBuffer + i))) {
            updateWifi = false;
        } else {
            Logger::error("unknown command: %s", command.c_str());
        }
    }

    // send updates to ichip wifi
    if (updateWifi) {
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);
    }
}

bool SerialConsole::handleConfigCmdMotorController(String command, long value)
{
    MotorController *motorController = deviceManager.getMotorController();
    MotorControllerConfiguration *config = NULL;

    if (!motorController) {
        return false;
    }
    config = (MotorControllerConfiguration *) motorController->getConfiguration();

    if (command == String("TORQ")) {
        value = constrain(value, 0, 1000000);
        Logger::console("Setting torque limit to %fNm", value / 10.0f);
        config->torqueMax = value;
    } else if (command == String("RPMS")) {
        value = constrain(value, 0, 1000000);
        Logger::console("Setting speed limit to %irpm", value);
        config->speedMax = value;
    } else if (command == String("REVLIM")) {
        value = constrain(value, 0, 1000);
        Logger::console("Setting reverse limit to %f%%", value / 10.0f);
        config->reversePercent = value;
    } else if (command == String("NOMV")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting fully charged voltage to %fV", value / 10.0f);
        config->nominalVolt = value;
    } else if (command == String("MOINVD")) {
        value = constrain(value, 0, 1);
        Logger::console("Setting motor direction to %s", (value ? "inverted" : "normal"));
        config->invertDirection = value;
    } else if (command == String("MOSLEW")) {
        Logger::console("Setting slew rate to %f percent/sec", value / 10.0f);
        config->slewRate = value;
    } else if (command == String("MOMWMX")) {
        Logger::console("Setting maximal mechanical power of motor to %fkW", value / 10.0f);
        config->maxMechanicalPowerMotor = value;
    } else if (command == String("MORWMX")) {
        Logger::console("Setting maximal mechanical power of regen to %fkW", value / 10.0f);
        config->maxMechanicalPowerRegen = value;
    } else if (command == String("MOMVMN") && (motorController->getId() == BRUSA_DMC5)) {
        Logger::console("Setting minimum DC voltage limit for motoring to %fV", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcVoltLimitMotor = value;
    } else if (command == String("MOMCMX") && (motorController->getId() == BRUSA_DMC5)) {
        Logger::console("Setting current limit for motoring to %fA", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcCurrentLimitMotor = value;
    } else if (command == String("MORVMX") && (motorController->getId() == BRUSA_DMC5)) {
        Logger::console("Setting maximum DC voltage limit for regen to %fV", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcVoltLimitRegen = value;
    } else if (command == String("MORCMX") && (motorController->getId() == BRUSA_DMC5)) {
        Logger::console("Setting current limit for regen to %fA", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcCurrentLimitRegen = value;
    } else if (command == String("MOOSC") && (motorController->getId() == BRUSA_DMC5)) {
        value = constrain(value, 0, 1);
        Logger::console("Setting oscillation limiter to %s", (value == 0 ? "disabled" : "enabled"));
        ((BrusaDMC5Configuration *) config)->enableOscillationLimiter = value;
    } else {
        return false;
    }
    motorController->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdThrottle(String command, long value)
{
    Throttle *throttle = deviceManager.getAccelerator();
    ThrottleConfiguration *config = NULL;

    if (!throttle) {
        return false;
    }
    config = (ThrottleConfiguration *) throttle->getConfiguration();

    if (command == String("TPOT") && (throttle->getId() == POTACCELPEDAL)) {
        value = constrain(value, 1, 3);
        Logger::console("Setting # of throttle pots to %i", value);
        ((PotThrottleConfiguration *) config)->numberPotMeters = value;
    } else if (command == String("TTYPE") && (throttle->getId() == POTACCELPEDAL)) {
        value = constrain(value, 1, 2);
        Logger::console("Setting throttle subtype to %s", (value == 2 ? "inverse" : "std linear"));
        ((PotThrottleConfiguration *) config)->throttleSubType = value;
    } else if (command == String("T1ADC") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting Throttle1 ADC pin to %i", value);
        ((PotThrottleConfiguration *) config)->AdcPin1 = value;
    } else if (command == String("T2ADC") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting Throttle2 ADC pin to %i", value);
        ((PotThrottleConfiguration *) config)->AdcPin2 = value;
    } else if (command == String("T2MN") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 min to %i", value);
        ((PotThrottleConfiguration *) config)->minimumLevel2 = value;
    } else if (command == String("T2MX") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 max to %i", value);
        ((PotThrottleConfiguration *) config)->maximumLevel2 = value;
    } else if (command == String("TCTP") && (throttle->getId() == CANACCELPEDAL)) {
        Logger::console("Setting car type to %i", value);
        ((CanThrottleConfiguration *) config)->carType = value;
    } else if (command == String("T1MN")) {
        Logger::console("Setting throttle 1 min to %i", value);
        config->minimumLevel = value;
    } else if (command == String("T1MX")) {
        Logger::console("Setting throttle 1 max to %i", value);
        config->maximumLevel = value;
    } else if (command == String("TRGNMAX")) {
        value = constrain(value, 0, config->positionRegenMinimum);
        Logger::console("Setting throttle regen maximum to %f%%", value / 10.0f);
        config->positionRegenMaximum = value;
    } else if (command == String("TRGNMIN")) {
        value = constrain(value, config->positionRegenMaximum, config->positionForwardMotionStart);
        Logger::console("Setting throttle regen minimum to %f%%", value / 10.0f);
        config->positionRegenMinimum = value;
    } else if (command == String("TFWD")) {
        value = constrain(value, config->positionRegenMinimum, config->positionHalfPower);
        Logger::console("Setting throttle forward start to %f%%", value / 10.0f);
        config->positionForwardMotionStart = value;
    } else if (command == String("TMAP")) {
        value = constrain(value, config->positionForwardMotionStart, 1000);
        Logger::console("Setting throttle 50%% torque to %f%%", value / 10.0f);
        config->positionHalfPower = value;
    } else if (command == String("TMINRN")) {
        value = constrain(value, 0, config->maximumRegen);
        Logger::console("Setting throttle regen minimum strength to %i%%", value);
        config->minimumRegen = value;
    } else if (command == String("TMAXRN")) {
        value = constrain(value, config->minimumRegen, 100);
        Logger::console("Setting throttle Regen maximum strength to %i%%", value);
        config->maximumRegen = value;
    } else if (command == String("TCREEP")) {
        value = constrain(value, 0, 100);
        Logger::console("Setting throttle creep strength to %i%%", value);
        config->creep = value;
    } else {
        return false;
    }
    throttle->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdBrake(String command, long value)
{
    Throttle *brake = deviceManager.getBrake();
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
        value = constrain(value, 0, config->maximumRegen);
        Logger::console("Setting min brake regen to %i%%", value);
        config->minimumRegen = value;
    } else if (command == String("BMAXR")) {
        value = constrain(value, config->minimumRegen, 100);
        Logger::console("Setting max brake regen to %i%%", value);
        config->maximumRegen = value;
    } else if (command == String("B1ADC") && (brake->getId() == POTBRAKEPEDAL)) {
        Logger::console("Setting Brake ADC pin to %i", value);
        ((PotBrakeConfiguration *) config)->AdcPin1 = value;
    } else {
        return false;
    }
    brake->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdSystemIO(String command, long value)
{
    SystemIOConfiguration *config = systemIO.getConfiguration();

    if (command == String("ENABLEI")) {
        if (value <= CFG_NUMBER_DIGITAL_INPUTS && value >= 0) {
            Logger::console("Setting enable signal to input %i.", value);
            config->enableInput = value;
        } else {
            Logger::console("Invalid enable signal input number. Please enter a value 0 - %d", CFG_NUMBER_DIGITAL_INPUTS - 1);
        }
    } else if (command == String("CHARGEI")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting charge signal to input %i", value);
        config->chargePowerAvailableInput = value;
    } else if (command == String("INTERLI")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting interlock signal to input %i", value);
        config->interlockInput = value;
    } else if (command == String("REVIN")) {
        config->reverseInput = value;
        Logger::console("Motor reverse signal set to input %i.", value);
    } else if (command == String("PREDELAY")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting precharge time to %ims", value);
        config->prechargeMillis = value;
    } else if (command == String("PRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting precharge relay to output %i", value);
        config->prechargeRelayOutput = value;
    } else if (command == String("MRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting main contactor to output %i", value);
        config->mainContactorOutput = value;
    } else if (command == String("NRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting secondary contactor to output %i", value);
        config->secondaryContactorOutput = value;
    } else if (command == String("FRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting fast charge contactor to output %i", value);
        config->fastChargeContactorOutput = value;
    } else if (command == String("ENABLEM")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable motor signal to output %i", value);
        config->enableMotorOutput = value;
    } else if (command == String("ENABLEC")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable charger signal to output %i", value);
        config->enableChargerOutput = value;
    } else if (command == String("ENABLED")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable DC-DC converter signal to output %i", value);
        config->enableDcDcOutput = value;
    } else if (command == String("ENABLEH")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable heater signal to output %i", value);
        config->enableHeaterOutput = value;
    } else if (command == String("HEATVALV")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting heater valve signal output %i", value);
        config->heaterValveOutput = value;
    } else if (command == String("HEATPUMP")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting heater pump signal to output %i", value);
        config->heaterPumpOutput = value;
    } else if (command == String("COOLPUMP")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting cooling pump signal to output %i", value);
        config->coolingPumpOutput = value;
    } else if (command == String("COOLFAN")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting cooling fan signal to output %i", value);
        config->coolingFanOutput = value;
    } else if (command == String("COOLON")) {
        value = constrain(value, 0, 200);
        Logger::console("Setting cooling ON temperature to: %i deg C", value);
        config->coolingTempOn = value;
    } else if (command == String("COOLOFF")) {
        value = constrain(value, 0, config->coolingTempOn);
        Logger::console("Setting cooling OFF temperature to: %i deg C", value);
        config->coolingTempOff = value;
    } else if (command == String("BRAKELT")) {
        value = constrain(value, 0, 255);
        Logger::console("Brake light signal set to output %i.", value);
        config->brakeLightOutput = value;
    } else if (command == String("REVLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Reverse light signal set to output %i.", value);
        config->reverseLightOutput = value;
    } else if (command == String("PWRSTR")) {
        value = constrain(value, 0, 255);
        Logger::console("Power steering signal set to output %i.", value);
        config->powerSteeringOutput = value;
    } else if (command == String("WARNLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Warning signal set to output %i.", value);
        config->warningOutput = value;
    } else if (command == String("LIMITLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Limit signal set to output %i.", value);
        config->powerLimitationOutput = value;
    } else if (command == String("SOCHG")) {
        value = constrain(value, 0, 255);
        Logger::console("State of charge set to output %i.", value);
        config->stateOfChargeOutput = value;
    } else if (command == String("OUTPUT") && value < 8) {
        Logger::console("DOUT%d,  STATE: %t", value, systemIO.getDigitalOut(value));
        systemIO.setDigitalOut(value, !systemIO.getDigitalOut(value));
        Logger::console("DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d, DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", systemIO.getDigitalOut(0),
                systemIO.getDigitalOut(1), systemIO.getDigitalOut(2), systemIO.getDigitalOut(3), systemIO.getDigitalOut(4), systemIO.getDigitalOut(5),
                systemIO.getDigitalOut(6), systemIO.getDigitalOut(7));
    } else {
        return false;
    }
    systemIO.saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdCharger(String command, long value)
{
    Charger *charger = deviceManager.getCharger();
    ChargerConfiguration *config = NULL;

    if (!charger) {
        return false;
    }
    config = (ChargerConfiguration *) charger->getConfiguration();

    if (command == String("CHCC")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting constant current to %fA", value / 10.0f);
        config->constantCurrent = value;
    } else if (command == String("CHCV")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting constant voltage to %fV", value / 10.0f);
        config->constantVoltage = value;
    } else if (command == String("CHTC")) {
        value = constrain(value, 0, config->constantCurrent);
        Logger::console("Setting terminate current to %fA", value / 10.0f);
        config->terminateCurrent = value;
    } else if (command == String("CHICMX")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting max input current to %fV", value / 10.0f);
        config->maximumInputCurrent = value;
    } else if (command == String("CHBVMN")) {
        value = constrain(value, 0, config->maximumBatteryVoltage);
        Logger::console("Setting min battery voltage to %fV", value / 10.0f);
        config->minimumBatteryVoltage = value;
    } else if (command == String("CHBVMX")) {
        value = constrain(value, config->minimumBatteryVoltage, 100000);
        Logger::console("Setting max battery voltage to %fV", value / 10.0f);
        config->maximumBatteryVoltage = value;
    } else if (command == String("CHTPMN")) {
        value = constrain(value, -1000, config->maximumTemperature);
        Logger::console("Setting min battery temp to %f deg C", value / 10.0f);
        config->minimumTemperature = value;
    } else if (command == String("CHTPMX")) {
        value = constrain(value, config->minimumTemperature, 10000);
        Logger::console("Setting max battery temp to %f deg C", value / 10.0f);
        config->maximumTemperature = value;
    } else if (command == String("CHAHMX")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting max Ampere hours to %fAh", value / 10.0f);
        config->maximumAmpereHours = value;
    } else if (command == String("CHCTMX")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting max charge time to %i min", value);
        config->maximumChargeTime = value;
    } else if (command == String("CHTDRC")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting derating of charge current to %fA per deg C", value / 10.0f);
        config->deratingRate = value;
    } else if (command == String("CHTDRS")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting derating reference temp to %f deg C", value / 10.0f);
        config->deratingReferenceTemperature = value;
    } else if (command == String("CHTHYS")) {
        value = constrain(value, config->hystereseResumeTemperature, 10000);
        Logger::console("Setting hysterese temp to stop charging to %f deg C", value / 10.0f);
        config->hystereseStopTemperature = value;
    } else if (command == String("CHTHYR")) {
        value = constrain(value, 0, config->hystereseStopTemperature);
        Logger::console("Setting hysterese temp to resume charging to %f deg C", value / 10.0f);
        config->hystereseResumeTemperature = value;
    } else {
        return false;
    }
    charger->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdDcDcConverter(String command, long value)
{
    DcDcConverter *dcdcConverter = deviceManager.getDcDcConverter();
    DcDcConverterConfiguration *config = NULL;

    if (!dcdcConverter) {
        return false;
    }
    config = (DcDcConverterConfiguration *) dcdcConverter->getConfiguration();

    if (command == String("DCMODE")) {
        Logger::console("Setting mode to %s", (value == 0 ? "buck" : "boost"));
        config->mode = value;
    } else if (command == String("DCBULV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck LV voltage to %fV", value / 10.0f);
        config->lowVoltageCommand = value;
    } else if (command == String("DCBULVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck LV current limit to %iA", value);
        config->lvBuckModeCurrentLimit = value;
    } else if (command == String("DCBUHVV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck HV under voltage limit to %iV", value);
        config->hvUndervoltageLimit = value;
    } else if (command == String("DCBUHVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck HV current limit to %fA", value / 10.0f);
        config->hvBuckModeCurrentLimit = value;
    } else if (command == String("DCBOHV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost HV voltage to %iV", value);
        config->highVoltageCommand = value;
    } else if (command == String("DCBOLVV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost LV undervoltage limit to %fV", value / 10.0f);
        config->lvUndervoltageLimit = value;
    } else if (command == String("DCBOLVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost LV current limit to %iA", value);
        config->lvBoostModeCurrentLinit = value;
    } else if (command == String("DCBOHVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost HV current limit to %fA", value / 10.0f);
        config->hvBoostModeCurrentLimit = value;
    } else {
        return false;
    }
    dcdcConverter->saveConfiguration();
    return true;
}

bool SerialConsole::handleConfigCmdSystem(String command, long value, char *parameter)
{

    if (command == String("ENABLE")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_ENABLE, NULL)) {
            Logger::console("Invalid device ID (%X, %d)", value, value);
        }
    } else if (command == String("DISABLE")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_DISABLE, NULL)) {
            Logger::console("Invalid device ID (%X, %d)", value, value);
        }
    } else if (command == String("SYSTYPE")) {
        systemIO.setSystemType((SystemType) constrain(value, GEVCU1, GEVCU4));
        Logger::console("System type updated. Power cycle to apply.");
    } else if (command == String("LOGLEVEL")) {
        if (strchr(parameter, ',') == NULL) {
            Logger::setLoglevel((Logger::LogLevel) value);
            Logger::console("setting loglevel to %d", value);
            systemIO.setLogLevel(Logger::getLogLevel());
        } else {
            DeviceId deviceId = (DeviceId) strtol(strtok(parameter, ","), NULL, 0);
            value = atol(strtok(NULL, ","));
            Logger::setLoglevel(deviceId, (Logger::LogLevel) value);
        }
    } else if (command == String("NUKE") && value == 1) {
        // write zero to the checksum location of every device in the table.
        uint8_t zeroVal = 0;
        for (int j = 0; j < 64; j++) {
            memCache.Write(EE_DEVICES_BASE + (EE_DEVICE_SIZE * j), zeroVal);
            memCache.FlushAllPages();
        }
        Logger::console("Device settings have been nuked. Reboot to reload default settings");
    } else {
        return false;
    }
    return true;
}

bool SerialConsole::handleConfigCmdWifi(String command, String parameter)
{
    if (command == String("WLAN")) {
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) parameter.c_str());
        Logger::info("sent \"%s%s\" to wifi device", Constants::ichipCommandPrefix, parameter.c_str());
    } else if (command == String("SSID")) {
        sendWifiCommand("WLSI=", parameter);
    } else if (command == String("IP")) {
        sendWifiCommand("DIP=", parameter);
    } else if (command == String("CHANNEL")) {
        sendWifiCommand("WLCH=", parameter);
    } else if (command == String("SECURITY")) {
        sendWifiCommand("WLPP=", parameter);
    } else if (command == String("PWD")) {
        sendWifiCommand("WPWD=", parameter);
    } else {
        return false;
    }
    deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "DOWN");

    return true;
}

void SerialConsole::sendWifiCommand(String command, String parameter)
{
    command.concat("=");
    command.concat(parameter);
    Logger::info("sent \"%s%s\" to wifi device", Constants::ichipCommandPrefix, command.c_str());
    deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) command.c_str());
}

void SerialConsole::handleShortCmd()
{
    uint8_t val;
    MotorController* motorController = (MotorController*) deviceManager.getMotorController();
    Throttle *accelerator = deviceManager.getAccelerator();
    Throttle *brake = deviceManager.getBrake();
    Heartbeat *heartbeat = (Heartbeat *) deviceManager.getDeviceByID(HEARTBEAT);

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
            memCache.Write(1000 + i, (uint8_t) i);
        }

        Logger::info("Flushing cache");
        memCache.FlushAllPages(); //write everything to eeprom
        memCache.InvalidateAll(); //remove all data from cache
        Logger::console("Operation complete.");
        break;

    case 'I':
        Logger::console("Retrieving data previously saved");

        for (int i = 0; i < 256; i++) {
            memCache.Read(1000 + i, &val);
            Logger::console("%d: %d", i, val);
        }
        break;

    case 'K': //set all outputs high
        for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
            systemIO.setDigitalOut(tout, true);
        }

        Logger::console("all outputs: ON");
        break;

    case 'J': //set the four outputs low
        for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
            systemIO.setDigitalOut(tout, false);
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
        deviceManager.printDeviceList();
        break;

    case 's':
        Logger::console("Finding and listing all nearby WiFi access points");
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "RP20");
        break;

    case 'W':
        Logger::console("Setting Wifi Adapter to WPS mode (make sure you press the WPS button on your router)");
        // restore factory defaults and give it some time
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "AWPS");
        break;

    case 'w':
        Logger::console("Resetting wifi to factory defaults and setting up to auto connect to open APs, this takes about 50sec, please stand-by");
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_RESET, NULL);
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL); // reload configuration params as they were lost
        Logger::console("Wifi 4.2 initialized");
        break;

    case 'X':
        setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
        break;
    }
}
