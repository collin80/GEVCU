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
    logger.console("\n%s (build: %d)", CFG_VERSION, CFG_BUILD_NUM);
    logger.console("System State: %s", status.systemStateToStr(status.getSystemState()).c_str());
    logger.console("System Menu:\n");
    logger.console("Enable line endings of some sort (LF, CR, CRLF)\n");
    logger.console("Short Commands:");
    logger.console("h = help (displays this message)");
    Device *heartbeat = deviceManager.getDeviceByID(HEARTBEAT);
    if (heartbeat != NULL && heartbeat->isEnabled()) {
        logger.console("L = show raw analog/digital input/output values (toggle)");
    }
    logger.console("K = set all outputs high");
    logger.console("J = set all outputs low");
    //logger.console("U,I = test EEPROM routines");
    logger.console("z = detect throttle min/max, num throttles and subtype");
    logger.console("Z = save throttle values");
    logger.console("b = detect brake min/max");
    logger.console("B = save brake values");
    logger.console("p = enable wifi passthrough (reboot required to resume normal operation)");
    logger.console("S = show list of devices");
    logger.console("w = reset wifi to factory defaults, setup GEVCU ad-hoc network");
    logger.console("W = activate wifi WPS mode for pairing");
    logger.console("s = Scan WiFi for nearby access points");
    logger.console("P = perform pre-charge measurement");

    logger.console("\nConfig Commands (enter command=newvalue)\n");
    logger.console("LOGLEVEL=[deviceId,]%d - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", logger.getLogLevel());
    logger.console("SYSTYPE=%d - Set board revision (Dued=2, GEVCU3=3, GEVCU4=4)", systemIO.getSystemType());
    logger.console("WLAN - send a AT+i command to the wlan device");
    logger.console("NUKE=1 - Resets all device settings in EEPROM. You have been warned.");
    logger.console("KILL=... - kill a device temporarily (until reboot)\n");

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
        logger.console("\nMOTOR CONTROLS\n");
        logger.console("TORQ=%d - Set torque upper limit (in 0.1Nm)", config->torqueMax);
        logger.console("RPMS=%d - Set maximum speed (in RPMs)", config->speedMax);
        logger.console("MOMODE=%d - Set power mode (0=torque, 1=speed, default=0)", config->powerMode);
        logger.console("CRLVL=%d - Torque to use for creep (0=disable) (in 1%%)", config->creepLevel);
        logger.console("CRSPD=%d - maximal speed until creep is applied (in RPMs)", config->creepSpeed);
        logger.console("REVLIM=%d - How much torque to allow in reverse (in 0.1%%)", config->reversePercent);
        logger.console("MOINVD=%d - invert the direction of the motor (0=normal, 1=invert)", config->invertDirection);
        logger.console("MOSLEWM=%d - slew rate for motoring (in 0.1 percent/sec, 0=disabled)", config->slewRateMotor);
        logger.console("MOSLEWR=%d - slew rate for regen (in 0.1 percent/sec, 0=disabled)", config->slewRateRegen);
        logger.console("MOMWMX=%d - maximal mechanical power of motor (in 100W steps)", config->maxMechanicalPowerMotor);
        logger.console("MORWMX=%d - maximal mechanical power of regen (in 100W steps)", config->maxMechanicalPowerRegen);
        logger.console("MOBRHD=%d - percentage of max torque to apply for brake hold (in 1%%)", config->brakeHold);
        logger.console("MOBRHQ=%d - coefficient for brake hold, the higher the smoother brake hold force will be applied (1-255, 10=default)", config->brakeHoldForceCoefficient);
        logger.console("NOMV=%d - Fully charged pack voltage that automatically resets the kWh counter (in 0.1V)", config->nominalVolt);
        logger.console("CRUISEP=%f - Kp value for cruise control (default 1.0, entered as 1000)", config->cruiseKp);
        logger.console("CRUISEI=%f - Ki value for cruise control (default 0.2, entered as 200)", config->cruiseKi);
        logger.console("CRUISED=%f - Kd value for cruise control (default 0.1, enteres as 100)", config->cruiseKd);
        logger.console("CRUISEL=%d - Delta in rpm/kph to actual speed while pressing +/- button > 1sec (default 500)", config->cruiseLongPressDelta);
        logger.console("CRUISES=%d - Delta in rpm/kph to target speed when pressing +/- button < 1 sec (default 300)", config->cruiseStepDelta);
        logger.console("CRUISER=%d - use rpm or vehicle speed to control cruise speed (default 1, 1=rpm/0=speed)", config->cruiseUseRpm);
        if (motorController->getId() == BRUSA_DMC5) {
            BrusaDMC5Configuration *dmc5Config = (BrusaDMC5Configuration *) config;
            logger.console("MOMVMN=%d - minimum DC voltage limit for motoring (in 0.1V)", dmc5Config->dcVoltLimitMotor);
            logger.console("MOMCMX=%d - current limit for motoring (in 0.1A)", dmc5Config->dcCurrentLimitMotor);
            logger.console("MORVMX=%d - maximum DC voltage limit for regen (in 0.1V)", dmc5Config->dcVoltLimitRegen);
            logger.console("MORCMX=%d - current limit for regen (in 0.1A)", dmc5Config->dcCurrentLimitRegen);
            logger.console("MOOSC=%d - enable the DMC5 oscillation limiter (1=enable, 0=disable, also set DMC parameter!)",
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
        logger.console("\nTHROTTLE CONTROLS\n");
        if (accelerator->getId() == POTACCELPEDAL) {
            PotThrottleConfiguration *potConfig = (PotThrottleConfiguration *) config;
            logger.console("TPOT=%d - Number of pots to use (1 or 2)", potConfig->numberPotMeters);
            logger.console("TTYPE=%d - Set throttle subtype (1=std linear, 2=inverse)", potConfig->throttleSubType);
            logger.console("T1MN=%d - Set throttle 1 min value", potConfig->minimumLevel);
            logger.console("T1MX=%d - Set throttle 1 max value", potConfig->maximumLevel);
            logger.console("T1ADC=%d - Set throttle 1 ADC pin", potConfig->AdcPin1);
            logger.console("T2MN=%d - Set throttle 2 min value", potConfig->minimumLevel2);
            logger.console("T2MX=%d - Set throttle 2 max value", potConfig->maximumLevel2);
            logger.console("T2ADC=%d - Set throttle 2 ADC pin", potConfig->AdcPin2);
        }
        if (accelerator->getId() == CANACCELPEDAL) {
            CanThrottleConfiguration *canConfig = (CanThrottleConfiguration *) config;
            logger.console("T1MN=%d - Set throttle 1 min value", canConfig->minimumLevel);
            logger.console("T1MX=%d - Set throttle 1 max value", canConfig->maximumLevel);
        }
        logger.console("TRGNMAX=%d - Pedal position where regen is at max (in 0.1%%)", config->positionRegenMaximum);
        logger.console("TRGNMIN=%d - Pedal position where regen is at min (in 0.1%%)", config->positionRegenMinimum);
        logger.console("TFWD=%d - Pedal position where forward motion starts  (in 0.1%%)", config->positionForwardMotionStart);
        logger.console("TMAP=%d - Pedal position of 50%% torque (in 0.1%%)", config->positionHalfPower);
        logger.console("TMINRN=%d - Torque to use for min throttle regen (in 1%%)", config->minimumRegen);
        logger.console("TMAXRN=%d - Torque to use for max throttle regen (in 1%%)", config->maximumRegen);
    }
}

void SerialConsole::printMenuBrake()
{
    Throttle *brake = deviceManager.getBrake();

    if (brake && brake->getConfiguration()) {
        ThrottleConfiguration *config = (ThrottleConfiguration *) brake->getConfiguration();
        logger.console("\nBRAKE CONTROLS\n");
        if (brake->getId() == POTBRAKEPEDAL) {
            PotBrakeConfiguration *potConfig = (PotBrakeConfiguration *) config;
            logger.console("B1ADC=%d - Set brake ADC pin", potConfig->AdcPin1);
        }
        logger.console("B1MN=%d - Set brake min value", config->minimumLevel);
        logger.console("B1MX=%d - Set brake max value", config->maximumLevel);
        logger.console("BMINR=%d - Torque for start of brake regen (in 1%%)", config->minimumRegen);
        logger.console("BMAXR=%d - Torque for maximum brake regen (in 1%%)", config->maximumRegen);
    }
}

void SerialConsole::printMenuSystemIO()
{
    SystemIOConfiguration *config = systemIO.getConfiguration();

    if (config) {
        logger.console("\nSYSTEM I/O\n");
        logger.console("CTP=%d - Set car type (0=OBD2 compatible, 1=Volvo S80, 2=Volvo V50)", config->carType);
        logger.console("ENABLEI=%d - Digital input to use for enable signal (255 to disable)", config->enableInput);
        logger.console("CHARGEI=%d - Digital input to use for charger signal (255 to disable)", config->chargePowerAvailableInput);
        logger.console("INTERLI=%d - Digital input to use for interlock signal (255 to disable)", config->interlockInput);
        logger.console("REVIN=%d - Digital input to reverse motor rotation (255 to disable)\n", config->reverseInput);
        logger.console("ABSIN=%d - Digital input to indicate active ABS system (255 to disable)\n", config->absInput);
        logger.console("GEARIN=%d - Digital input to indicate active gear change (255 to disable)\n", config->gearChangeInput);

        logger.console("PREDELAY=%d - Precharge delay time (in milliseconds)", config->prechargeMillis);
        logger.console("PRELAY=%d - Digital output to use for precharge contactor (255 to disable)", config->prechargeRelayOutput);
        logger.console("MRELAY=%d - Digital output to use for main contactor (255 to disable)", config->mainContactorOutput);
        logger.console("NRELAY=%d - Digital output to use for secondary contactor (255 to disable)", config->secondaryContactorOutput);
        logger.console("FRELAY=%d - Digital output to use for fast charge contactor (255 to disable)\n", config->fastChargeContactorOutput);

        logger.console("ENABLEM=%d - Digital output to use for enable motor signal (255 to disable)", config->enableMotorOutput);
        logger.console("ENABLEC=%d - Digital output to use for enable charger signal (255 to disable)", config->enableChargerOutput);
        logger.console("ENABLED=%d - Digital output to use for enable dc-dc converter signal (255 to disable)", config->enableDcDcOutput);
        logger.console("ENABLEH=%d - Digital output to use for enable heater signal (255 to disable)\n", config->enableHeaterOutput);
        logger.console("HEATERON=%d - external temperature below which heater is turned on (0 - 40 deg C, 255 = ignore)", config->heaterTemperatureOn);

        logger.console("HEATVALV=%d - Digital output to actuate heater valve (255 to disable)", config->heaterValveOutput);
        logger.console("HEATPUMP=%d - Digital output to turn on heater pump (255 to disable)", config->heaterPumpOutput);
        logger.console("COOLPUMP=%d - Digital output to turn on cooling pump (255 to disable)", config->coolingPumpOutput);
        logger.console("COOLFAN=%d - Digital output to turn on cooling fan (255 to disable)", config->coolingFanOutput);
        logger.console("COOLON=%d - Controller temperature to turn cooling on (deg celsius)", config->coolingTempOn);
        logger.console("COOLOFF=%d - Controller temperature to turn cooling off (deg celsius)\n", config->coolingTempOff);

        logger.console("BRAKELT=%d - Digital output to use for brake light (255 to disable)", config->brakeLightOutput);
        logger.console("REVLT=%d - Digital output to use for reverse light (255 to disable)", config->reverseLightOutput);
        logger.console("PWRSTR=%d - Digital output to use to enable power steering (255 to disable)", config->powerSteeringOutput);
//        logger.console("TBD=%d - Digital output to use to xxxxxx (255 to disable)", config->unusedOutput);

        logger.console("WARNLT=%d - Digital output to use for reverse light (255 to disable)", config->warningOutput);
        logger.console("LIMITLT=%d - Digital output to use for limitation indicator (255 to disable)", config->powerLimitationOutput);
        logger.console("SOCHG=%d - Analog output to use to indicate state of charge (255 to disable)", config->stateOfChargeOutput);
        logger.console("STLGT=%d - Analog output to use to indicate system status (255 to disable)", config->statusLightOutput);
        logger.console("OUTPUT=<0-7> - toggles state of specified digital output");
    }
}

void SerialConsole::printMenuCharger()
{
    Charger *charger = deviceManager.getCharger();

    if (charger && charger->getConfiguration()) {
        ChargerConfiguration *config = (ChargerConfiguration *) charger->getConfiguration();
        logger.console("\nCHARGER CONTROLS\n");
        logger.console("CHCC=%d - Constant current (in 0.1A)", config->constantCurrent);
        logger.console("CHCV=%d - Constant voltage (in 0.1V)", config->constantVoltage);
        logger.console("CHTC=%d - Terminate current (in 0.1A)", config->terminateCurrent);
        logger.console("CHICMX=%d - Maximum Input current (in 0.1A)", config->maximumInputCurrent);
        logger.console("CHBVMN=%d - Minimum battery voltage (in 0.1V)", config->minimumBatteryVoltage);
        logger.console("CHBVMX=%d - Maximum battery voltage (in 0.1V)", config->maximumBatteryVoltage);
        logger.console("CHTPMN=%d - Minimum battery temperature for charging (in 0.1 deg C)", config->minimumTemperature);
        logger.console("CHTPMX=%d - Maximum battery temperature for charging (in 0.1 deg C)", config->maximumTemperature);
        logger.console("CHAHMX=%d - Maximum ampere hours (in 0.1Ah)", config->maximumAmpereHours);
        logger.console("CHCTMX=%d - Maximum charge time (in 1 min)", config->maximumChargeTime);
        logger.console("CHTDRC=%d - Derating of charge current (in 0.1A per deg C)", config->deratingRate);
        logger.console("CHTDRS=%d - Reference temperature for derating (in 0.1 deg C)", config->deratingReferenceTemperature);
        logger.console("CHTHYS=%d - Hysterese temperature where charging will be stopped (in 0.1 deg C)", config->hystereseStopTemperature);
        logger.console("CHTHYR=%d - Hysterese temperature where charging will resume (in 0.1 deg C)", config->hystereseResumeTemperature);
        logger.console("CHMT=%d - Input voltage measurement time (in ms)", config->measureTime);
        logger.console("CHMC=%d - Input voltage measurement current (in 0.1 A)", config->measureCurrent);
        logger.console("CHVD=%d - Input voltage drop factor (V / CHVD = allowed voltage drop)", config->voltageDrop);
    }
}

void SerialConsole::printMenuDcDcConverter()
{
    DcDcConverter *dcDcConverter = deviceManager.getDcDcConverter();

    if (dcDcConverter && dcDcConverter->getConfiguration()) {
        DcDcConverterConfiguration *config = (DcDcConverterConfiguration *) dcDcConverter->getConfiguration();
        logger.console("\nDCDC CONVERTER CONTROLS\n");
        logger.console("DCMODE=%d - operation mode (0 = buck/default, 1 = boost)", config->mode);
        logger.console("DCBULV=%d - Buck mode LV voltage (in 0.1V)", config->lowVoltageCommand);
        logger.console("DCBULVC=%d - Buck mode LV current limit (in 1A)", config->lvBuckModeCurrentLimit);
        logger.console("DCBUHVV=%d - Buck mode HV under voltage limit (in 1 V)", config->hvUndervoltageLimit);
        logger.console("DCBUHVC=%d - Buck mode HV current limit (in 0.1A)", config->hvBuckModeCurrentLimit);
        logger.console("DCBOHV=%d - Boost mode HV voltage (in 1V)", config->highVoltageCommand);
        logger.console("DCBOLVV=%d - Boost mode LV under voltage limit (in 0.1V)", config->lvUndervoltageLimit);
        logger.console("DCBOLVC=%d - Boost mode LV current limit (in 1A)", config->lvBoostModeCurrentLinit);
        logger.console("DCBOHVC=%d - Boost mode HV current limit (in 0.1A)", config->hvBoostModeCurrentLimit);
        if (dcDcConverter->getId() == BRUSA_BSC6) {
            BrusaBSC6Configuration *bscConfig = (BrusaBSC6Configuration *) config;
            logger.console("DCDBG=%d - Enable debug mode of BSC6 (0=off,1=on)", bscConfig->debugMode);
        }
    }
}

void SerialConsole::printMenuCanOBD2()
{
    CanOBD2 *canObd2 = (CanOBD2 *)deviceManager.getDeviceByID(CANOBD2);

    if (canObd2 && canObd2->isEnabled() && canObd2->getConfiguration()) {
        CanOBD2Configuration *config = (CanOBD2Configuration *) canObd2->getConfiguration();
        logger.console("\nCAN OBD2 CONTROLS\n");
        logger.console("ODBRES=%d - can bus number on which to respond to ODB2 PID requests (0=EV bus, 1=car bus, default=0)", config->canBusRespond);
        logger.console("ODBRESO=%d - offset to can ID 0x7e8 to respond to OBD2 PID requests (0-7, default=0)", config->canIdOffsetRespond);
        logger.console("OBDPOL=%d - can bus number on which we poll data from the car (0=EV, 1=car, default=1)", config->canBusPoll);
        logger.console("OBDPOLO=%d - offset to can ID 0x7e0 to request ODB2 data (0-7, 255=broadcast, default = 255)", config->canIdOffsetPoll);
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
            handleCmd();
        }
    }

    handlingEvent = false;
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
 comparison purposes.
 */
void SerialConsole::handleCmd()
{
    int i;
    int value;
    bool updateWifi = true;

    //logger.debug("Cmd size: %d", ptrBuffer);
    if (ptrBuffer < 6) {
        return;    //4 digit command, =, value is at least 6 characters
    }

    cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
    String command = String();
    i = 0;

    while (cmdBuffer[i] != '=' && i < ptrBuffer) {
        /*if (cmdBuffer[i] >= '0' && cmdBuffer[i] <= '9') {
         whichEntry = cmdBuffer[i++] - '0';
         }
         else */command.concat(String(cmdBuffer[i++]));
    }

    i++; //skip the =

    if (i >= ptrBuffer) {
        logger.console("Command needs a value..ie TORQ=3000\n");
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
            logger.warn("unknown command: %s", command.c_str());
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
        logger.console("Setting torque limit to %fNm", value / 10.0f);
        config->torqueMax = value;
    } else if (command == String("RPMS")) {
        value = constrain(value, 0, 1000000);
        logger.console("Setting speed limit to %drpm", value);
        config->speedMax = value;
    } else if (command == String("MOMODE")) {
        value = constrain(value, 0, 1);
        logger.console("Setting power mode to %s", (value == 0 ? "torque" : "speed (be careful !!)"));
        config->powerMode = (PowerMode)value;
    } else if (command == String("CRLVL")) {
        value = constrain(value, 0, 100);
        logger.console("Setting creep level to %d%%", value);
        config->creepLevel = value;
    } else if (command == String("CRSPD")) {
        value = constrain(value, 0, config->speedMax);
        logger.console("Setting creep speed to %drpm", value);
        config->creepSpeed = value;
    } else if (command == String("REVLIM")) {
        value = constrain(value, 0, 1000);
        logger.console("Setting reverse limit to %f%%", value / 10.0f);
        config->reversePercent = value;
    } else if (command == String("NOMV")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting fully charged voltage to %fV", value / 10.0f);
        config->nominalVolt = value;
    } else if (command == String("MOINVD")) {
        value = constrain(value, 0, 1);
        logger.console("Setting motor direction to %s", (value ? "inverted" : "normal"));
        config->invertDirection = value;
    } else if (command == String("MOSLEWM")) {
        logger.console("Setting slew rate motoring to %f percent/sec", value / 10.0f);
        config->slewRateMotor = value;
    } else if (command == String("MOSLEWR")) {
        logger.console("Setting slew regen rate to %f percent/sec", value / 10.0f);
        config->slewRateRegen = value;
    } else if (command == String("MOMWMX")) {
        logger.console("Setting maximal mechanical power of motor to %fkW", value / 10.0f);
        config->maxMechanicalPowerMotor = value;
    } else if (command == String("MORWMX")) {
        logger.console("Setting maximal mechanical power of regen to %fkW", value / 10.0f);
        config->maxMechanicalPowerRegen = value;
    } else if (command == String("MOBRHD")) {
        logger.console("Setting maximal brake hold level to %d%%", value);
        config->brakeHold = value;
    } else if (command == String("MOBRHQ")) {
        value = constrain(value, 1, 255);
        logger.console("Setting brake hold force coefficient to %d", value);
        config->brakeHoldForceCoefficient = value;
    } else if (command == String("MOMVMN") && (motorController->getId() == BRUSA_DMC5)) {
        logger.console("Setting minimum DC voltage limit for motoring to %fV", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcVoltLimitMotor = value;
    } else if (command == String("MOMCMX") && (motorController->getId() == BRUSA_DMC5)) {
        logger.console("Setting current limit for motoring to %fA", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcCurrentLimitMotor = value;
    } else if (command == String("MORVMX") && (motorController->getId() == BRUSA_DMC5)) {
        logger.console("Setting maximum DC voltage limit for regen to %fV", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcVoltLimitRegen = value;
    } else if (command == String("MORCMX") && (motorController->getId() == BRUSA_DMC5)) {
        logger.console("Setting current limit for regen to %fA", value / 10.0f);
        ((BrusaDMC5Configuration *) config)->dcCurrentLimitRegen = value;
    } else if (command == String("MOOSC") && (motorController->getId() == BRUSA_DMC5)) {
        value = constrain(value, 0, 1);
        logger.console("Setting oscillation limiter to %s", (value == 0 ? "disabled" : "enabled"));
        ((BrusaDMC5Configuration *) config)->enableOscillationLimiter = value;
    } else if (command == String("CRUISEP")) {
        logger.console("Setting cruise control Kp value to %f", value / 1000.0f);
        config->cruiseKp = value / 1000.0f;
    } else if (command == String("CRUISEI")) {
        logger.console("Setting cruise control Ki value to %f", value / 1000.0f);
        config->cruiseKi = value / 1000.0f;
    } else if (command == String("CRUISED")) {
        logger.console("Setting cruise control Kd value to %f", value / 1000.0f);
        config->cruiseKd = value / 1000.0f;
    } else if (command == String("CRUISEL")) {
        value = constrain(value, 1, 9000);
        logger.console("Setting delta in rpm/kph to actual speed to %d", value);
        config->cruiseLongPressDelta = value;
    } else if (command == String("CRUISES")) {
        value = constrain(value, 1, 9000);
        logger.console("Setting delta in rpm/kph to target speed to %d", value);
        config->cruiseStepDelta = value;
    } else if (command == String("CRUISER")) {
        value = constrain(value, 0, 1);
        logger.console("Setting cruise control method to '%s'", (value == 1 ? "rpm" : "vehicle speed"));
        config->cruiseUseRpm = value;
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
        logger.console("Setting # of throttle pots to %d", value);
        ((PotThrottleConfiguration *) config)->numberPotMeters = value;
    } else if (command == String("TTYPE") && (throttle->getId() == POTACCELPEDAL)) {
        value = constrain(value, 1, 2);
        logger.console("Setting throttle subtype to %s", (value == 2 ? "inverse" : "std linear"));
        ((PotThrottleConfiguration *) config)->throttleSubType = value;
    } else if (command == String("T1ADC") && (throttle->getId() == POTACCELPEDAL)) {
        logger.console("Setting Throttle1 ADC pin to %d", value);
        ((PotThrottleConfiguration *) config)->AdcPin1 = value;
    } else if (command == String("T2ADC") && (throttle->getId() == POTACCELPEDAL)) {
        logger.console("Setting Throttle2 ADC pin to %d", value);
        ((PotThrottleConfiguration *) config)->AdcPin2 = value;
    } else if (command == String("T2MN") && (throttle->getId() == POTACCELPEDAL)) {
        logger.console("Setting throttle 2 min to %d", value);
        ((PotThrottleConfiguration *) config)->minimumLevel2 = value;
    } else if (command == String("T2MX") && (throttle->getId() == POTACCELPEDAL)) {
        logger.console("Setting throttle 2 max to %d", value);
        ((PotThrottleConfiguration *) config)->maximumLevel2 = value;
    } else if (command == String("T1MN")) {
        logger.console("Setting throttle 1 min to %d", value);
        config->minimumLevel = value;
    } else if (command == String("T1MX")) {
        logger.console("Setting throttle 1 max to %d", value);
        config->maximumLevel = value;
    } else if (command == String("TRGNMAX")) {
        value = constrain(value, 0, config->positionRegenMinimum);
        logger.console("Setting throttle regen maximum to %f%%", value / 10.0f);
        config->positionRegenMaximum = value;
    } else if (command == String("TRGNMIN")) {
        value = constrain(value, config->positionRegenMaximum, config->positionForwardMotionStart);
        logger.console("Setting throttle regen minimum to %f%%", value / 10.0f);
        config->positionRegenMinimum = value;
    } else if (command == String("TFWD")) {
        value = constrain(value, config->positionRegenMinimum, config->positionHalfPower);
        logger.console("Setting throttle forward start to %f%%", value / 10.0f);
        config->positionForwardMotionStart = value;
    } else if (command == String("TMAP")) {
        value = constrain(value, config->positionForwardMotionStart, 1000);
        logger.console("Setting throttle 50%% torque to %f%%", value / 10.0f);
        config->positionHalfPower = value;
    } else if (command == String("TMINRN")) {
        value = constrain(value, 0, config->maximumRegen);
        logger.console("Setting throttle regen minimum strength to %d%%", value);
        config->minimumRegen = value;
    } else if (command == String("TMAXRN")) {
        value = constrain(value, config->minimumRegen, 100);
        logger.console("Setting throttle Regen maximum strength to %d%%", value);
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

    if (command == String("B1MN")) {
        logger.console("Setting brake min to %d", value);
        config->minimumLevel = value;
    } else if (command == String("B1MX")) {
        logger.console("Setting brake max to %d", value);
        config->maximumLevel = value;
    } else if (command == String("BMINR")) {
        value = constrain(value, 0, config->maximumRegen);
        logger.console("Setting min brake regen to %d%%", value);
        config->minimumRegen = value;
    } else if (command == String("BMAXR")) {
        value = constrain(value, config->minimumRegen, 100);
        logger.console("Setting max brake regen to %d%%", value);
        config->maximumRegen = value;
    } else if (command == String("B1ADC") && (brake->getId() == POTBRAKEPEDAL)) {
        logger.console("Setting Brake ADC pin to %d", value);
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

    if (command == String("CTP")) {
        logger.console("Setting car type to %d", value);
        config->carType = (SystemIOConfiguration::CarType) value;
    } else if (command == String("ENABLEI")) {
        if (value <= CFG_NUMBER_DIGITAL_INPUTS && value >= 0) {
            logger.console("Setting enable signal to input %d.", value);
            config->enableInput = value;
        } else {
            logger.console("Invalid enable signal input number. Please enter a value 0 - %d", CFG_NUMBER_DIGITAL_INPUTS - 1);
        }
    } else if (command == String("CHARGEI")) {
        value = constrain(value, 0, 255);
        logger.console("Setting charge signal to input %d", value);
        config->chargePowerAvailableInput = value;
    } else if (command == String("INTERLI")) {
        value = constrain(value, 0, 255);
        logger.console("Setting interlock signal to input %d", value);
        config->interlockInput = value;
    } else if (command == String("REVIN")) {
        config->reverseInput = value;
        logger.console("Motor reverse signal set to input %d.", value);
    } else if (command == String("ABSIN")) {
        config->absInput = value;
        logger.console("ABS signal set to input %d.", value);
    } else if (command == String("GEARIN")) {
        config->gearChangeInput = value;
        logger.console("Gear change signal set to input %d.", value);
    } else if (command == String("PREDELAY")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting precharge time to %dms", value);
        config->prechargeMillis = value;
    } else if (command == String("PRELAY")) {
        value = constrain(value, 0, 255);
        logger.console("Setting precharge relay to output %d", value);
        config->prechargeRelayOutput = value;
    } else if (command == String("MRELAY")) {
        value = constrain(value, 0, 255);
        logger.console("Setting main contactor to output %d", value);
        config->mainContactorOutput = value;
    } else if (command == String("NRELAY")) {
        value = constrain(value, 0, 255);
        logger.console("Setting secondary contactor to output %d", value);
        config->secondaryContactorOutput = value;
    } else if (command == String("FRELAY")) {
        value = constrain(value, 0, 255);
        logger.console("Setting fast charge contactor to output %d", value);
        config->fastChargeContactorOutput = value;
    } else if (command == String("ENABLEM")) {
        value = constrain(value, 0, 255);
        logger.console("Setting enable motor signal to output %d", value);
        config->enableMotorOutput = value;
    } else if (command == String("ENABLEC")) {
        value = constrain(value, 0, 255);
        logger.console("Setting enable charger signal to output %d", value);
        config->enableChargerOutput = value;
    } else if (command == String("ENABLED")) {
        value = constrain(value, 0, 255);
        logger.console("Setting enable DC-DC converter signal to output %d", value);
        config->enableDcDcOutput = value;
    } else if (command == String("ENABLEH")) {
        value = constrain(value, 0, 255);
        logger.console("Setting enable heater signal to output %d", value);
        config->enableHeaterOutput = value;
    } else if (command == String("HEATVALV")) {
        value = constrain(value, 0, 255);
        logger.console("Setting heater valve signal output %d", value);
        config->heaterValveOutput = value;
    } else if (command == String("HEATERON")) {
        value = constrain(value, 0, 40);
        logger.console("Setting enable heater temp on to %d deg C", value);
        config->heaterTemperatureOn = value;
    } else if (command == String("HEATPUMP")) {
        value = constrain(value, 0, 255);
        logger.console("Setting heater pump signal to output %d", value);
        config->heaterPumpOutput = value;
    } else if (command == String("COOLPUMP")) {
        value = constrain(value, 0, 255);
        logger.console("Setting cooling pump signal to output %d", value);
        config->coolingPumpOutput = value;
    } else if (command == String("COOLFAN")) {
        value = constrain(value, 0, 255);
        logger.console("Setting cooling fan signal to output %d", value);
        config->coolingFanOutput = value;
    } else if (command == String("COOLON")) {
        value = constrain(value, 0, 200);
        logger.console("Setting cooling ON temperature to: %d deg C", value);
        config->coolingTempOn = value;
    } else if (command == String("COOLOFF")) {
        value = constrain(value, 0, config->coolingTempOn);
        logger.console("Setting cooling OFF temperature to: %d deg C", value);
        config->coolingTempOff = value;
    } else if (command == String("BRAKELT")) {
        value = constrain(value, 0, 255);
        logger.console("Brake light signal set to output %d.", value);
        config->brakeLightOutput = value;
    } else if (command == String("REVLT")) {
        value = constrain(value, 0, 255);
        logger.console("Reverse light signal set to output %d.", value);
        config->reverseLightOutput = value;
    } else if (command == String("PWRSTR")) {
        value = constrain(value, 0, 255);
        logger.console("Power steering signal set to output %d.", value);
        config->powerSteeringOutput = value;
    } else if (command == String("WARNLT")) {
        value = constrain(value, 0, 255);
        logger.console("Warning signal set to output %d.", value);
        config->warningOutput = value;
    } else if (command == String("LIMITLT")) {
        value = constrain(value, 0, 255);
        logger.console("Limit signal set to output %d.", value);
        config->powerLimitationOutput = value;
    } else if (command == String("SOCHG")) {
        value = constrain(value, 0, 255);
        logger.console("State of charge set to output %d.", value);
        config->stateOfChargeOutput = value;
    } else if (command == String("STLGT")) {
        value = constrain(value, 0, 255);
        logger.console("Status light set to output %d.", value);
        config->statusLightOutput = value;
    } else if (command == String("STLGTMD")) {
        logger.console("Status light set to mode %s.", cmdBuffer + command.length() + 1);
        deviceManager.sendMessage(DEVICE_DISPLAY, STATUSINDICATOR, MSG_UPDATE, (void *)(cmdBuffer + command.length() + 1));
    } else if (command == String("OUTPUT") && value < 8) {
        logger.console("DOUT%d,  STATE: %d", value, systemIO.getDigitalOut(value));
        systemIO.setDigitalOut(value, !systemIO.getDigitalOut(value));
        logger.console("DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d, DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", systemIO.getDigitalOut(0),
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
        logger.console("Setting constant current to %fA", value / 10.0f);
        config->constantCurrent = value;
    } else if (command == String("CHCV")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting constant voltage to %fV", value / 10.0f);
        config->constantVoltage = value;
    } else if (command == String("CHTC")) {
        value = constrain(value, 0, config->constantCurrent);
        logger.console("Setting terminate current to %fA", value / 10.0f);
        config->terminateCurrent = value;
    } else if (command == String("CHICMX")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting max input current to %fV", value / 10.0f);
        config->maximumInputCurrent = value;
    } else if (command == String("CHBVMN")) {
        value = constrain(value, 0, config->maximumBatteryVoltage);
        logger.console("Setting min battery voltage to %fV", value / 10.0f);
        config->minimumBatteryVoltage = value;
    } else if (command == String("CHBVMX")) {
        value = constrain(value, config->minimumBatteryVoltage, 100000);
        logger.console("Setting max battery voltage to %fV", value / 10.0f);
        config->maximumBatteryVoltage = value;
    } else if (command == String("CHTPMN")) {
        value = constrain(value, -1000, config->maximumTemperature);
        logger.console("Setting min battery temp to %f deg C", value / 10.0f);
        config->minimumTemperature = value;
    } else if (command == String("CHTPMX")) {
        value = constrain(value, config->minimumTemperature, 10000);
        logger.console("Setting max battery temp to %f deg C", value / 10.0f);
        config->maximumTemperature = value;
    } else if (command == String("CHAHMX")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting max Ampere hours to %fAh", value / 10.0f);
        config->maximumAmpereHours = value;
    } else if (command == String("CHCTMX")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting max charge time to %d min", value);
        config->maximumChargeTime = value;
    } else if (command == String("CHTDRC")) {
        value = constrain(value, 0, 100000);
        logger.console("Setting derating of charge current to %fA per deg C", value / 10.0f);
        config->deratingRate = value;
    } else if (command == String("CHTDRS")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting derating reference temp to %f deg C", value / 10.0f);
        config->deratingReferenceTemperature = value;
    } else if (command == String("CHTHYS")) {
        value = constrain(value, config->hystereseResumeTemperature, 10000);
        logger.console("Setting hysterese temp to stop charging to %f deg C", value / 10.0f);
        config->hystereseStopTemperature = value;
    } else if (command == String("CHTHYR")) {
        value = constrain(value, 0, config->hystereseStopTemperature);
        logger.console("Setting hysterese temp to resume charging to %f deg C", value / 10.0f);
        config->hystereseResumeTemperature = value;
    } else if (command == String("CHMT")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting measure time to %d ms", value);
        config->measureTime = value;
    } else if (command == String("CHMC")) {
        value = constrain(value, 0, 2000);
        logger.console("Setting measure current to %f A", value / 10.0f);
        config->measureCurrent = value;
    } else if (command == String("CHVD")) {
        value = constrain(value, 1, 100);
        logger.console("Setting voltage drop divisor to %d", value);
        config->voltageDrop = value;
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
        logger.console("Setting mode to %s", (value == 0 ? "buck" : "boost"));
        config->mode = value;
    } else if (command == String("DCBULV")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting buck LV voltage to %fV", value / 10.0f);
        config->lowVoltageCommand = value;
    } else if (command == String("DCBULVC")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting buck LV current limit to %dA", value);
        config->lvBuckModeCurrentLimit = value;
    } else if (command == String("DCBUHVV")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting buck HV under voltage limit to %dV", value);
        config->hvUndervoltageLimit = value;
    } else if (command == String("DCBUHVC")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting buck HV current limit to %fA", value / 10.0f);
        config->hvBuckModeCurrentLimit = value;
    } else if (command == String("DCBOHV")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting boost HV voltage to %dV", value);
        config->highVoltageCommand = value;
    } else if (command == String("DCBOLVV")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting boost LV undervoltage limit to %fV", value / 10.0f);
        config->lvUndervoltageLimit = value;
    } else if (command == String("DCBOLVC")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting boost LV current limit to %dA", value);
        config->lvBoostModeCurrentLinit = value;
    } else if (command == String("DCBOHVC")) {
        value = constrain(value, 0, 10000);
        logger.console("Setting boost HV current limit to %fA", value / 10.0f);
        config->hvBoostModeCurrentLimit = value;
    } else if (command == String("DCDBG") && dcdcConverter->getId() == BRUSA_BSC6) {
        BrusaBSC6Configuration *bscConfig = (BrusaBSC6Configuration *) config;
        value = constrain(value, 0, 1);
        logger.console("Setting BSC6 debug mode %d", value);
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
            logger.console("Invalid device ID (%#x, %d)", value, value);
        }
    } else if (command == String("DISABLE")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_DISABLE, NULL)) {
            logger.console("Invalid device ID (%#x, %d)", value, value);
        }
    } else if (command == String("KILL")) {
        if (!deviceManager.sendMessage(DEVICE_ANY, (DeviceId) value, MSG_KILL, NULL)) {
            logger.console("Invalid device ID (%#x, %d)", value, value);
        }
    } else if (command == String("SYSTYPE")) {
        systemIO.setSystemType((SystemIOConfiguration::SystemType) constrain(value, SystemIOConfiguration::GEVCU1, SystemIOConfiguration::GEVCU4));
        logger.console("System type updated. Power cycle to apply.");
    } else if (command == String("LOGLEVEL")) {
        if (strchr(parameter, ',') == NULL) {
            logger.setLoglevel((Logger::LogLevel) value);
            logger.console("setting loglevel to %d", value);
            systemIO.setLogLevel(logger.getLogLevel());
        } else {
            DeviceId deviceId = (DeviceId) strtol(strtok(parameter, ","), NULL, 0);
            Device *device = deviceManager.getDeviceByID(deviceId);
            if (device != NULL) {
                value = atol(strtok(NULL, ","));
                logger.setLoglevel(device, (Logger::LogLevel) value);
            }
        }
    } else if (command == String("NUKE") && value == 1) {
        // write zero to the checksum location of every device in the table.
        uint8_t zeroVal = 0;
        for (int j = 0; j < 64; j++) {
            memCache.Write(EE_DEVICES_BASE + (EE_DEVICE_SIZE * j), zeroVal);
            memCache.FlushAllPages();
        }
        logger.console("Device settings have been nuked. Reboot to reload default settings");
    } else {
        return false;
    }
    return true;
}

bool SerialConsole::handleConfigCmdWifi(String command, String parameter)
{
    if (command == String("WLAN")) {
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) parameter.c_str());
        logger.info("sent \"AT+i%s\" to wifi device", parameter.c_str());
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
    return true;
}

void SerialConsole::sendWifiCommand(String command, String parameter)
{
    command.concat("=");
    command.concat(parameter);
    logger.info("sent \"AT+i%s\" to wifi device", command.c_str());
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
        logger.console("Setting listener can bus to %d", value);
        config->canBusRespond = value;
    } else if (command == String("ODBRESO")) {
        value = constrain(value, 0, 7);
        logger.console("Setting respond can ID offset to %d", value);
        config->canIdOffsetRespond = value;
    } else if (command == String("OBDPOL")) {
        value = constrain(value, 0, 1);
        logger.console("Setting query can bus to %d", value);
        config->canBusPoll = value;
    } else if (command == String("OBDPOLO")) {
        if (value != 255) {
            value = constrain(value, 0, 7);
        }
        logger.console("Setting request can ID to %d", value);
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
                logger.console("Output raw throttle");
            } else {
                logger.console("Cease raw throttle output");
            }
        }
        break;

    case 'U':
        logger.console("Adding a sequence of values from 0 to 255 into eeprom");

        for (int i = 0; i < 256; i++) {
            memCache.Write(1000 + i, (uint8_t) i);
        }

        logger.info("Flushing cache");
        memCache.FlushAllPages(); //write everything to eeprom
        memCache.InvalidateAll(); //remove all data from cache
        logger.console("Operation complete.");
        break;

    case 'I':
        logger.console("Retrieving data previously saved");

        for (int i = 0; i < 256; i++) {
            memCache.Read(1000 + i, &val);
            logger.console("%d: %d", i, val);
        }
        break;

    case 'K': //set all outputs high
        for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
            systemIO.setDigitalOut(tout, true);
        }

        logger.console("all outputs: ON");
        break;

    case 'J': //set the four outputs low
        for (int tout = 0; tout < CFG_NUMBER_DIGITAL_OUTPUTS; tout++) {
            systemIO.setDigitalOut(tout, false);
        }

        logger.console("all outputs: OFF");
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
        logger.console("PASSTHROUGH MODE - All traffic Serial3 <-> SerialUSB");
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
        logger.console("Finding and listing all nearby WiFi access points");
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "RP20");
        break;

    case 'W':
        logger.console("Setting Wifi Adapter to WPS mode (make sure you press the WPS button on your router)");
        // restore factory defaults and give it some time
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *) "AWPS");
        break;

    case 'w':
        logger.console("Resetting wifi to factory defaults and setting up AP, this takes about 50sec, please stand-by");
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_RESET, NULL);
        deviceManager.sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL); // reload configuration params as they were lost
        logger.console("Wifi initialized");
        break;

    case 'X':
        setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
        break;

    case 'P':
        logger.console("measuring pre-charge cycle");
        systemIO.measurePreCharge();
        break;
    }
}
