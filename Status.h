/*
 * Status.h
 *
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

#ifndef STATUS_H_
#define STATUS_H_

#include <Arduino.h>
#include "Logger.h"
#include "Sys_Messages.h"

class Status
{
public:
    enum SystemState
    {
        startup     = 0, // the system is starting-up (next states: init, error)
        init        = 1, // the system is being initialized and is not ready for operation yet (next states: preCharge, ready, error)
        preCharge   = 2, // the system is initialized and executing the pre-charge cycle (next states: ready, error)
        preCharged  = 3, // the system is pre-charged, the pre-charge cycle is finished
        batteryHeating = 4, // before charging, the batteries need to be heated
        charging    = 5, // the batteries are being charged
        charged     = 6, // the charging is finished
        ready       = 7, // the system is ready to accept commands but the motor controller is not enabled yet (next states: running, error)
        running     = 8, // the system is running and the motor controller is to be enabled (next states: ready, error)
        shutdown    = 9, // the system is shutdown and must be restarted to get operational again
        error       = 99 // the system is in an error state and not operational (no power on motor, turn of power stage)
    };

    // power limitation flags
    bool limitationTorque; // a general indicator that torque is limited by one or more limiters
    bool limitationMaxTorque; // torque limitation active due to configured/allowed maximum torque
    bool limitationSpeed; // torque limitation active due to configured maximum speed
    bool limitationControllerTemperature; // torque limitation active due to high temperature of the motor controller
    bool limitationMotorTemperature; // torque limitation active due to high motor temperature
    bool limitationSlewRate; // torque limitation active due to configured torque/speed slew rate
    bool limitationMotorModel; // torque limitation active due to the motor model
    bool limitationMechanicalPower; // torque limitation active due to configured maximum mechanical power
    bool limitationAcVoltage; // torque limitation active due to configured maximum AC voltage
    bool limitationAcCurrent; // torque limitation active due to configured maximum AC current
    bool limitationDcVoltage; // torque limitation active due to configured DC voltage limits
    bool limitationDcCurrent; // torque limitation active due to configured DC current limits (motoring or regenerating)

    // warning flags
    bool warning; // a general warning condition is present but the system could still be/become operational
    bool driverShutdownPathActive; // a shut-down path of the motor controller is active and prevents activation of the power stage
    bool externalShutdownPath1Off; // the external shut-down path 1 of the motor controller is switched off
    bool externalShutdownPath2Off; // the external shut-down path 2 of the motor controller is switched off
    bool oscillationLimitControllerActive; // the oscillation limiter of the motor controller is active to dampen oscillations in the requested torque
    bool speedSensorSignal; // the speed sensor signal is bad but not bad enough to report an error (e.g. certain amount of lost position counts or invalid transitions)
    bool maximumModulationLimiter; // the motor's maximum modulation limiter is active
    bool temperatureSensor; // invalid data is received from one or a group of temperature sensors
    bool systemCheckActive; // is the system not ready yet because of a system check?

    // error flags
    bool speedSensor; // the encoder or position sensor deliver a faulty signal
    bool speedSensorSupply; // power supply of the speed sensor failed
    bool canLimitMessageInvalid; // the limit data of the CAN message sent to the motor controller is invalid
    bool canControlMessageInvalid; // the control data of the CAN message sent to the motor controller is invalid
    bool canLimitMessageLost; // timeout of CAN message with limit data
    bool overvoltageInternalSupply; // over voltage of the internal power supply of the motor controller
    bool voltageMeasurement; // the motor controller detected differences in the redundant voltage measurement
    bool shortCircuit; // short circuit in the power stage
    bool canControlMessageLost; // timeout of CAN message with control data
    bool canControl2MessageLost; // timeout of CAN message with supplementary control data
    bool overtempController; // severe over temperature in motor controller
    bool overtempMotor; // severe over temperature in motor
    bool overspeed; // over speed detected
    bool hvUndervoltage; // the HV voltage dropped below the motor controller's limits
    bool hvOvervoltage; // the HV voltage exceeded the motor controller's limits
    bool hvOvercurrent; // the HV current exceeded the motor controller's limits
    bool acOvercurrent; // the demanded AC current would exceed / exceeds the allowed maximum current
    bool initalisation; // error during initialisation
    bool analogInput; // an analog input signal is outside its boundaries
    bool unexpectedShutdown; // the power stage of the motor controller was shut-down in an uncontrolled fashion
    bool powerMismatch; // plausibility error between electrical and mechanical power
    bool motorEeprom; // error in motor/controller EEPROM module
    bool storage; // data consistency check failed in motor controller
    bool enableSignalLost; // the enable signal was lost, motor controller shut-down (is imminent)
    bool canCommunicationStartup; // the motor controller received CAN messages which were not appropriate to its state
    bool internalSupply; // problem with the internal power supply of the motor controller
    bool osTrap; // a severe problem in the operation system of the motor controller occured

    bool preChargeRelay; // is the pre-charge relay activated ?
    bool mainContactor; // is the main contactor relay activated ?
    bool secondaryContactor; // is the secondary relay activated ?
    bool fastChargeContactor; // is the secondary relay activated ?

    bool enableMotor; // is the 'enable' output activated ?
    bool enableCharger; // is the charger (relay) activated ?
    bool enableDcDc; // is the dc dc (relay) activated ?
    bool enableHeater; // is the heater (relay) activated ?

    bool heaterValve; // is the heater valve relay enabled (battery / cabin heating)?
    bool heaterPump; // is the heater pump relay enabled ?
    bool coolingPump; // is the cooling pump relay activated ?
    bool coolingFan; // is the cooling relay activated ?

    bool brakeLight; // is the brake light relay activated ?
    bool reverseLight; // is the reverse light relay activated ?
    bool powerSteering; // is the power steering activated ?
    bool unused; // is the ... activated ?

    bool enableIn; // is the 'enable' input signal active ?
    bool chargePowerAvailable; // is shore power available (connected to charging station)
    bool interlockPresent; // is the interlock circuit closed and the signal available ?
    bool reverseInput; // is the reverse signal present ?
    bool absActive; // is ABS or another source active to disable any power output to the motor
    bool enableRegen; // is regen currently activated ?
    bool enableCreep; // is creep activated ?

    uint32_t energyConsumption; // accumulated consumption in wattSeconds (or kilowattmilliseconds)
    uint8_t stateOfCharge; // 0 to 255 to indicate the state of charge (divide by 2.55 to get percent)
    uint32_t flowCoolant; // ml per second coolant flow
    uint32_t flowHeater; // ml per second heater flow

    bool digitalInput[CFG_NUMBER_DIGITAL_INPUTS]; // the the digital input x activated ?
    bool digitalOutput[CFG_NUMBER_DIGITAL_OUTPUTS]; // the the digital output x activated ?

    int16_t temperatureBattery[CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS]; // temperature reported via CAN from external device
    int16_t temperatureCoolant; // temperature of the coolant water, reported via CAN from GEVCU extension
    int16_t heaterTemperature; // temperature of the heater water, calculated from analog input
    int16_t temperatureExterior; // exterior temperature (ambient) reported via CAN from GEVCU extension
    uint16_t heaterPower; // the power of the heater in Watt

    Status();
    SystemState getSystemState();
    SystemState setSystemState(SystemState);
    char *systemStateToStr(SystemState);
    uint32_t getBitField1();
    uint32_t getBitField2();
    uint32_t getBitField3();
    int16_t getLowestBatteryTemperature();
    int16_t getHighestBatteryTemperature();
    uint16_t getEnergyConsumption();

private:
    SystemState systemState; // the current state of the system, to be modified by the state machine of this class only
};

extern Status status;

#endif /* STATUS_H_ */
