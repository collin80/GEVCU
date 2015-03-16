/*
 * BrusaDMC5.h
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

#ifndef BRUSADMC5_H_
#define BRUSADMC5_H_

#include <Arduino.h>
#include "config.h"
#include "Status.h"
#include "MotorController.h"
#include "SystemIO.h"
#include "TickHandler.h"
#include "DeviceManager.h"
#include "ichip_2128.h"
#include "DeviceTypes.h"

// CAN bus id's for frames sent to DMC5

#define CAN_ID_CONTROL      0x210 // send commands (DMC_CTRL)
#define CAN_ID_LIMIT        0x211 // send limitations (DMC_LIM)
#define CAN_ID_CONTROL_2    0x212 // send commands (DMC_CTRL2)

// CAN bus id's for frames received from DMC5

#define CAN_ID_STATUS           0x258 // receive (limit) status message (DMC_TRQS)      01001011000
#define CAN_ID_ACTUAL_VALUES    0x259 // receive actual values (DMC_ACTV)               01001011001
#define CAN_ID_ERRORS           0x25a // receive error codes (DMC_ERR)                  01001011010
#define CAN_ID_TORQUE_LIMIT     0x268 // receive torque limit information (DMC_TRQS2)   01001101000
#define CAN_MASK_1              0x7cc // mask for above id's                            11111001100
#define CAN_MASKED_ID_1         0x248 // masked id for id's from 0x258 to 0x268         01001001000

#define CAN_ID_TEMP         0x458 // receive temperature information (DMC_TEMP)         10001011000
#define CAN_MASK_2          0x7ff // mask for above id's                                11111111111
#define CAN_MASKED_ID_2     0x458 // masked id for id's from 0x258 to 0x268             10001011000

class BrusaDMC5Configuration : public MotorControllerConfiguration
{
public:
    // DMC_CTRL2
    uint16_t maxMechanicalPowerMotor; // maximal mechanical power of motor in 4W steps
    uint16_t maxMechanicalPowerRegen; // maximal mechanical power of regen in 4W steps

    // DMC_LIM
    uint16_t dcVoltLimitMotor; // minimum DC voltage limit for motoring in 0.1V
    uint16_t dcVoltLimitRegen; //  maximum DC voltage limit for regen in 0.1V
    uint16_t dcCurrentLimitMotor; // current limit for motoring in 0.1A
    uint16_t dcCurrentLimitRegen; // current limit for regen in 0.1A

    bool enableOscillationLimiter; // this will enable the DMC5 oscillation limiter (if also enabled by parameter)
};

class BrusaDMC5: public MotorController
{
public:
    // Message id=0x258, DMC_TRQS
    // The value is composed of 2 bytes: (data[1] << 0) | (data[0] << 8)
    enum DMC5_Status {
        motorModelLimitation        = 1 << 0,  // 0x0001, data[1], Motorola bit 15
        mechanicalPowerLimitation   = 1 << 1,  // 0x0002, data[1], Motorola bit 14
        maxTorqueLimitation         = 1 << 2,  // 0x0004, data[1], Motorola bit 13
        acCurrentLimitation         = 1 << 3,  // 0x0008, data[1], Motorola bit 12
        temperatureLimitation       = 1 << 4,  // 0x0010, data[1], Motorola bit 11
        speedLimitation             = 1 << 5,  // 0x0020, data[1], Motorola bit 10
        voltageLimitation           = 1 << 6,  // 0x0040, data[1], Motorola bit 9
        currentLimitation           = 1 << 7,  // 0x0080, data[1], Motorola bit 8

        torqueLimitation            = 1 << 8,  // 0x0100, data[0], Motorola bit 7
        errorFlag                   = 1 << 9,  // 0x0200, data[0], Motorola bit 6
        warningFlag                 = 1 << 10, // 0x0400, data[0], Motorola bit 5
        slewRateLimitation          = 1 << 12, // 0x1000, data[0], Motorola bit 3
        motorTemperatureLimitation  = 1 << 13, // 0x2000, data[0], Motorola bit 2
        stateRunning                = 1 << 14, // 0x4000, data[0], Motorola bit 1
        stateReady                  = 1 << 15  // 0x8000, data[0], Motorola bit 0
    };

    // Message id=0x25a, DMC_ERR
    // The value is composed of 2 bytes: (data[7] << 0) | (data[6] << 8)
    enum DMC5_Warning {
        systemCheckActive                   = 1 << 0,  // 0x0001, data[6], Motorola bit 63
        externalShutdownPathAw2Off          = 1 << 1,  // 0x0002, data[6], Motorola bit 62
        externalShutdownPathAw1Off          = 1 << 2,  // 0x0004, data[6], Motorola bit 61
        oscillationLimitControllerActive    = 1 << 3,  // 0x0008, data[6], Motorola bit 60

        driverShutdownPathActive            = 1 << 10, // 0x0400, data[7], Motorola bit 53
        powerMismatchDetected               = 1 << 11, // 0x0800, data[7], Motorola bit 52
        speedSensorSignal                   = 1 << 12, // 0x1000, data[7], Motorola bit 51
        hvUndervoltage                      = 1 << 13, // 0x2000, data[7], Motorola bit 50
        maximumModulationLimiter            = 1 << 14, // 0x4000, data[7], Motorola bit 49
        temperatureSensor                   = 1 << 15, // 0x8000, data[7], Motorola bit 48
    };

    // Message id=0x25a, DMC_ERR
    // The error value is composed of 4 bytes : (data[1] << 0) | (data[0] << 8) | (data[5] << 16) | (data[4] << 24)
    enum DMC5_Error {
        speedSensorSupply           = 1 << 0,  // 0x00000001, data[1], Motorola bit 15
        speedSensor                 = 1 << 1,  // 0x00000002, data[1], Motorola bit 14
        canLimitMessageInvalid      = 1 << 2,  // 0x00000004, data[1], Motorola bit 13
        canControlMessageInvalid    = 1 << 3,  // 0x00000008, data[1], Motorola bit 12
        canLimitMessageLost         = 1 << 4,  // 0x00000010, data[1], Motorola bit 11
        overvoltageSkyConverter     = 1 << 5,  // 0x00000020, data[1], Motorola bit 10
        voltageMeasurement          = 1 << 6,  // 0x00000040, data[1], Motorola bit 9
        shortCircuit                = 1 << 7,  // 0x00000080, data[1], Motorola bit 8

        canControlMessageLost       = 1 << 8,  // 0x00000100, data[0], Motorola bit 7
        overtemp                    = 1 << 9,  // 0x00000200, data[0], Motorola bit 6
        overtempMotor               = 1 << 10, // 0x00000400, data[0], Motorola bit 5
        overspeed                   = 1 << 11, // 0x00000800, data[0], Motorola bit 4
        undervoltage                = 1 << 12, // 0x00001000, data[0], Motorola bit 3
        overvoltage                 = 1 << 13, // 0x00002000, data[0], Motorola bit 2
        overcurrent                 = 1 << 14, // 0x00004000, data[0], Motorola bit 1
        initalisation               = 1 << 15, // 0x00008000, data[0], Motorola bit 0

        analogInput                 = 1 << 16, // 0x00010000, data[5], Motorola bit 47
        driverShutdown              = 1 << 22, // 0x00400000, data[5], Motorola bit 41
        powerMismatch               = 1 << 23, // 0x00800000, data[5], Motorola bit 40

        canControl2MessageLost      = 1 << 24, // 0x01000000, data[4], Motorola bit 39
        motorEeprom                 = 1 << 25, // 0x02000000, data[4], Motorola bit 38
        storage                     = 1 << 26, // 0x04000000, data[4], Motorola bit 37
        enablePinSignalLost         = 1 << 27, // 0x08000000, data[4], Motorola bit 36
        canCommunicationStartup     = 1 << 28, // 0x10000000, data[4], Motorola bit 35
        internalSupply              = 1 << 29, // 0x20000000, data[4], Motorola bit 34
        acOvercurrent               = 1 << 30, // 0x40000000, data[4], Motorola bit 33
        osTrap                      = 1 << 31  // 0x80000000, data[4], Motorola bit 32
    };

    // Message id=0x210, DMC_CTRL
    // The value is composed of 1 byte : data[0]
    enum DMC5_Control {
        enablePositiveTorqueSpeed   = 1 << 0, // 0x01, data[0], Motorola bit 7
        enableNegativeTorqueSpeed   = 1 << 1, // 0x02, data[0], Motorola bit 6
        clearErrorLatch             = 1 << 3, // 0x08, data[0], Motorola bit 4
        enableOscillationLimiter    = 1 << 5, // 0x20, data[0], Motorola bit 2
        enableSpeedMode             = 1 << 6, // 0x40, data[0], Motorola bit 1
        enablePowerStage            = 1 << 7  // 0x80, data[0], Motorola bit 0
    };

    BrusaDMC5();
    void handleTick();
    void handleCanFrame(CAN_FRAME *frame);
    void setup();
    DeviceId getId();
    uint32_t getTickInterval();

    void loadConfiguration();
    void saveConfiguration();

private:
    // DMC_TRQS2
    int16_t maxPositiveTorque; // max positive available torque in 0.01Nm -> divide by 100 to get Nm
    int16_t minNegativeTorque; // minimum negative available torque in 0.01Nm
    uint8_t limiterStateNumber; // state number of active limiter
    uint32_t bitfield; // various bit fields

    int tickCounter; // count how many times handleTick() was called
    CAN_FRAME outputFrame; // the output CAN frame;

    void sendControl();
    void sendControl2();
    void sendLimits();
    void prepareOutputFrame(uint32_t);
    void processStatus(uint8_t data[]);
    void processActualValues(uint8_t data[]);
    void processErrors(uint8_t data[]);
    void processTorqueLimit(uint8_t data[]);
    void processTemperature(uint8_t data[]);
};

#endif /* BRUSADMC5_H_ */
