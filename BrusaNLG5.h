/*
 * BrusaNLG5.h
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

#ifndef BRUSANLG5_H_
#define BRUSANLG5_H_

#include <Arduino.h>
#include "config.h"
#include "Status.h"
#include "SystemIO.h"
#include "TickHandler.h"
#include "CanHandler.h"
#include "DeviceManager.h"
#include "DeviceTypes.h"
#include "Charger.h"

// CAN bus id's for frames sent to NLG5

#define CAN_ID_COMMAND      0x618 // send commands (NLG5_CTL)

// CAN bus id's for frames received from NLG5

#define CAN_ID_STATUS       0x610 // receive status information (NLG5_ST)               11000010000
#define CAN_ID_VALUES_1     0X611 // receive actual values information (NLG5_ACT_I)     11000010001
#define CAN_ID_VALUES_2     0X612 // receive actual values information (NLG5_ACT_II)    11000010010
#define CAN_ID_TEMPERATURE  0X613 // receive debug information (NLG5_TEMP)              11000010011
#define CAN_ID_ERROR        0X614 // receive debug information (NLG5_ERR)               11000010100
#define CAN_MASK            0x7f8 // mask for above id's                                11111111000
#define CAN_MASKED_ID       0x610 // masked id for id's from 0x610 to 0x614             11000010000

class BrusaNLG5Configuration : public DeviceConfiguration
{
public:
    uint16_t maxMainsCurrent; // maximum mains current in 0.1A
    uint16_t constantCurrent; // current during constant current phase in 0.1A
    uint16_t constantVoltage; // current during constant voltage phase and point where switching from constant current to constant voltage in 0.1V
    uint16_t terminateCurrent; // current at which to terminate the charge process in 0.1A
    uint16_t minimumBatteryVoltage; // minimum battery voltage where to start the charge process in 0.1V
    uint16_t maximumBatteryVoltage; // maximum battery voltage - if exceeded, the charge process will terminate in 0.1V
};

class BrusaNLG5: public Charger, CanObserver
{
public:
    // Message id=0x610, NLG5_ST
    // The error value is composed of 4 bytes : (data[1] << 0) | (data[0] << 8) | (data[3] << 16) | (data[2] << 24)
    enum NLG5_Status {
        limitPowerMaximum           = 1 << 0,  // 0x00000001, data[1], Motorola bit 15
        limitPowerControlPilot      = 1 << 1,  // 0x00000002, data[1], Motorola bit 14
        limitPowerIndicatorInput    = 1 << 2,  // 0x00000004, data[1], Motorola bit 13
        limitMainsCurrent           = 1 << 3,  // 0x00000008, data[1], Motorola bit 12
        limitBatteryCurrent         = 1 << 4,  // 0x00000010, data[1], Motorola bit 11
        limitBatteryVoltage         = 1 << 5,  // 0x00000020, data[1], Motorola bit 10
        bypassDetection1            = 1 << 6,  // 0x00000040, data[1], Motorola bit 9
        bypassDetection2            = 1 << 7,  // 0x00000080, data[1], Motorola bit 8

        controlPilotSignal          = 1 << 8,  // 0x00000100, data[0], Motorola bit 7
        usMainsLevel2               = 1 << 9,  // 0x00000200, data[0], Motorola bit 6
        usMainsLevel1               = 1 << 10, // 0x00000400, data[0], Motorola bit 5
        euMains                     = 1 << 11, // 0x00000800, data[0], Motorola bit 4
        coolingFan                  = 1 << 12, // 0x00001000, data[0], Motorola bit 3
        warning                     = 1 << 13, // 0x00002000, data[0], Motorola bit 2
        error                       = 1 << 14, // 0x00004000, data[0], Motorola bit 1
        hardwareEnabled             = 1 << 15, // 0x00008000, data[0], Motorola bit 0

        automaticChargingActive     = 1 << 23, // 0x00800000, data[3], Motorola bit 31

        limitBatteryTemperature     = 1 << 24, // 0x01000000, data[2], Motorola bit 23
        limitTransformerTemperature = 1 << 25, // 0x02000000, data[2], Motorola bit 22
        limitDiodesTemperature      = 1 << 26, // 0x04000000, data[2], Motorola bit 21
        limitPowerStageTemperature  = 1 << 27, // 0x08000000, data[2], Motorola bit 20
        limitCapacitorTemperature   = 1 << 28, // 0x10000000, data[2], Motorola bit 19
        limitMaximumOutputVoltage   = 1 << 29, // 0x20000000, data[2], Motorola bit 18
        limitMaximumOutputCurrent   = 1 << 30, // 0x40000000, data[2], Motorola bit 17
        limitMaximumMainsCurrent    = 1 << 31  // 0x80000000, data[2], Motorola bit 16
    };

    // Message id=0x614, NLG5_ERR
    // The error value is composed of 4 bytes : (data[1] << 0) | (data[0] << 8) | (data[3] << 16) | (data[2] << 24)
    enum NLG5_Error {
        extTemperatureSensor3Defect = 1 << 0,  // 0x00000001, data[1], Motorola bit 15
        extTemperatureSensor2Defect = 1 << 1,  // 0x00000002, data[1], Motorola bit 14
        extTemperatureSensor1Defect = 1 << 2,  // 0x00000004, data[1], Motorola bit 13
        temperatureSensorTransformer= 1 << 3,  // 0x00000008, data[1], Motorola bit 12
        temperatureSensorDiodes     = 1 << 4,  // 0x00000010, data[1], Motorola bit 11
        temperatureSensorPowerStage = 1 << 5,  // 0x00000020, data[1], Motorola bit 10
        temperatureSensorCapacitor  = 1 << 6,  // 0x00000040, data[1], Motorola bit 9
        batteryPolarity             = 1 << 7,  // 0x00000080, data[1], Motorola bit 8

        mainFuseDefective           = 1 << 8,  // 0x00000100, data[0], Motorola bit 7
        outputFuseDefective         = 1 << 9,  // 0x00000200, data[0], Motorola bit 6
        mainsVoltagePlausibility    = 1 << 10, // 0x00000400, data[0], Motorola bit 5
        batteryVoltagePlausibility  = 1 << 11, // 0x00000800, data[0], Motorola bit 4
        shortCircuit                = 1 << 12, // 0x00001000, data[0], Motorola bit 3
        mainsOvervoltage1           = 1 << 13, // 0x00002000, data[0], Motorola bit 2
        mainsOvervoltage2           = 1 << 14, // 0x00004000, data[0], Motorola bit 1
        batteryOvervoltage          = 1 << 15, // 0x00008000, data[0], Motorola bit 0

        emergencyChargeTime         = 1 << 18, // 0x00040000, data[3], Motorola bit 29
        emergencyAmpHours           = 1 << 19, // 0x00080000, data[3], Motorola bit 28
        emergencyBatteryVoltage     = 1 << 20, // 0x00100000, data[3], Motorola bit 27
        emergencyBatteryTemperature = 1 << 21, // 0x00200000, data[3], Motorola bit 26
        canReceiveOverflow          = 1 << 22, // 0x00400000, data[3], Motorola bit 25
        canTransmitOverflow         = 1 << 23, // 0x00800000, data[3], Motorola bit 24

        canOff                      = 1 << 24, // 0x01000000, data[2], Motorola bit 23
        canTimeout                  = 1 << 25, // 0x02000000, data[2], Motorola bit 22
        initializationError         = 1 << 26, // 0x04000000, data[2], Motorola bit 21
        watchDogTimeout             = 1 << 27, // 0x08000000, data[2], Motorola bit 20
        crcPowerEeprom              = 1 << 28, // 0x10000000, data[2], Motorola bit 19
        crcSystemEeprom             = 1 << 29, // 0x20000000, data[2], Motorola bit 18
        crcNVSRAM                   = 1 << 30, // 0x40000000, data[2], Motorola bit 17
        crcFlashMemory              = 1 << 31  // 0x80000000, data[2], Motorola bit 16
    };

    // Message id=0x614, NLG_ERR
    // The value is composed of 1 byte : data[4]
    enum NLG5_Warning {
        saveCharging            = 1 << 0, // 0x01, data[4], Motorola bit 39
        ledDriver               = 1 << 1, // 0x02, data[4], Motorola bit 38
        controlMessageNotActive = 1 << 3, // 0x08, data[4], Motorola bit 36
        valueOutOfRange         = 1 << 4, // 0x10, data[4], Motorola bit 35
        limitOvertemperature    = 1 << 5, // 0x20, data[4], Motorola bit 34
        limitLowBatteryVoltage  = 1 << 6, // 0x40, data[4], Motorola bit 33
        limitLowMainsVoltage    = 1 << 7  // 0x80, data[4], Motorola bit 32
    };

    // Message id=0x618, NLG5COM
    // The value is composed of 1 byte : data[0]
    enum NLG5_Command {
        controlPilotStageC  = 1 << 4, // 0x10, data[0], Motorola bit 3
        facilityVentilation = 1 << 5, // 0x20, data[0], Motorola bit 2
        errorLatch          = 1 << 6, // 0x40, data[0], Motorola bit 1
        enable              = 1 << 7, // 0x80, data[0], Motorola bit 0
    };

    BrusaNLG5();
    void handleTick();
    void handleCanFrame(CAN_FRAME *frame);
    void setup();
    DeviceType getType();
    DeviceId getId();
    uint32_t getTickInterval();

    void loadConfiguration();
    void saveConfiguration();

private:
    CanHandler *canHandlerEv;
    uint32_t bitfield; // various bit fields
    uint16_t mainsCurrent; // 0 - 50A in 0.01A
    uint16_t mainsVoltage; // 0 - 500V in 0.1V
    uint16_t batteryVoltage; // 0 - 1000V in 0.1V
    uint16_t batteryCurrent; // 0 - 150A in 0.01A
    uint16_t currentLimitControlPilot; // 0 - 100A in 0.1A
    uint8_t currentLimitPowerIndicator; // 0 - 20A in 0.1A
    uint8_t auxBatteryVoltage; // 0 - 25V in 0.1V
    int16_t extChargeBalance; // -327.68 - 327.67Ah in 0.01Ah
    uint16_t boosterOutputCurrent; // 0 - 50A in 0.01A
    int16_t temperaturePowerStage; // -40 - 300°C in 0.1°C
    int16_t temperatureExtSensor1; // -40 - 300°C in 0.1°C
    int16_t temperatureExtSensor2; // -40 - 300°C in 0.1°C
    int16_t temperatureExtSensor3; // -40 - 300°C in 0.1°C
    bool errorPresent;
    bool clearErrorLatch;

    CAN_FRAME outputFrame; // the output CAN frame;

    void sendControl();
    void prepareOutputFrame(uint32_t);
    void processStatus(uint8_t data[]);
    void processValues1(uint8_t data[]);
    void processValues2(uint8_t data[]);
    void processTemperature(uint8_t data[]);
    void processError(uint8_t data[]);
};

#endif /* BRUSANLG5_H_ */
