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

class Status {
public:
    enum SystemState {
        unknown     = 0, // at start-up the system state is unknown (next states: init, error)
        init        = 1, // the system is being initialized and is not ready for operation yet (next states: preCharge, ready, error)
        preCharge   = 2, // the system is initialized and executing the pre-charge cycle (next states: ready, error)
        ready       = 3, // the system is ready to accept commands but the motor controller's power stage is inactive (next states: running, error)
        running     = 4, // the system is running and the power stage of the motor controller is active (next states: ready, error)
        error       = 99 // the system is in an error state and not operational (no power on motor, turn of power stage)
    };

    // status flags
    static bool warning; // a general warning condition is present but the system could still be/become operational

    // power limitation flags
    static bool limitationTorque; // a general indicator that torque is limited by one or more limiters
    static bool limitationMaxTorque; // torque limitation active due to configured/allowed maximum torque
    static bool limitationSpeed; // torque limitation active due to configured maximum speed
    static bool limitationControllerTemperature; // torque limitation active due to high temperature of the motor controller
    static bool limitationMotorTemperature; // torque limitation active due to high motor temperature
    static bool limitationSlewRate; // torque limitation active due to configured torque/speed slew rate
    static bool limitationMotorModel; // torque limitation active due to the motor model
    static bool limitationMechanicalPower; // torque limitation active due to configured maximum mechanical power
    static bool limitationAcVoltage; // torque limitation active due to configured maximum AC voltage
    static bool limitationAcCurrent; // torque limitation active due to configured maximum AC current
    static bool limitationDcVoltage; // torque limitation active due to configured DC voltage limits
    static bool limitationDcCurrent; // torque limitation active due to configured DC current limits (motoring or regenerating)

    // warning flags
    static bool driverShutdownPathActive; // a shut-down path of the motor controller is active and prevents activation of the power stage
    static bool externalShutdownPath1Off; // the external shut-down path 1 of the motor controller is switched off
    static bool externalShutdownPath2Off; // the external shut-down path 2 of the motor controller is switched off
    static bool oscillationLimitControllerActive; // the oscillation limiter of the motor controller is active to dampen oscillations in the requested torque
    static bool speedSensorSignal; // the speed sensor signal is bad but not bad enough to report an error (e.g. certain amount of lost position counts or invalid transitions)
    static bool maximumModulationLimiter; // the motor's maximum modulation limiter is active
    static bool temperatureSensor; // invalid data is received from one or a group of temperature sensors

    // error flags
    static bool speedSensor; // the encoder or position sensor deliver a faulty signal
    static bool speedSensorSupply; // power supply of the speed sensor failed
    static bool canLimitMessageInvalid; // the limit data of the CAN message sent to the motor controller is invalid
    static bool canControlMessageInvalid; // the control data of the CAN message sent to the motor controller is invalid
    static bool canLimitMessageLost; // timeout of CAN message with limit data
    static bool overvoltageInternalSupply; // over voltage of the internal power supply of the motor controller
    static bool voltageMeasurement; // the motor controller detected differences in the redundant voltage measurement
    static bool shortCircuit; // short circuit in the power stage
    static bool canControlMessageLost; // timeout of CAN message with control data
    static bool canControl2MessageLost; // timeout of CAN message with supplementary control data
    static bool overtempController; // severe over temperature in motor controller
    static bool overtempMotor; // severe over temperature in motor
    static bool overspeed; // over speed detected
    static bool hvUndervoltage; // the HV voltage dropped below the motor controller's limits
    static bool hvOvervoltage; // the HV voltage exceeded the motor controller's limits
    static bool hvOvercurrent; // the HV current exceeded the motor controller's limits
    static bool acOvercurrent; // the demanded AC current would exceed / exceeds the allowed maximum current
    static bool initalisation; // error during initialisation
    static bool analogInput; // an analog input signal is outside its boundaries
    static bool unexpectedShutdown; // the power stage of the motor controller was shut-down in an uncontrolled fashion
    static bool powerMismatch; // plausibility error between electrical and mechanical power
    static bool motorEeprom; // error in motor/controller EEPROM module
    static bool storage; // data consistency check failed in motor controller
    static bool enableSignalLost; // the enable signal was lost, motor controller shut-down (is imminent)
    static bool canCommunicationStartup; // the motor controller received CAN messages which were not appropriate to its state
    static bool internalSupply; // problem with the internal power supply of the motor controller
    static bool osTrap; // a severe problem in the operation system of the motor controller occured

    static SystemState getSystemState();
    static SystemState setSystemState(SystemState);
    static uint32_t getBitField1();
    static uint32_t getBitField2();
    static uint32_t getBitField3();

private:
    static SystemState systemState; // the current state of the system, only to be modified by the state machine of this class
    static char *systemStateToStr(SystemState);

    /*
     // Message id=0x258, DMC_TRQS
     // The value is composed of 2 bytes: (data[1] << 0) | (data[0] << 8)
     enum Status {
     motorModelLimitation        = 1 << 0,  // 0x0001
     mechanicalPowerLimitation   = 1 << 1,  // 0x0002
     maxTorqueLimitation         = 1 << 2,  // 0x0004
     acCurrentLimitation         = 1 << 3,  // 0x0008
     temperatureLimitation       = 1 << 4,  // 0x0010
     speedLimitation             = 1 << 5,  // 0x0020
     voltageLimitation           = 1 << 6,  // 0x0040
     currentLimitation           = 1 << 7,  // 0x0080
     torqueLimitation            = 1 << 8,  // 0x0100
     errorFlag                   = 1 << 9,  // 0x0200, see DMC_ERR
     warningFlag                 = 1 << 10, // 0x0400, see DMC_ERR
     slewRateLimitation          = 1 << 12, // 0x1000
     motorTemperatureLimitation  = 1 << 13, // 0x2000
     stateRunning                = 1 << 14, // 0x4000
     stateReady                  = 1 << 15  // 0x8000
     };

     // Message id=0x25a, DMC_ERR
     // The value is composed of 2 bytes: (data[7] << 0) | (data[6] << 8)
     enum Warning {
     systemCheckActive                   = 1 << 0,  // 0x0001
     externalShutdownPathAw2Off          = 1 << 1,  // 0x0002
     externalShutdownPathAw1Off          = 1 << 2,  // 0x0004
     oscillationLimitControllerActive    = 1 << 3,  // 0x0008
     driverShutdownPathActive            = 1 << 10, // 0x0400
     powerMismatchDetected               = 1 << 11, // 0x0800
     speedSensorSignal                   = 1 << 12, // 0x1000
     hvUndervoltage                      = 1 << 13, // 0x2000
     maximumModulationLimiter            = 1 << 14, // 0x4000
     temperatureSensor                   = 1 << 15, // 0x8000
     };

     // Message id=0x25a, DMC_ERR
     // The error value is composed of 4 bytes : (data[1] << 0) | (data[0] << 8) | (data[5] << 16) | (data[4] << 24)
     enum Error {
     speedSensorSupply           = 1 << 0,  // 0x00000001, data[1]
     speedSensor                 = 1 << 1,  // 0x00000002, data[1]
     canLimitMessageInvalid      = 1 << 2,  // 0x00000004, data[1]
     canControlMessageInvalid    = 1 << 3,  // 0x00000008, data[1]
     canLimitMessageLost         = 1 << 4,  // 0x00000010, data[1]
     overvoltageSkyConverter     = 1 << 5,  // 0x00000020, data[1]
     voltageMeasurement          = 1 << 6,  // 0x00000040, data[1]
     shortCircuit                = 1 << 7,  // 0x00000080, data[1]
     canControlMessageLost       = 1 << 8,  // 0x00000100, data[0]
     overtemp                    = 1 // the system is running and the power stage of the controller is active<< 9,  // 0x00000200, data[0]
     overtempMotor               = 1 << 10, // 0x00000400, data[0]
     overspeed                   = 1 << 11, // 0x00000800, data[0]
     undervoltage                = 1 << 12, // 0x00001000, data[0]
     overvoltage                 = 1 << 13, // 0x00002000, data[0]
     overcurrent                 = 1 << 14, // 0x00004000, data[0]
     initalisation               = 1 << 15, // 0x00008000, data[0]
     analogInput                 = 1 << 16, // 0x00010000, data[5]
     driverShutdown              = 1 << 22, // 0x00400000, data[5]
     powerMismatch               = 1 << 23, // 0x00800000, data[5]
     canControl2MessageLost      = 1 << 24, // 0x01000000, data[4]
     motorEeprom                 = 1 << 25, // 0x02000000, data[4]
     storage                     = 1 << 26, // 0x04000000, data[4]
     enablePinSignalLost         = 1 << 27, // 0x08000000, data[4]
     canCommunicationStartup     = 1 << 28, // 0x10000000, data[4]
     internalSupply              = 1 << 29, // 0x20000000, data[4]
     acOvercurrent               = 1 << 30, // 0x40000000, data[4]
     osTrap                      = 1 << 31  // 0x80000000, data[4]
     };
     */
};
#endif /* STATUS_H_ */
