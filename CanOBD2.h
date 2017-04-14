/*
 * CanOBD2.h
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

#ifndef CAN_OBD2_H_
#define CAN_OBD2_H_

#include <Arduino.h>
#include "config.h"
#include "TickHandler.h"
#include "CanHandler.h"
#include "constants.h"
#include "OBD2Handler.h"
#include "DeviceManager.h"

#define CAN_ID_BROADCAST      0x7df // broadcast address for requests         11111011111
#define CAN_ID_REQUEST        0x7e0 // to 0x7e7, specific request address     11111100000 - 11111100111
#define CAN_MASK_REQUEST      0x7c0 // mask for above id's                    11111000000
#define CAN_MASKED_ID_REQUEST 0x7c0 // masked id for id's 0x7df, 0x7e0-0x7e7  11111000000

#define CAN_ID_RESPONSE             0x7e8 // to 7ef, response to PID request  11111101000 - 11111101111
#define CAN_MASK_POLL_RESPONSE      0x7f0 // mask for above id's              11111110000
#define CAN_MASKED_ID_POLL_RESPONSE 0x7e0 // masked id for id's 0x7e8-0x7ef   11111100000

//                                 PID  LEN DESCRIPTION                MIN  MAX  UNITS  FORMULA
#define PID_SUPPORTED_01_20       0x00 // 4 PIDs supported [01 - 20]                    Bit encoded [A7..D0] == [PID $01..PID $20
#define PID_SUPPORTED_21_40       0x20 // 4 PIDs supported [21 - 40]                    Bit encoded [A7..D0] == [PID $21..PID $40]
#define PID_SUPPORTED_41_60       0x40 // 4 PIDs supported [41 - 60]                    Bit encoded [A7..D0] == [PID $41..PID $60]
#define PID_SUPPORTED_61_80       0x60 // 4 PIDs supported [61 - 80]                    Bit encoded [A7..D0] == [PID $61..PID $80]

#define PID_VEHICLE_SPEED         0x0D // 1 Vehicle speed                0  255   km/h  A
#define PID_INTAKE_AIR_TEMP       0x0F // 1 Intake air temperature     -40  215     °C  A − 40
#define PID_THROTTLE_POS          0x11 // 1 Throttle position            0  100      %  100/255*A
#define PID_BAROMETRIC_PRESSURE   0x33 // 1 Absolute Barometric Pressure 0  255    kPa  A
#define PID_THROTTLE_POS_RELATIVE 0x45 // 1 Relative throttle position   0  100      %  100/255*A
#define PID_AMBIENT_TEMP          0x46 // 1 Ambient air temperature    -40  215     °C  A − 40
#define PID_THROTTLE_POS_B        0x47 // 1 Absolute throttle position B 0  100      %  100/255*A
#define PID_THROTTLE_POS_C        0x48 // 1 Absolute throttle position C
#define PID_THROTTLE_POS_D        0x49 // 1 Accelerator pedal position D
#define PID_THROTTLE_POS_E        0x4A // 1 Accelerator pedal position E
#define PID_THROTTLE_POS_F        0x4B // 1 Accelerator pedal position F
#define PID_THROTTLE_COMMANDED    0x4C // 1 Commanded throttle actuator
#define PID_TORQUE_DEMAND         0x61 // 1 Demand engine torque %    -125  125      %  A-125
#define PID_TORQUE_ACTUAL         0x62 // 1 Actual engine torque %    -125  125      %  A-125
#define PID_TORQUE_REFERENCE      0x63 // 2 Engine reference torque      0  65,535  Nm  256*A + B
#define PID_COOLANT_TEMP          0x67 // 3 Engine coolant temperature
#define PID_INTAKE_AIR_TEMP2      0x68 // 7 Intake air temperature sensor
#define PID_THROTTLE_COMMANDED2   0x6C // 5 Commanded throttle actuator control and relative throttle position

class CanOBD2Configuration : public DeviceConfiguration
{
public:
    uint8_t canBusRespond; // whch can bus should we respond to OBD2 requests
    uint8_t canIdOffsetRespond; // offset for can id on wich we listen to incoming requests (0-7)

    uint8_t canBusPoll; // whch can bus should we poll OBD2 data from
    uint8_t canIdOffsetPoll; // offset for can id on which we will request OBD2 data from (0-7, 255=broadcast)
};

class CanOBD2: public Device, CanObserver
{
public:
    CanOBD2();
    void setup();
    void tearDown();
    void handleTick();
    void handleCanFrame(CAN_FRAME *frame);
    DeviceId getId();

    void loadConfiguration();
    void saveConfiguration();

protected:

private:
    CanHandler *canHandlerRespond;
    CanHandler *canHandlerPoll;
    CAN_FRAME pollFrame;

    uint8_t arrayPos;
    bool lastRequestAnswered;

    void processRequest(CAN_FRAME* frame);
    void processResponse(CAN_FRAME* frame);
};

#endif //CAN_OBD2_H_
