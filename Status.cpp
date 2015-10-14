/*
 * Status.cpp
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

#include "Status.h"
#include "DeviceManager.h"

Status status;

/*
 * Constructor
 */
Status::Status() {
    systemState = startup;

    limitationTorque                = false;
    limitationMaxTorque             = false;
    limitationSpeed                 = false;
    limitationControllerTemperature = false;
    limitationMotorTemperature      = false;
    limitationSlewRate              = false;
    limitationMotorModel            = false;
    limitationMechanicalPower       = false;
    limitationAcVoltage             = false;
    limitationAcCurrent             = false;
    limitationDcVoltage             = false;
    limitationDcCurrent             = false;

    warning                         = false;
    driverShutdownPathActive        = false;
    externalShutdownPath1Off        = false;
    externalShutdownPath2Off        = false;
    oscillationLimitControllerActive= false;
    speedSensorSignal               = false;
    maximumModulationLimiter        = false;
    temperatureSensor               = false;
    systemCheckActive               = false;

    speedSensor                     = false;
    speedSensorSupply               = false;
    canLimitMessageInvalid          = false;
    canControlMessageInvalid        = false;
    canLimitMessageLost             = false;
    overvoltageInternalSupply       = false;
    voltageMeasurement              = false;
    shortCircuit                    = false;
    canControlMessageLost           = false;
    canControl2MessageLost          = false;
    overtempController              = false;
    overtempMotor                   = false;
    overspeed                       = false;
    hvUndervoltage                  = false;
    hvOvervoltage                   = false;
    hvOvercurrent                   = false;
    acOvercurrent                   = false;
    initalisation                   = false;
    analogInput                     = false;
    unexpectedShutdown              = false;
    powerMismatch                   = false;
    motorEeprom                     = false;
    storage                         = false;
    enableSignalLost                = false;
    canCommunicationStartup         = false;
    internalSupply                  = false;
    osTrap                          = false;

    preChargeRelay = false;
    mainContactor = false;
    secondaryContactor = false;
    fastChargeContactor = false;

    enableMotor = false;
    enableCharger = false;
    enableDcDc = false;
    enableHeater = false;

    heaterValve = false;
    heaterPump = false;
    coolingPump = false;
    coolingFan = false;

    brakeLight = false;
    reverseLight = false;

    enableIn            = false;
    chargePowerAvailable= false;
    interlockPresent    = false;
    reverseInput        = false;

    for (int i = 0; i < CFG_NUMBER_DIGITAL_OUTPUTS; i++) {
        digitalOutput[i] = false;
    }
    for (int i = 0; i < CFG_NUMBER_DIGITAL_INPUTS; i++) {
        digitalInput[i] = false;
    }
    for (int i = 0; i < CFG_NUMBER_TEMPERATURE_SENSORS; i++) {
        externalTemperature[i] = CFG_NO_TEMPERATURE_DATA;
    }
}

/*
 * Retrieve the current system state.
 */
Status::SystemState Status::getSystemState() {
    return systemState;
}

/*
 * Set a new system state. The new system state is validated if the
 * transition is allowed from the old state. If an invalid transition is
 * attempted, the new state will be 'error'.
 */
Status::SystemState Status::setSystemState(SystemState newSystemState) {

    if (systemState == newSystemState) {
        return systemState;
    }

    SystemState oldSystemState = systemState;

    if (newSystemState == error) {
        systemState = error;
    } else {
        switch (systemState) {
        case startup:
            if (newSystemState == init) {
                systemState = newSystemState;
            }
            break;
        case init:
            if (newSystemState == preCharge) {
                systemState = newSystemState;
            }
            break;
        case preCharge:
            if (newSystemState == preCharged) {
                systemState = newSystemState;
            }
            break;
        case preCharged:
            if (newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case batteryHeating:
            if (newSystemState == charging || newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case charging:
            if (newSystemState == charged || newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case charged:
            if (newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case ready:
            if (newSystemState == running || newSystemState == charging || newSystemState == batteryHeating) {
                systemState = newSystemState;
            }
            break;
        case running:
            if (newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case error:
            if (newSystemState == init) {
                systemState = newSystemState;
            }
            break;
        }
    }
    if (systemState == newSystemState) {
        Logger::info("switching to system state '%s'", systemStateToStr(systemState));
    } else {
        Logger::error("switching from system state '%s' to '%s' is not allowed", systemStateToStr(systemState), systemStateToStr(newSystemState));
        systemState = error;
    }

    SystemState params[] = { oldSystemState, systemState };
    deviceManager.sendMessage(DEVICE_ANY, INVALID, MSG_STATE_CHANGE, params);

    return systemState;
}

int16_t Status::getLowestExternalTemperature() {
    int16_t temp = CFG_NO_TEMPERATURE_DATA;

    for (int i = 0; i < CFG_NUMBER_TEMPERATURE_SENSORS; i++) {
        if (externalTemperature[i] != CFG_NO_TEMPERATURE_DATA) {
            temp = min(temp, externalTemperature[i]);
        }
    }
    return temp;
}

int16_t Status::getHighestExternalTemperature() {
    int16_t temp = -9999;

    for (int i = 0; i < CFG_NUMBER_TEMPERATURE_SENSORS; i++) {
        if (externalTemperature[i] != CFG_NO_TEMPERATURE_DATA) {
            temp = max(temp, externalTemperature[i]);
        }
    }
    return (temp == -9999 ? CFG_NO_TEMPERATURE_DATA : temp);
}

/*
 * Convert the current state into a string.
 */
char *Status::systemStateToStr(SystemState state) {
    switch (state) {
    case startup:
        return "unknown";
    case init:
        return "init";
    case preCharge:
        return "pre-charge";
    case preCharged:
        return "pre-charged";
    case ready:
        return "ready";
    case running:
        return "running";
    case error:
        return "error";
    case batteryHeating:
        return "battery heating";
    case charging:
        return "charging";
    case charged:
        return "charged";
    }
    Logger::error("the system state is invalid, contact your support!");
    return "invalid";
}

/*
 * Calculate the bit field 1 from the respective flags for faster transmission
 * to the web site.
 *
 * Bitfield 1 contains warnings and torque limitation information
 */
uint32_t Status::getBitField1() {
    uint32_t bitfield = 0;

    bitfield |= (warning                            ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (driverShutdownPathActive           ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (externalShutdownPath1Off           ? 1 << 2 : 0);  // 0x00000004
    bitfield |= (externalShutdownPath2Off           ? 1 << 3 : 0);  // 0x00000008
    bitfield |= (oscillationLimitControllerActive   ? 1 << 4 : 0);  // 0x00000010
    bitfield |= (speedSensorSignal                  ? 1 << 5 : 0);  // 0x00000020
    bitfield |= (maximumModulationLimiter           ? 1 << 6 : 0);  // 0x00000040
    bitfield |= (temperatureSensor                  ? 1 << 7 : 0);  // 0x00000080

//    bitfield |= (reserve                          ? 1 << 8 : 0);  // 0x00000100
//    bitfield |= (reserve                          ? 1 << 9 : 0);  // 0x00000200
//    bitfield |= (reserve                          ? 1 << 10 : 0); // 0x00000400
//    bitfield |= (reserve                          ? 1 << 11 : 0); // 0x00000800

    bitfield |= (limitationTorque                   ? 1 << 12 : 0); // 0x00001000
    bitfield |= (limitationMaxTorque                ? 1 << 13 : 0); // 0x00002000
    bitfield |= (limitationSpeed                    ? 1 << 14 : 0); // 0x00004000
    bitfield |= (limitationControllerTemperature    ? 1 << 15 : 0); // 0x00008000
    bitfield |= (limitationMotorTemperature         ? 1 << 16 : 0); // 0x00010000
    bitfield |= (limitationSlewRate                 ? 1 << 17 : 0); // 0x00020000
    bitfield |= (limitationMotorModel               ? 1 << 18 : 0); // 0x00040000
    bitfield |= (limitationMechanicalPower          ? 1 << 19 : 0); // 0x00080000
    bitfield |= (limitationAcVoltage                ? 1 << 20 : 0); // 0x00100000
    bitfield |= (limitationAcCurrent                ? 1 << 21 : 0); // 0x00200000
    bitfield |= (limitationDcVoltage                ? 1 << 22 : 0); // 0x00400000
    bitfield |= (limitationDcCurrent                ? 1 << 23 : 0); // 0x00800000

//    bitfield |= (reserve                          ? 1 << 24 : 0); // 0x01000000
//    bitfield |= (reserve                          ? 1 << 25 : 0); // 0x02000000
//    bitfield |= (reserve                          ? 1 << 26 : 0); // 0x04000000
//    bitfield |= (reserve                          ? 1 << 27 : 0); // 0x08000000
//    bitfield |= (reserve                          ? 1 << 28 : 0); // 0x10000000
//    bitfield |= (reserve                          ? 1 << 29 : 0); // 0x20000000
//    bitfield |= (reserve                          ? 1 << 30 : 0); // 0x40000000
//    bitfield |= (reserve                          ? 1 << 31 : 0); // 0x80000000

    return bitfield;
}

/*
 * Calculate the bit field 2 from the respective flags for faster transmission
 * to the web site.
 *
 * Bitfield 2 containts error information
 */
uint32_t Status::getBitField2() {
    uint32_t bitfield = 0;

    bitfield |= (systemState == error               ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (speedSensor                        ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (speedSensorSupply                  ? 1 << 2 : 0);  // 0x00000004
    bitfield |= (canLimitMessageInvalid             ? 1 << 3 : 0);  // 0x00000008
    bitfield |= (canControlMessageInvalid           ? 1 << 4 : 0);  // 0x00000010
    bitfield |= (canLimitMessageLost                ? 1 << 5 : 0);  // 0x00000020
    bitfield |= (overvoltageInternalSupply          ? 1 << 6 : 0);  // 0x00000040
    bitfield |= (voltageMeasurement                 ? 1 << 7 : 0);  // 0x00000080
    bitfield |= (shortCircuit                       ? 1 << 8 : 0);  // 0x00000100
    bitfield |= (canControlMessageLost              ? 1 << 9 : 0);  // 0x00000200
    bitfield |= (canControl2MessageLost             ? 1 << 10 : 0); // 0x00000400
    bitfield |= (overtempController                 ? 1 << 11 : 0); // 0x00000800
    bitfield |= (overtempMotor                      ? 1 << 12 : 0); // 0x00001000
    bitfield |= (overspeed                          ? 1 << 13 : 0); // 0x00002000
    bitfield |= (hvUndervoltage                     ? 1 << 14 : 0); // 0x00004000
    bitfield |= (hvOvervoltage                      ? 1 << 15 : 0); // 0x00008000
    bitfield |= (hvOvercurrent                      ? 1 << 16 : 0); // 0x00010000
    bitfield |= (acOvercurrent                      ? 1 << 17 : 0); // 0x00020000
    bitfield |= (initalisation                      ? 1 << 18 : 0); // 0x00040000
    bitfield |= (analogInput                        ? 1 << 19 : 0); // 0x00080000
    bitfield |= (unexpectedShutdown                 ? 1 << 20 : 0); // 0x00100000
    bitfield |= (powerMismatch                      ? 1 << 21 : 0); // 0x00200000
    bitfield |= (motorEeprom                        ? 1 << 22 : 0); // 0x00400000
    bitfield |= (storage                            ? 1 << 23 : 0); // 0x00800000
    bitfield |= (enableSignalLost                   ? 1 << 24 : 0); // 0x01000000
    bitfield |= (canCommunicationStartup            ? 1 << 25 : 0); // 0x02000000
    bitfield |= (internalSupply                     ? 1 << 26 : 0); // 0x04000000
//    bitfield |= (reserve                          ? 1 << 27 : 0); // 0x08000000
//    bitfield |= (reserve                          ? 1 << 28 : 0); // 0x10000000
//    bitfield |= (reserve                          ? 1 << 29 : 0); // 0x20000000
//    bitfield |= (reserve                          ? 1 << 30 : 0); // 0x40000000
    bitfield |= (osTrap                             ? 1 << 31 : 0); // 0x80000000

    return bitfield;
}

/*
 * Calculate the bit field 3 from the respective flags for faster transmission
 * to the web site.
 *
 * Bitfield 3 containts information about the digital outputs
 */
uint32_t Status::getBitField3() {
    uint32_t bitfield = 0;

    bitfield |= (systemState == ready               ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (systemState == running             ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (preChargeRelay                     ? 1 << 2 : 0);  // 0x00000004
    bitfield |= (secondaryContactor                 ? 1 << 3 : 0);  // 0x00000008
    bitfield |= (mainContactor                      ? 1 << 4 : 0);  // 0x00000010
    bitfield |= (enableMotor                        ? 1 << 5 : 0);  // 0x00000020
    bitfield |= (coolingFan                         ? 1 << 6 : 0);  // 0x00000040
    bitfield |= (brakeLight                         ? 1 << 7 : 0);  // 0x00000080
    bitfield |= (reverseLight                       ? 1 << 8 : 0);  // 0x00000100
    bitfield |= (enableIn                           ? 1 << 9 : 0);  // 0x00000200
//    bitfield |= (reserve                          ? 1 << 1 : 0);  // 0x00000400
//    bitfield |= (reserve                          ? 1 << 11 : 0); // 0x00000800
//    bitfield |= (reserve                          ? 1 << 12 : 0); // 0x00001000
//    bitfield |= (reserve                          ? 1 << 13 : 0); // 0x00002000
//    bitfield |= (reserve                          ? 1 << 14 : 0); // 0x00004000
//    bitfield |= (reserve                          ? 1 << 15 : 0); // 0x00008000
//    bitfield |= (reserve                          ? 1 << 16 : 0); // 0x00010000
//    bitfield |= (reserve                          ? 1 << 17 : 0); // 0x00020000
//    bitfield |= (reserve                          ? 1 << 18 : 0); // 0x00040000
//    bitfield |= (reserve                          ? 1 << 19 : 0); // 0x00080000
    bitfield |= (digitalOutput[0]                   ? 1 << 20 : 0); // 0x00100000
    bitfield |= (digitalOutput[1]                   ? 1 << 21 : 0); // 0x00200000
    bitfield |= (digitalOutput[2]                   ? 1 << 22 : 0); // 0x00400000
    bitfield |= (digitalOutput[3]                   ? 1 << 23 : 0); // 0x00800000
    bitfield |= (digitalOutput[4]                   ? 1 << 24 : 0); // 0x01000000
    bitfield |= (digitalOutput[5]                   ? 1 << 25 : 0); // 0x02000000
    bitfield |= (digitalOutput[6]                   ? 1 << 26 : 0); // 0x04000000
    bitfield |= (digitalOutput[7]                   ? 1 << 27 : 0); // 0x08000000
    bitfield |= (digitalInput[0]                    ? 1 << 28 : 0); // 0x10000000
    bitfield |= (digitalInput[1]                    ? 1 << 29 : 0); // 0x20000000
    bitfield |= (digitalInput[2]                    ? 1 << 30 : 0); // 0x40000000
    bitfield |= (digitalInput[3]                    ? 1 << 31 : 0); // 0x80000000

    return bitfield;
}
