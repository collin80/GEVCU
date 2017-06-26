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
    Logger::console("\n%s (build: %d)", CFG_VERSION, CFG_BUILD_NUM);
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
    Logger::console("w = reset wifi to factory defaults, setup GEVCU ad-hoc network");
    Logger::console("W = activate wifi WPS mode for pairing");
    Logger::console("s = Scan WiFi for nearby access points");

    Logger::console("\nConfig Commands (enter command=newvalue)\n");
    Logger::console("LOGLEVEL=[deviceId,]%d - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", Logger::getLogLevel());
    Logger::console("SYSTYPE=%d - Set board revision (Dued=2, GEVCU3=3, GEVCU4=4)", systemIO.getSystemType());
    Logger::console("ECONS=%.2f - kiloWatt Hours of energy used", status.getEnergyConsumption() / 10.0f);
    Logger::console("WLAN - send a AT+i command to the wlan device");
    Logger::console("NUKE=1 - Resets all device settings in EEPROM. You have been warned.");
    Logger::console("KILL=... - kill a device temporarily (until reboot)\n");

    deviceManager.printDeviceList();

    printMenuMotorController();
    printMenuThrottle();
    printMenuBrake();
    printMenuSystemIO();
    printMenuCharger();
    printMenuDcDcConverter();
    printMenuCanOBD2();
}

void SerialConsole::printMenuMotorController()
{
    MotorController* motorController = (MotorController*) deviceManager.getMotorController();

    if (motorController && motorController->getConfiguration()) {
        MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();
        Logger::console("\nMOTOR CONTROLS\n");
        Logger::console("TORQ=%d - Set torque upper limit (in 0.1Nm)", config->torqueMax);
        Logger::console("RPMS=%d - Set maximum speed (in RPMs)", config->speedMax);
        Logger::console("MOMODE=%d - Set power mode (0=torque, 1=speed, default=0)", config->torqueMax);
        Logger::console("CRLVL=%d - Torque to use for creep (0=disable) (in 1%%)", config->creepLevel);
        Logger::console("CRSPD=%d - maximal speed until creep is applied (in RPMs)", config->creepSpeed);
        Logger::console("REVLIM=%d - How much torque to allow in reverse (in 0.1%%)", config->reversePercent);
        Logger::console("MOINVD=%d - invert the direction of the motor (0=normal, 1=invert)", config->invertDirection);
        Logger::console("MOSLEW=%d - slew rate (in 0.1 percent/sec, 0=disabled)", config->slewRate);
        Logger::console("MOMWMX=%d - maximal mechanical power of motor (in 100W steps)", config->maxMechanicalPowerMotor);
        Logger::console("MORWMX=%d - maximal mechanical power of regen (in 100W steps)", config->maxMechanicalPowerRegen);
        Logger::console("MOBRHD=%d - percentage of max torque to apply for brake hold (in 1%%)", config->brakeHold);
        Logger::console("MOBRHQ=%d - coefficient for brake hold, the higher the smoother brake hold force will be applied (1-255, 10=default)", config->brakeHoldForceCoefficient);
        Logger::console("MOGCHS=%d - enable gear change support (1=aproximate rpm at gear shift, 0=off)", config->gearChangeSupport);
        Logger::console("NOMV=%d - Fully charged pack voltage that automatically resets the kWh counter (in 0.1V)", config->nominalVolt);
        if (motorController->getId() == BRUSA_DMC5) {
            BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
            Logger::console("MOMVMN=%d - minimum DC voltage limit for motoring (in 0.1V)", dmc5Config->dcVoltLimitMotor);
            Logger::console("MOMCMX=%d - current limit for motoring (in 0.1A)", dmc5Config->dcCurrentLimitMotor);
            Logger::console("MORVMX=%d - maximum DC voltage limit for regen (in 0.1V)", dmc5Config->dcVoltLimitRegen);
            Logger::console("MORCMX=%d - current limit for regen (in 0.1A)", dmc5Config->dcCurrentLimitRegen);
            Logger::console("MOOSC=%d - enable the DMC5 oscillation limiter (1=enable, 0=disable, also set DMC parameter!)",
                    dmc5Config->enableOscillationLimiter);
        }
    }
}

PowerMode powerMode;


void SerialConsole::printMenuThrottle()
{
    Throttle *accelerator = deviceManager.getAccelerator();

    if (accelerator && accelerator->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) accelerator->getConfiguration();
        Logger::console("\nTHROTTLE CONTROLS\n");
        if (accelerator->getId() == POTACCELPEDAL) {
            PotThrottleConfiguration *potConfig = (PotThrottleConfiguration *) config;
            Logger::console("TPOT=%d - Number of pots to use (1 or 2)", potConfig->numberPotMeters);
            Logger::console("TTYPE=%d - Set throttle subtype (1=std linear, 2=inverse)", potConfig->throttleSubType);
            Logger::console("T1MN=%d - Set throttle 1 min value", potConfig->minimumLevel);
            Logger::console("T1MX=%d - Set throttle 1 max value", potConfig->maximumLevel);
            Logger::console("T1ADC=%d - Set throttle 1 ADC pin", potConfig->AdcPin1);
            Logger::console("T2MN=%d - Set throttle 2 min value", potConfig->minimumLevel2);
            Logger::console("T2MX=%d - Set throttle 2 max value", potConfig->maximumLevel2);
            Logger::console("T2ADC=%d - Set throttle 2 ADC pin", potConfig->AdcPin2);
        }
        if (accelerator->getId() == CANACCELPEDAL) {
            CanThrottleConfiguration *canConfig = (CanThrottleConfiguration *) config;
            Logger::console("T1MN=%d - Set throttle 1 min value", canConfig->minimumLevel);
            Logger::console("T1MX=%d - Set throttle 1 max value", canConfig->maximumLevel);
            Logger::console("TCTP=%d - Set car type", canConfig->carType);
        }
        Logger::console("TRGNMAX=%d - Pedal position where regen is at max (in 0.1%%)", config->positionRegenMaximum);
        Logger::console("TRGNMIN=%d - Pedal position where regen is at min (in 0.1%%)", config->positionRegenMinimum);
        Logger::console("TFWD=%d - Pedal position where forward motion starts  (in 0.1%%)", config->positionForwardMotionStart);
        Logger::console("TMAP=%d - Pedal position of 50%% torque (in 0.1%%)", config->positionHalfPower);
        Logger::console("TMINRN=%d - Torque to use for min throttle regen (in 1%%)", config->minimumRegen);
        Logger::console("TMAXRN=%d - Torque to use for max throttle regen (in 1%%)", config->maximumRegen);
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
            Logger::console("B1ADC=%d - Set brake ADC pin", potConfig->AdcPin1);
        }
        if (brake->getId() == CANBRAKEPEDAL) {
            CanBrakeConfiguration *canConfig = (CanBrakeConfiguration *) config;
            Logger::console("BCTP=%d - Set car type", canConfig->carType);
        }
        Logger::console("B1MN=%d - Set brake min value", config->minimumLevel);
        Logger::console("B1MX=%d - Set brake max value", config->maximumLevel);
        Logger::console("BMINR=%d - Torque for start of brake regen (in 1%%)", config->minimumRegen);
        Logger::console("BMAXR=%d - Torque for maximum brake regen (in 1%%)", config->maximumRegen);
    }
}

void SerialConsole::printMenuSystemIO()
{
    SystemIOConfiguration *config = systemIO.getConfiguration();

    if (config) {
        Logger::console("\nSYSTEM I/O\n");
        Logger::console("ENABLEI=%d - Digital input to use for enable signal (255 to disable)", config->enableInput);
        Logger::console("CHARGEI=%d - Digital input to use for charger signal (255 to disable)", config->chargePowerAvailableInput);
        Logger::console("INTERLI=%d - Digital input to use for interlock signal (255 to disable)", config->interlockInput);
        Logger::console("REVIN=%d - Digital input to reverse motor rotation (255 to disable)\n", config->reverseInput);
        Logger::console("ABSIN=%d - Digital input to indicate active ABS system (255 to disable)\n", config->absInput);

        Logger::console("PREDELAY=%d - Precharge delay time (in milliseconds)", config->prechargeMillis);
        Logger::console("PRELAY=%d - Digital output to use for precharge contactor (255 to disable)", config->prechargeRelayOutput);
        Logger::console("MRELAY=%d - Digital output to use for main contactor (255 to disable)", config->mainContactorOutput);
        Logger::console("NRELAY=%d - Digital output to use for secondary contactor (255 to disable)", config->secondaryContactorOutput);
        Logger::console("FRELAY=%d - Digital output to use for fast charge contactor (255 to disable)\n", config->fastChargeContactorOutput);

        Logger::console("ENABLEM=%d - Digital output to use for enable motor signal (255 to disable)", config->enableMotorOutput);
        Logger::console("ENABLEC=%d - Digital output to use for enable charger signal (255 to disable)", config->enableChargerOutput);
        Logger::console("ENABLED=%d - Digital output to use for enable dc-dc converter signal (255 to disable)", config->enableDcDcOutput);
        Logger::console("ENABLEH=%d - Digital output to use for enable heater signal (255 to disable)\n", config->enableHeaterOutput);
        Logger::console("HEATERON=%d - external temperature below which heater is turned on (0 - 40 deg C, 255 = ignore)", config->heaterTemperatureOn);

        Logger::console("HEATVALV=%d - Digital output to actuate heater valve (255 to disable)", config->heaterValveOutput);
        Logger::console("HEATPUMP=%d - Digital output to turn on heater pump (255 to disable)", config->heaterPumpOutput);
        Logger::console("COOLPUMP=%d - Digital output to turn on cooling pump (255 to disable)", config->coolingPumpOutput);
        Logger::console("COOLFAN=%d - Digital output to turn on cooling fan (255 to disable)", config->coolingFanOutput);
        Logger::console("COOLON=%d - Controller temperature to turn cooling on (deg celsius)", config->coolingTempOn);
        Logger::console("COOLOFF=%d - Controller temperature to turn cooling off (deg celsius)\n", config->coolingTempOff);

        Logger::console("BRAKELT=%d - Digital output to use for brake light (255 to disable)", config->brakeLightOutput);
        Logger::console("REVLT=%d - Digital output to use for reverse light (255 to disable)", config->reverseLightOutput);
        Logger::console("PWRSTR=%d - Digital output to use to enable power steering (255 to disable)", config->powerSteeringOutput);
//        Logger::console("TBD=%d - Digital output to use to xxxxxx (255 to disable)", config->unusedOutput);

        Logger::console("WARNLT=%d - Digital output to use for reverse light (255 to disable)", config->warningOutput);
        Logger::console("LIMITLT=%d - Digital output to use for limitation indicator (255 to disable)", config->powerLimitationOutput);
        Logger::console("SOCHG=%d - Analog output to use to indicate state of charge (255 to disable)", config->stateOfChargeOutput);
        Logger::console("STLGT=%d - Analog output to use to indicate system status (255 to disable)", config->statusLightOutput);
        Logger::console("OUTPUT=<0-7> - toggles state of specified digital output");
    }
}

void SerialConsole::printMenuCharger()
{
    Charger *charger = deviceManager.getCharger();

    if (charger && charger->getConfiguration()) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();
        Logger::console("\nCHARGER CONTROLS\n");
        Logger::console("CHCC=%d - Constant current (in 0.1A)", config->constantCurrent);
        Logger::console("CHCV=%d - Constant voltage (in 0.1V)", config->constantVoltage);
        Logger::console("CHTC=%d - Terminate current (in 0.1A)", config->terminateCurrent);
        Logger::console("CHICMX=%d - Maximum Input current (in 0.1A)", config->maximumInputCurrent);
        Logger::console("CHBVMN=%d - Minimum battery voltage (in 0.1V)", config->minimumBatteryVoltage);
        Logger::console("CHBVMX=%d - Maximum battery voltage (in 0.1V)", config->maximumBatteryVoltage);
        Logger::console("CHTPMN=%d - Minimum battery temperature for charging (in 0.1 deg C)", config->minimumTemperature);
        Logger::console("CHTPMX=%d - Maximum battery temperature for charging (in 0.1 deg C)", config->maximumTemperature);
        Logger::console("CHAHMX=%d - Maximum ampere hours (in 0.1Ah)", config->maximumAmpereHours);
        Logger::console("CHCTMX=%d - Maximum charge time (in 1 min)", config->maximumChargeTime);
        Logger::console("CHTDRC=%d - Derating of charge current (in 0.1A per deg C)", config->deratingRate);
        Logger::console("CHTDRS=%d - Reference temperature for derating (in 0.1 deg C)", config->deratingReferenceTemperature);
        Logger::console("CHTHYS=%d - Hysterese temperature where charging will be stopped (in 0.1 deg C)", config->hystereseStopTemperature);
        Logger::console("CHTHYR=%d - Hysterese temperature where charging will resume (in 0.1 deg C)", config->hystereseResumeTemperature);
    }
}

void SerialConsole::printMenuDcDcConverter()
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter && dcDcConverter->getConfiguration()) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();
        Logger::console("\nDCDC CONVERTER CONTROLS\n");
        Logger::console("DCMODE=%d - operation mode (0 = buck/default, 1 = boost)", config->mode);
        Logger::console("DCBULV=%d - Buck mode LV voltage (in 0.1V)", config->lowVoltageCommand);
        Logger::console("DCBULVC=%d - Buck mode LV current limit (in 1A)", config->lvBuckModeCurrentLimit);
        Logger::console("DCBUHVV=%d - Buck mode HV under voltage limit (in 1 V)", config->hvUndervoltageLimit);
        Logger::console("DCBUHVC=%d - Buck mode HV current limit (in 0.1A)", config->hvBuckModeCurrentLimit);
        Logger::console("DCBOHV=%d - Boost mode HV voltage (in 1V)", config->highVoltageCommand);
        Logger::console("DCBOLVV=%d - Boost mode LV under voltage limit (in 0.1V)", config->lvUndervoltageLimit);
        Logger::console("DCBOLVC=%d - Boost mode LV current limit (in 1A)", config->lvBoostModeCurrentLinit);
        Logger::console("DCBOHVC=%d - Boost mode HV current limit (in 0.1A)", config->hvBoostModeCurrentLimit);
        if (dcDcConverter->getId() == BRUSA_BSC6) {
            BrusaBSC6Configuration *bscConfig = (BrusaBSC6Configuration *) config;
            Logger::console("DCDBG=%d - Enable debug mode of BSC6 (0=off,1=on)", bscConfig->debugMode);
        }
    }
}

void SerialConsole::printMenuCanOBD2()
{
    CanOBD2 *canObd2 = (CanOBD2 *)deviceManager.getDeviceByID(CANOBD2);

    if (canObd2 && canObd2->isEnabled() && canObd2->getConfiguration()) {
        CanOBD2Configuration *config = (CanOBD2Configuration *) canObd2->getConfiguration();
        Logger::console("\nCAN OBD2 CONTROLS\n");
        Logger::console("ODBRES=%d - can bus number on which to respond to ODB2 PID requests (0=EV bus, 1=car bus, default=0)", config->canBusRespond);
        Logger::console("ODBRESO=%d - offset to can ID 0x7e8 to respond to OBD2 PID requests (0-7, default=0)", config->canIdOffsetRespond);
        Logger::console("OBDPOL=%d - can bus number on which we poll data from the car (0=EV, 1=car, default=1)", config->canBusPoll);
        Logger::console("OBDPOLO=%d - offset to can ID 0x7e0 to request ODB2 data (0-7, 255=broadcast, default = 255)", config->canIdOffsetPoll);
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

    //Logger::debug("Cmd size: %d", ptrBuffer);
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
            Logger::warn("unknown command: %s", command.c_str());
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
        Logger::console("Setting speed limit to %drpm", value);
        config->speedMax = value;
    } else if (command == String("MOMODE")) {
        value = constrain(value, 0, 1);
        Logger::console("Setting power mode to %s", (value == 0 ? "torque" : "speed (be careful !!)"));
    } else if (command == String("CRLVL")) {
        value = constrain(value, 0, 100);
        Logger::console("Setting creep level to %d%%", value);
    } else if (command == String("CRSPD")) {
        value = constrain(value, 0, config->speedMax);
        Logger::console("Setting creep speed to %drpm", value);
        config->creepSpeed = value;
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
    } else if (command == String("MOBRHD")) {
        Logger::console("Setting maximal brake hold level to %d%%", value);
        config->brakeHold = value;
    } else if (command == String("MOBRHQ")) {
        value = constrain(value, 1, 255);
        Logger::console("Setting brake hold force coefficient to %d", value);
        config->brakeHoldForceCoefficient = value;
    } else if (command == String("MOGCHS")) {
        value = constrain(value, 0, 1);
        Logger::console("Setting gear change support to '%s'", (value == 1 ? "on" : "off"));
        config->gearChangeSupport = value;
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
        Logger::console("Setting # of throttle pots to %d", value);
        ((PotThrottleConfiguration *) config)->numberPotMeters = value;
    } else if (command == String("TTYPE") && (throttle->getId() == POTACCELPEDAL)) {
        value = constrain(value, 1, 2);
        Logger::console("Setting throttle subtype to %s", (value == 2 ? "inverse" : "std linear"));
        ((PotThrottleConfiguration *) config)->throttleSubType = value;
    } else if (command == String("T1ADC") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting Throttle1 ADC pin to %d", value);
        ((PotThrottleConfiguration *) config)->AdcPin1 = value;
    } else if (command == String("T2ADC") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting Throttle2 ADC pin to %d", value);
        ((PotThrottleConfiguration *) config)->AdcPin2 = value;
    } else if (command == String("T2MN") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 min to %d", value);
        ((PotThrottleConfiguration *) config)->minimumLevel2 = value;
    } else if (command == String("T2MX") && (throttle->getId() == POTACCELPEDAL)) {
        Logger::console("Setting throttle 2 max to %d", value);
        ((PotThrottleConfiguration *) config)->maximumLevel2 = value;
    } else if (command == String("TCTP") && (throttle->getId() == CANACCELPEDAL)) {
        Logger::console("Setting car type to %d", value);
        ((CanThrottleConfiguration *) config)->carType = value;
    } else if (command == String("T1MN")) {
        Logger::console("Setting throttle 1 min to %d", value);
        config->minimumLevel = value;
    } else if (command == String("T1MX")) {
        Logger::console("Setting throttle 1 max to %d", value);
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
        Logger::console("Setting throttle regen minimum strength to %d%%", value);
        config->minimumRegen = value;
    } else if (command == String("TMAXRN")) {
        value = constrain(value, config->minimumRegen, 100);
        Logger::console("Setting throttle Regen maximum strength to %d%%", value);
        config->maximumRegen = value;
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

    if (command == String("BCTP") && (brake->getId() == CANBRAKEPEDAL)) {
        Logger::console("Setting brake car type to %d", value);
        ((CanBrakeConfiguration *) config)->carType = value;
    } else if (command == String("B1MN")) {
        Logger::console("Setting brake min to %d", value);
        config->minimumLevel = value;
    } else if (command == String("B1MX")) {
        Logger::console("Setting brake max to %d", value);
        config->maximumLevel = value;
    } else if (command == String("BMINR")) {
        value = constrain(value, 0, config->maximumRegen);
        Logger::console("Setting min brake regen to %d%%", value);
        config->minimumRegen = value;
    } else if (command == String("BMAXR")) {
        value = constrain(value, config->minimumRegen, 100);
        Logger::console("Setting max brake regen to %d%%", value);
        config->maximumRegen = value;
    } else if (command == String("B1ADC") && (brake->getId() == POTBRAKEPEDAL)) {
        Logger::console("Setting Brake ADC pin to %d", value);
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
            Logger::console("Setting enable signal to input %d.", value);
            config->enableInput = value;
        } else {
            Logger::console("Invalid enable signal input number. Please enter a value 0 - %d", CFG_NUMBER_DIGITAL_INPUTS - 1);
        }
    } else if (command == String("CHARGEI")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting charge signal to input %d", value);
        config->chargePowerAvailableInput = value;
    } else if (command == String("INTERLI")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting interlock signal to input %d", value);
        config->interlockInput = value;
    } else if (command == String("REVIN")) {
        config->reverseInput = value;
        Logger::console("Motor reverse signal set to input %d.", value);
    } else if (command == String("ABSIN")) {
        config->absInput = value;
        Logger::console("ABS signal set to input %d.", value);
    } else if (command == String("PREDELAY")) {
        value = constrain(value, 0, 100000);
        Logger::console("Setting precharge time to %dms", value);
        config->prechargeMillis = value;
    } else if (command == String("PRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting precharge relay to output %d", value);
        config->prechargeRelayOutput = value;
    } else if (command == String("MRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting main contactor to output %d", value);
        config->mainContactorOutput = value;
    } else if (command == String("NRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting secondary contactor to output %d", value);
        config->secondaryContactorOutput = value;
    } else if (command == String("FRELAY")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting fast charge contactor to output %d", value);
        config->fastChargeContactorOutput = value;
    } else if (command == String("ENABLEM")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable motor signal to output %d", value);
        config->enableMotorOutput = value;
    } else if (command == String("ENABLEC")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable charger signal to output %d", value);
        config->enableChargerOutput = value;
    } else if (command == String("ENABLED")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable DC-DC converter signal to output %d", value);
        config->enableDcDcOutput = value;
    } else if (command == String("ENABLEH")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting enable heater signal to output %d", value);
        config->enableHeaterOutput = value;
    } else if (command == String("HEATVALV")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting heater valve signal output %d", value);
        config->heaterValveOutput = value;
    } else if (command == String("HEATERON")) {
        value = constrain(value, 0, 40);
        Logger::console("Setting enable heater temp on to %d deg C", value);
        config->heaterTemperatureOn = value;
    } else if (command == String("HEATPUMP")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting heater pump signal to output %d", value);
        config->heaterPumpOutput = value;
    } else if (command == String("COOLPUMP")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting cooling pump signal to output %d", value);
        config->coolingPumpOutput = value;
    } else if (command == String("COOLFAN")) {
        value = constrain(value, 0, 255);
        Logger::console("Setting cooling fan signal to output %d", value);
        config->coolingFanOutput = value;
    } else if (command == String("COOLON")) {
        value = constrain(value, 0, 200);
        Logger::console("Setting cooling ON temperature to: %d deg C", value);
        config->coolingTempOn = value;
    } else if (command == String("COOLOFF")) {
        value = constrain(value, 0, config->coolingTempOn);
        Logger::console("Setting cooling OFF temperature to: %d deg C", value);
        config->coolingTempOff = value;
    } else if (command == String("BRAKELT")) {
        value = constrain(value, 0, 255);
        Logger::console("Brake light signal set to output %d.", value);
        config->brakeLightOutput = value;
    } else if (command == String("REVLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Reverse light signal set to output %d.", value);
        config->reverseLightOutput = value;
    } else if (command == String("PWRSTR")) {
        value = constrain(value, 0, 255);
        Logger::console("Power steering signal set to output %d.", value);
        config->powerSteeringOutput = value;
    } else if (command == String("WARNLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Warning signal set to output %d.", value);
        config->warningOutput = value;
    } else if (command == String("LIMITLT")) {
        value = constrain(value, 0, 255);
        Logger::console("Limit signal set to output %d.", value);
        config->powerLimitationOutput = value;
    } else if (command == String("SOCHG")) {
        value = constrain(value, 0, 255);
        Logger::console("State of charge set to output %d.", value);
        config->stateOfChargeOutput = value;
    } else if (command == String("STLGT")) {
        value = constrain(value, 0, 255);
        Logger::console("Statu light set to output %d.", value);
        config->statusLightOutput = value;
    } else if (command == String("OUTPUT") && value < 8) {
        Logger::console("DOUT%d,  STATE: %d", value, systemIO.getDigitalOut(value));
        systemIO.setDigitalOut(value, !systemIO.getDigitalOut(value));
        Logger::console("DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d, DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", systemIO.getDigitalOut(0),
                systemIO.getDigitalOut(1), systemIO.getDigitalOut(2), systemIO.getDigitalOut(3), systemIO.getDigitalOut(4), systemIO.getDigitalOut(5),
                systemIO.getDigitalOut(6), systemIO.getDigitalOut(7));
    } else if (command == String("ECONS")) {
        value = constrain(value, 0, 500);
        Logger::console("energy consumption set to %fkwh.", value / 10.0f);
        status.energyConsumption = value * 360000;
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
        Logger::console("Setting max charge time to %d min", value);
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
        Logger::console("Setting buck LV current limit to %dA", value);
        config->lvBuckModeCurrentLimit = value;
    } else if (command == String("DCBUHVV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck HV under voltage limit to %dV", value);
        config->hvUndervoltageLimit = value;
    } else if (command == String("DCBUHVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting buck HV current limit to %fA", value / 10.0f);
        config->hvBuckModeCurrentLimit = value;
    } else if (command == String("DCBOHV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost HV voltage to %dV", value);
        config->highVoltageCommand = value;
    } else if (command == String("DCBOLVV")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost LV undervoltage limit to %fV", value / 10.0f);
        config->lvUndervoltageLimit = value;
    } else if (command == String("DCBOLVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost LV current limit to %dA", value);
        config->lvBoostModeCurrentLinit = value;
    } else if (command == String("DCBOHVC")) {
        value = constrain(value, 0, 10000);
        Logger::console("Setting boost HV current limit to %fA", value / 10.0f);
        config->hvBoostModeCurrentLimit = value;
    } else if (command == String("DCDBG") && dcdcConverter->getId() == BRUSA_BSC6) {
        BrusaBSC6Configuration *bscConfig = (BrusaBSC6Configuration *) config;
        value = constrain(value, 0, 1);
        Logger::console("Setting BSC6 debug mode %d", value);
        bscConfig->debugMode = value;
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
            Logger::console("Invalid device ID (%#x, %d)", value, value);
        }
    } else if (command == String("DISABLE")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_DISABLE, NULL)) {
            Logger::console("Invalid device ID (%#x, %d)", value, value);
        }
    } else if (command == String("KILL")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_KILL, NULL)) {
            Logger::console("Invalid device ID (%#x, %d)", value, value);
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
            Device *device = deviceManager.getDeviceByID(deviceId);
            if (device != NULL) {
                value = atol(strtok(NULL, ","));
                Logger::setLoglevel(device, (Logger::LogLevel) value);
            }
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

bool SerialConsole::handleConfigCmdCanOBD2(String command, long value)
{
    CanOBD2 *canObd2 = (CanOBD2 *) deviceManager.getDeviceByID(CANOBD2);
    CanOBD2Configuration *config = NULL;

    if (!canObd2) {
        return false;
    }
    config = (CanOBD2Configuration *) canObd2->getConfiguration();

    if (command == String("ODBRES")) {
        value = constrain(value, 0, 1);
        Logger::console("Setting listener can bus to %d", value);
        config->canBusRespond = value;
    } else if (command == String("ODBRESO")) {
        value = constrain(value, 0, 7);
        Logger::console("Setting respond can ID offset to %d", value);
        config->canIdOffsetRespond = value;
    } else if (command == String("OBDPOL")) {
        value = constrain(value, 0, 1);
        Logger::console("Setting query can bus to %d", value);
        config->canBusPoll = value;
    } else if (command == String("OBDPOLO")) {
        if (value != 255) {
            value = constrain(value, 0, 7);
        }
        Logger::console("Setting request can ID to %d", value);
        config->canIdOffsetPoll = value;
    } else {
        return false;
    }
    canObd2->saveConfiguration();
    return true;
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
        Logger::console("Resetting wifi to factory defaults and setting up AP, this takes about 50sec, please stand-by");
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_RESET, NULL);
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL); // reload configuration params as they were lost
        Logger::console("Wifi initialized");
        break;

    case 'X':
        setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
        break;
    }
}
