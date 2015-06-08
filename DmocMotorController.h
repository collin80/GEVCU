/*
 * DmocMotorController.h
 *
 * Note that the dmoc needs to have some form of input for gear selector (drive/neutral/reverse)
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

#ifndef DMOC_H_
#define DMOC_H_

#include <Arduino.h>
#include "config.h"
#include "MotorController.h"
#include "SystemIO.h"
#include "TickHandler.h"

// CAN bus id's for frames sent to DMOC

#define CAN_ID_COMMAND      0x232 // send commands (speed, gear)
#define CAN_ID_LIMIT        0x233 // send limitations (torque)
#define CAN_ID_LIMIT2       0x234 // send limitations (power)
#define CAN_ID_CHALLENGE    0x235 // send challenge/response
#define CAN_ID_CHALLENGE2   0x236 // send challenge/response

// CAN bus id's for frames received from DMOC

#define CAN_ID_TORQUE       0x23a // receive actual torque values              01000111010
#define CAN_ID_STATUS       0x23b // receive status and speed information      01000111011
#define CAN_MASK_1          0x7fe // mask for above id's                       11111111110
#define CAN_MASKED_ID_1     0x23a // masked id for id's from 0x26a to 0x26f    01000111010

#define CAN_ID_HV_STATUS    0x650 // receive hv bus status information         11001010000
#define CAN_ID_TEMPERATURE  0x651 // receive actual temperature                11001010001
#define CAN_MASK_2          0x7fe // mask for above id's                       11111111110
#define CAN_MASKED_ID_2     0x650 // masked id for id's from 0x26a to 0x26f    11001010000

/*
 * Class for DMOC specific configuration parameters
 */
class DmocMotorControllerConfiguration : public MotorControllerConfiguration
{
public:
};

class DmocMotorController: public MotorController
{
public:

    enum Step {
        SPEED_TORQUE,
        CHAL_RESP
    };

    enum KeyState {
        OFF = 0,
        ON = 1,
        RESERVED = 2,
        NOACTION = 3
    };

public:
    virtual void handleTick();
    virtual void handleCanFrame(CAN_FRAME *frame);
    void handleStateChange(Status::SystemState state);
    virtual void setup();
    virtual void tearDown();
    void setGear(Gears gear);

    DmocMotorController();
    DeviceId getId();

    virtual void loadConfiguration();
    virtual void saveConfiguration();

private:
    Gears actualGear;
    int step;
    byte online; //counter for whether DMOC appears to be operating
    byte alive;
    int activityCount;

    void sendCmd1();
    void sendCmd2();
    void sendCmd3();
    void sendCmd4();
    void sendCmd5();
    byte calcChecksum(CAN_FRAME thisFrame);

};

#endif /* DMOC_H_ */

