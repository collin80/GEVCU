/*
 * BrusaBSC6.h
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

#ifndef BRUSABSC6_H_
#define BRUSABSC6_H_

#include <Arduino.h>
#include "config.h"
#include "Status.h"
#include "SystemIO.h"
#include "TickHandler.h"
#include "DeviceManager.h"
#include "DeviceTypes.h"
#include "DcDcConverter.h"

// CAN bus id's for frames sent to BSC6

#define CAN_ID_COMMAND      0x260 // send commands (BSC6COM)
#define CAN_ID_LIMIT        0x261 // send limitations (BSC6LIM)

// CAN bus id's for frames received from BSC6

#define CAN_ID_VALUES_1     0x26a // receive actual values and status info (BSC6VAL1)   01001101010
#define CAN_ID_VALUES_2     0x26b // receive actual values and error codes (BSC6VAL2)   01001101011
#define CAN_ID_DEBUG_1      0X26e // receive debug information (BSC6DBG1)               01001101110
#define CAN_ID_DEBUG_2      0X26f // receive debug information (BSC6DBG2)               01001101111
#define CAN_MASK            0x7fa // mask for above id's                                11111111010
#define CAN_MASKED_ID       0x26a // masked id for id's from 0x26a to 0x26f             01001101010

class BrusaBSC6Configuration : public DcDcConverterConfiguration
{
public:
    bool debugMode;
};

class BrusaBSC6: public DcDcConverter, CanObserver
{
public:
    // Message id=0x26a, BSC6VAL1
    // The value is composed of 1 byte : data[7]
    enum BSC6_Status {
        bsc6Running         = 1 << 0, // 0x01, data[7], Motorola bit 63
        bsc6Ready           = 1 << 1, // 0x02, data[7], Motorola bit 62
        automatic           = 1 << 2  // 0x04, data[7], Motorola bit 61
    };

    // Message id=0x26b, BSC6VAL2
    // The error value is composed of 3 bytes : (data[3] << 0) | (data[2] << 8) | (data[4] << 16)
    enum BSC6_Error {
        lowVoltageUndervoltage  = 1 << 0,  // 0x000001, data[3], Motorola bit 31
        lowVoltageOvervoltage   = 1 << 1,  // 0x000002, data[3], Motorola bit 30
        highVoltageUndervoltage = 1 << 2,  // 0x000004, data[3], Motorola bit 29
        highVoltageOvervoltage  = 1 << 3,  // 0x000008, data[3], Motorola bit 28
        internalSupply          = 1 << 4,  // 0x000010, data[3], Motorola bit 27
        temperatureSensor       = 1 << 5,  // 0x000020, data[3], Motorola bit 26
        trafoStartup            = 1 << 6,  // 0x000040, data[3], Motorola bit 25
        overTemperature         = 1 << 7,  // 0x000080, data[3], Motorola bit 24

        highVoltageFuse         = 1 << 8,  // 0x000100, data[2], Motorola bit 23
        lowVoltageFuse          = 1 << 9,  // 0x000200, data[2], Motorola bit 22
        currentSensorLowSide    = 1 << 10, // 0x000400, data[2], Motorola bit 21
        currentDeviation        = 1 << 11, // 0x000800, data[2], Motorola bit 20
        interLock               = 1 << 12, // 0x001000, data[2], Motorola bit 19
        internalSupply12V       = 1 << 13, // 0x002000, data[2], Motorola bit 18
        internalSupply6V        = 1 << 14, // 0x004000, data[2], Motorola bit 17
        voltageDeviation        = 1 << 15, // 0x008000, data[2], Motorola bit 16

        invalidValue            = 1 << 16, // 0x010000, data[4], Motorola bit 39
        commandMessageLost      = 1 << 17, // 0x020000, data[4], Motorola bit 38
        limitMessageLost        = 1 << 18, // 0x040000, data[4], Motorola bit 37
        crcErrorNVSRAM          = 1 << 22, // 0x400000, data[4], Motorola bit 33
        brokenTemperatureSensor = 1 << 23, // 0x800000, data[4], Motorola bit 32
    };

    // Message id=0x260, BSC6COM
    // The value is composed of 1 byte : data[0]
    enum BSC6_Command {
        enable              = 1 << 0, // 0x01, data[0], Motorola bit 7
        boostMode           = 1 << 1, // 0x02, data[0], Motorola bit 6
        debugMode           = 1 << 7  // 0x80, data[0], Motorola bit 0
    };

    BrusaBSC6();
    void handleTick();
    void handleCanFrame(CAN_FRAME *frame);
    void setup();
    void tearDown();
    DeviceId getId();

    void loadConfiguration();
    void saveConfiguration();

private:
    CanHandler *canHandlerEv;
    uint32_t bitfield; // various bit fields
    uint8_t mode; // operation mode / status
    uint8_t lvCurrentAvailable; // 0-250A in 1A
    uint8_t temperatureBuckBoostSwitch1; // 0 - 180C in 1C
    uint8_t temperatureBuckBoostSwitch2; // 0 - 180C in 1C
    uint8_t temperatureHvTrafostageSwitch1; // 0 - 180C in 1C
    uint8_t temperatureHvTrafostageSwitch2; // 0 - 180C in 1C
    uint8_t temperatureLvTrafostageSwitch1; // 0 - 180C in 1C
    uint8_t temperatureLvTrafostageSwitch2; // 0 - 180C in 1C
    uint8_t temperatureTransformerCoil1; // 0 - 180C in 1C
    uint8_t temperatureTransformerCoil2; // 0 - 180C in 1C
    uint8_t internal12VSupplyVoltage; // 0 - 20V in 0.1V
    uint8_t lsActualVoltage; // 0 - 25V in 0.1V
    uint16_t lsActualCurrent; // -30 - 30A in 0.005A, offset = -30A
    uint16_t lsCommandedCurrent; // -30 - 30A in 0.005A, offset = -30A
    uint8_t internalOperationState;

    CAN_FRAME outputFrame; // the output CAN frame;

    void sendCommand();
    void sendLimits();
    void processValues1(uint8_t data[]);
    void processValues2(uint8_t data[]);
    void processDebug1(uint8_t data[]);
    void processDebug2(uint8_t data[]);
};

#endif /* BRUSABSC6_H_ */
