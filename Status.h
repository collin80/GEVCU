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
    bool oscillationLimiter; // the oscillation limiter of the motor controller is active to dampen oscillations in the requested torque
    bool maximumModulationLimiter; // the motor's maximum modulation limiter is active
    bool systemCheckActive; // is the system not ready yet because of a system check?

    // error flags
    bool overtempController; // severe over temperature in motor controller
    bool overtempMotor; // severe over temperature in motor
    bool overspeed; // over speed detected
    bool hvUndervoltage; // the HV voltage dropped below the motor controller's limits
    bool hvOvervoltage; // the HV voltage exceeded the motor controller's limits
    bool hvOvercurrent; // the HV current exceeded the motor controller's limits
    bool acOvercurrent; // the demanded AC current would exceed / exceeds the allowed maximum current

    bool brakeHold; // is brake hold acitve ?
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

    uint8_t stateOfCharge; // state of charge (in 0.5%)
    uint32_t flowCoolant; // ml per second coolant flow
    uint32_t flowHeater; // ml per second heater flow
    uint8_t statusLight; // 0 to 255 for the PWM of the status light

    bool digitalInput[CFG_NUMBER_DIGITAL_INPUTS]; // the the digital input x activated ?
    bool digitalOutput[CFG_NUMBER_DIGITAL_OUTPUTS]; // the the digital output x activated ?

    int16_t temperatureBattery[CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS]; // temperature reported via CAN from external device
    int16_t temperatureCoolant; // temperature of the coolant water, reported via CAN from GEVCU extension
    int16_t heaterTemperature; // temperature of the heater water, calculated from analog input
    int16_t temperatureExterior; // exterior temperature (ambient) reported via CAN from GEVCU extension
    uint16_t heaterPower; // the power of the heater in Watt
    uint8_t vehicleSpeed; // vehicle speed in kmh
    uint8_t barometricPressure; // barometric pressure in kPa
    bool dcdcRunning; // is the dcdc converter running ? (true if no dcdc converter is enabled)

    bool bmsDclLowSoc; //DischargeLimit Reduced Due To Low SOC
    bool bmsDclHighCellResistance; //DischargeLimit Reduced Due To High Cell Resistance
    bool bmsDclTemperature; //DischargeLimit Reduced Due To Temperature
    bool bmsDclLowCellVoltage; //DischargeLimit Reduced Due To Low Cell Voltage
    bool bmsDclLowPackVoltage; //DischargeLimit Reduced Due To Low Pack Voltage
    bool bmsDclCclVoltageFailsafe; //DischargeLimit and ChargeLimit Reduced Due To Voltage Failsafe
    bool bmsDclCclCommunication; //DischargeLimit and ChargeLimit Reduced Due To Communication Failsafe: This only applies if there are multiple BMS units connected together in series over CANBUS.
    bool bmsCclHighSoc; //ChargeLimit Reduced Due To High SOC
    bool bmsCclHighCellResistance; //ChargeLimit Reduced Due To High Cell Resistance
    bool bmsCclTemperature; //ChargeLimit Reduced Due To Temperature
    bool bmsCclHighCellVoltage; //ChargeLimit Reduced Due To High Cell Voltage
    bool bmsCclHighPackVoltage; //ChargeLimit Reduced Due To High Pack Voltage
    bool bmsCclChargerLatch; //ChargeLimit Reduced Due To Charger Latch): This means the ChargeLimit is likely 0A because the charger has been turned off. This latch is removed when the Charge Power signal is removed and re-applied (ie: unplugging the car and plugging it back in).
    bool bmsCclAlternate; //ChargeLimit Reduced Due To Alternate Current Limit [MPI]
    bool bmsRelayDischarge; // Discharge relay enabled
    bool bmsRelayCharge; // Charge relay enabled
    bool bmsChagerSafety; // Charger safety enabled
    bool bmsDtcPresent; // Malfunction indicator active (DTC status)
    bool bmsVoltageFailsafe;
    bool bmsCurrentFailsafe;
    bool bmsDepleted;
    bool bmsBalancingActive;
    bool bmsDtcWeakCellFault;
    bool bmsDtcLowCellVolage;
    bool bmsDtcHVIsolationFault;
    bool bmsDtcVoltageRedundancyFault;

    Status();
    SystemState getSystemState();
    SystemState setSystemState(SystemState);
    char *systemStateToStr(SystemState);
    uint32_t getBitFieldMotor();
    uint32_t getBitFieldBms();
    uint32_t getBitFieldIO();
    int16_t getLowestBatteryTemperature();
    int16_t getHighestBatteryTemperature();

private:
    SystemState systemState; // the current state of the system, to be modified by the state machine of this class only
};

extern Status status;

#endif /* STATUS_H_ */
