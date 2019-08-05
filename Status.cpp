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
Status::Status()
{
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
    oscillationLimiter= false;
    maximumModulationLimiter        = false;
    systemCheckActive               = false;

    overtempController              = false;
    overtempMotor                   = false;
    overspeed                       = false;
    hvUndervoltage                  = false;
    hvOvervoltage                   = false;
    hvOvercurrent                   = false;
    acOvercurrent                   = false;

    preChargeRelay = false;
    mainContactor = false;
    secondaryContactor = false;
    fastChargeContactor = false;

    enableMotor = false;
    enableCharger = false;
    enableDcDc = false;
    enableHeater = false;
    enableRegen = true;
    enableCreep = true;

    heaterValve = false;
    heaterPump = false;
    coolingPump = false;
    coolingFan = false;

    brakeLight = false;
    reverseLight = false;
    powerSteering = false;
    unused = false;

    enableIn            = false;
    chargePowerAvailable= false;
    interlockPresent    = false;
    reverseInput        = false;
    absActive           = false;
    gearChangeActive    = false;
    statusLight         = false;
    brakeHold           = false;

    flowCoolant = 0;
    flowHeater = 0;
    heaterPower = 0;

    bmsDclLowSoc = false;
    bmsDclHighCellResistance = false;
    bmsDclTemperature = false;
    bmsDclLowCellVoltage = false;
    bmsDclLowPackVoltage = false;
    bmsDclCclVoltageFailsafe = false;
    bmsDclCclCommunication = false;
    bmsCclHighSoc = false;
    bmsCclHighCellResistance = false;
    bmsCclTemperature = false;
    bmsCclHighCellVoltage = false;
    bmsCclHighPackVoltage = false;
    bmsCclChargerLatch = false;
    bmsCclAlternate = false;
    bmsRelayDischarge = false;
    bmsRelayCharge = false;
    bmsChagerSafety = false;
    bmsDtcPresent = false;
    bmsVoltageFailsafe = false;
    bmsCurrentFailsafe = false;
    bmsDepleted = false;
    bmsBalancingActive = false;
    bmsDtcWeakCellFault = false;
    bmsDtcLowCellVolage = false;
    bmsDtcHVIsolationFault = false;
    bmsDtcVoltageRedundancyFault = false;

    for (int i = 0; i < CFG_NUMBER_DIGITAL_OUTPUTS; i++) {
        digitalOutput[i] = false;
    }
    for (int i = 0; i < CFG_NUMBER_DIGITAL_INPUTS; i++) {
        digitalInput[i] = false;
    }
    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        temperatureBattery[i] = CFG_NO_TEMPERATURE_DATA;
    }
    temperatureCoolant = CFG_NO_TEMPERATURE_DATA;
    heaterTemperature = CFG_NO_TEMPERATURE_DATA;
    temperatureExterior = CFG_NO_TEMPERATURE_DATA;
    barometricPressure = 0;
    vehicleSpeed = 0;

    stateOfCharge = 0;
    dcdcRunning = false;
    stateTimestamp = 0;
}

/*
 * Retrieve the current system state.
 */
Status::SystemState Status::getSystemState()
{
    return systemState;
}

/*
 * Set a new system state. The new system state is validated if the
 * transition is allowed from the old state. If an invalid transition is
 * attempted, the new state will be 'error'.
 * The old and new state are broadcast to all devices.
 */
Status::SystemState Status::setSystemState(SystemState newSystemState)
{
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
            if (newSystemState == ready || newSystemState == shutdown) {
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
        }
    }
    if (systemState == newSystemState) {
        Logger::info("switching to system state '%s'", systemStateToStr(systemState));
        stateTimestamp = millis();
    } else {
        Logger::error("switching from system state '%s' to '%s' is not allowed", systemStateToStr(systemState), systemStateToStr(newSystemState));
        systemState = error;
    }

    SystemState params[] = { oldSystemState, systemState };
    deviceManager.sendMessage(DEVICE_ANY, INVALID, MSG_STATE_CHANGE, params);

    return systemState;
}

int16_t Status::getLowestBatteryTemperature() {
    int16_t temp = CFG_NO_TEMPERATURE_DATA;

    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        temp = min(temp, temperatureBattery[i]);
    }
    return temp;
}

int16_t Status::getHighestBatteryTemperature() {
    int16_t temp = -9999;

    for (int i = 0; i < CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS; i++) {
        if (temperatureBattery[i] != CFG_NO_TEMPERATURE_DATA) {
            temp = max(temp, temperatureBattery[i]);
        }
    }
    return (temp == -9999 ? CFG_NO_TEMPERATURE_DATA : temp);
}

/*
 * Convert the current state into a string.
 */
char *Status::systemStateToStr(SystemState state)
{
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
    case shutdown:
        return "shut-down";
    }
    Logger::error("the system state is invalid, contact your support!");
    return "invalid";
}

/*
 * Calculate the bit field 1 from the respective flags for faster transmission
 * to the web site.
 *
 * Bitfield contains warnings, errors and torque limitation information from the motor controller
 */
uint32_t Status::getBitFieldMotor() {
    uint32_t bitfield = 0;

    // warnings
    bitfield |= (warning                            ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (oscillationLimiter                 ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (maximumModulationLimiter           ? 1 << 2 : 0);  // 0x00000004
//    bitfield |= (reserve                          ? 1 << 3 : 0);  // 0x00000008
//    bitfield |= (reserve                          ? 1 << 4 : 0);  // 0x00000010
//    bitfield |= (reserve                          ? 1 << 5 : 0);  // 0x00000020
//    bitfield |= (reserve                          ? 1 << 6 : 0);  // 0x00000040
//    bitfield |= (reserve                          ? 1 << 7 : 0);  // 0x00000080

    // errors
    bitfield |= (overtempController                 ? 1 << 8 : 0);  // 0x00000100
    bitfield |= (overtempMotor                      ? 1 << 9 : 0);  // 0x00000200
    bitfield |= (overspeed                          ? 1 << 10 : 0); // 0x00000400
    bitfield |= (hvUndervoltage                     ? 1 << 11 : 0); // 0x00000800
    bitfield |= (hvOvervoltage                      ? 1 << 12 : 0); // 0x00001000
    bitfield |= (hvOvercurrent                      ? 1 << 13 : 0); // 0x00002000
    bitfield |= (acOvercurrent                      ? 1 << 14 : 0); // 0x00004000
//    bitfield |= (reserve                          ? 1 << 15 : 0); // 0x00008000

    // limitations
    bitfield |= (limitationTorque                   ? 1 << 16 : 0); // 0x00010000
    bitfield |= (limitationMaxTorque                ? 1 << 17 : 0); // 0x00020000
    bitfield |= (limitationSpeed                    ? 1 << 18 : 0); // 0x00040000
    bitfield |= (limitationControllerTemperature    ? 1 << 19 : 0); // 0x00080000
    bitfield |= (limitationMotorTemperature         ? 1 << 20 : 0); // 0x00100000
    bitfield |= (limitationSlewRate                 ? 1 << 21 : 0); // 0x00200000
    bitfield |= (limitationMotorModel               ? 1 << 22 : 0); // 0x00400000
    bitfield |= (limitationMechanicalPower          ? 1 << 23 : 0); // 0x00800000
    bitfield |= (limitationAcVoltage                ? 1 << 24 : 0); // 0x01000000
    bitfield |= (limitationAcCurrent                ? 1 << 25 : 0); // 0x02000000
    bitfield |= (limitationDcVoltage                ? 1 << 26 : 0); // 0x04000000
    bitfield |= (limitationDcCurrent                ? 1 << 27 : 0); // 0x08000000
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
 * Bitfield contains bms information
 */
uint32_t Status::getBitFieldBms() {
    uint32_t bitfield = 0;

    // status
    bitfield |= (bmsRelayDischarge                  ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (bmsRelayCharge                     ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (bmsChagerSafety                    ? 1 << 2 : 0);  // 0x00000004
    bitfield |= (bmsDtcPresent                      ? 1 << 3 : 0);  // 0x00000008
//    bitfield |= (reserve                          ? 1 << 4 : 0);  // 0x00000010
//    bitfield |= (reserve                          ? 1 << 5 : 0);  // 0x00000020
//    bitfield |= (reserve                          ? 1 << 6 : 0);  // 0x00000040
//    bitfield |= (reserve                          ? 1 << 7 : 0);  // 0x00000080

    bitfield |= (bmsVoltageFailsafe                 ? 1 << 8 : 0);  // 0x00000100
    bitfield |= (bmsCurrentFailsafe                 ? 1 << 9 : 0);  // 0x00000200
    bitfield |= (bmsDepleted                        ? 1 << 10 : 0); // 0x00000400
    bitfield |= (bmsBalancingActive                 ? 1 << 11 : 0); // 0x00000800
    bitfield |= (bmsDtcWeakCellFault                ? 1 << 12 : 0); // 0x00001000
    bitfield |= (bmsDtcLowCellVolage                ? 1 << 13 : 0); // 0x00002000
    bitfield |= (bmsDtcHVIsolationFault             ? 1 << 14 : 0); // 0x00004000
    bitfield |= (bmsDtcVoltageRedundancyFault       ? 1 << 15 : 0); // 0x00008000

    // limitations
    bitfield |= (bmsDclLowSoc                       ? 1 << 16 : 0); // 0x00010000
    bitfield |= (bmsDclHighCellResistance           ? 1 << 17 : 0); // 0x00020000
    bitfield |= (bmsDclTemperature                  ? 1 << 18 : 0); // 0x00040000
    bitfield |= (bmsDclLowCellVoltage               ? 1 << 19 : 0); // 0x00080000
    bitfield |= (bmsDclLowPackVoltage               ? 1 << 20 : 0); // 0x00100000
    bitfield |= (bmsDclCclVoltageFailsafe           ? 1 << 21 : 0); // 0x00200000
    bitfield |= (bmsDclCclCommunication             ? 1 << 22 : 0); // 0x00400000
    bitfield |= (bmsCclHighSoc                      ? 1 << 23 : 0); // 0x00800000
    bitfield |= (bmsCclHighCellResistance           ? 1 << 24 : 0); // 0x01000000
    bitfield |= (bmsCclTemperature                  ? 1 << 25 : 0); // 0x02000000
    bitfield |= (bmsCclHighCellVoltage              ? 1 << 26 : 0); // 0x04000000
    bitfield |= (bmsCclHighPackVoltage              ? 1 << 27 : 0); // 0x08000000
    bitfield |= (bmsCclChargerLatch                 ? 1 << 28 : 0); // 0x10000000
    bitfield |= (bmsCclAlternate                    ? 1 << 29 : 0); // 0x20000000
//    bitfield |= (reserve                          ? 1 << 30 : 0); // 0x40000000
//    bitfield |= (reserve                          ? 1 << 31 : 0); // 0x80000000

    return bitfield;
}

/*
 * Calculate the bit field 3 from the respective flags for faster transmission
 * to the web site.
 *
 * Bitfield contains information about the digital inputs/outputs
 */
uint32_t Status::getBitFieldIO() {
    uint32_t bitfield = 0;

    bitfield |= (brakeHold                          ? 1 << 0 : 0);  // 0x00000001
    bitfield |= (preChargeRelay                     ? 1 << 1 : 0);  // 0x00000002
    bitfield |= (secondaryContactor                 ? 1 << 2 : 0);  // 0x00000004
    bitfield |= (mainContactor                      ? 1 << 3 : 0);  // 0x00000008
    bitfield |= (enableMotor                        ? 1 << 4 : 0);  // 0x00000010
    bitfield |= (coolingFan                         ? 1 << 5 : 0);  // 0x00000020
    bitfield |= (brakeLight                         ? 1 << 6 : 0);  // 0x00000040
    bitfield |= (reverseLight                       ? 1 << 7 : 0);  // 0x00000080
    bitfield |= (enableIn                           ? 1 << 8 : 0);  // 0x00000100
    bitfield |= (absActive                          ? 1 << 9 : 0);  // 0x00000200
//    bitfield |= (reserve                          ? 1 << 10 : 0); // 0x00000400
//    bitfield |= (reserve                          ? 1 << 11 : 0); // 0x00000800
//    bitfield |= (reserve                          ? 1 << 12 : 0); // 0x00001000
//    bitfield |= (reserve                          ? 1 << 13 : 0); // 0x00002000
//    bitfield |= (reserve                          ? 1 << 14 : 0); // 0x00004000
//    bitfield |= (reserve                          ? 1 << 15 : 0); // 0x00008000
//    bitfield |= (reserve                          ? 1 << 16 : 0); // 0x00010000
//    bitfield |= (reserve                          ? 1 << 17 : 0); // 0x00020000
//    bitfield |= (reserve                          ? 1 << 18 : 0); // 0x00040000
//    bitfield |= (reserve                          ? 1 << 19 : 0); // 0x00080000
//    bitfield |= (reserve                          ? 1 << 20 : 0); // 0x00100000
//    bitfield |= (reserve                          ? 1 << 21 : 0); // 0x00200000
//    bitfield |= (reserve                          ? 1 << 22 : 0); // 0x00400000
//    bitfield |= (reserve                          ? 1 << 23 : 0); // 0x00800000
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
