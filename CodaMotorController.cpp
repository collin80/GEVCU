/*
 * CodaMotorController.cpp
 *
 * CAN Interface to the Coda flavored UQM Powerphase 100 inverter - 
 Handles sending of commands and reception of status frames to drive
 the inverter and thus motor.  Endianess is configurable in the firmware
 inside the UQM inverter but default is little endian.  This object module * uses little endian format
 - the least significant byte is the first in order with the MSB following.
 **************NOTE***************
 Ticks are critical for the UQM inverter.  A tick value of 10000 in config.h is necessary as the inverter
 expects a torque command within each 12 millisecond period.  Failing to provide it is a bit subtle to catch
 but quite dramatic.  The motor will run at speed for about 5 to 7 minutes and then "cough" losing all torque and
 then recovering.  Five minutes later, this will repeat.  Setting to a very fast value of 10000 seems to cure it NOW.
 As the software grows and the load on the CPU increases, this could show up again.   
 *
 Copyright (c) 2014 Jack Rickard

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

#include "CodaMotorController.h"

//template<class T> inline Print &operator <<(Print &obj, T arg) {
//    obj.print(arg);
//    return obj;
//}

const uint8_t swizzleTable[] = { 0xAA, 0x7F, 0xFE, 0x29, 0x52, 0xA4, 0x9D, 0xEF, 0xB, 0x16, 0x2C, 0x58, 0xB0, 0x60, 0xC0, 1 };

CodaMotorController::CodaMotorController() : MotorController() {
    prefsHandler = new PrefHandler(CODAUQM);
    sequence = 0;
    commonName = "Coda UQM Powerphase 100 Inverter";
}

void CodaMotorController::setup() {
    tickHandler->detach(this);

    loadConfiguration();
    MotorController::setup(); // run the parent class version of this function

    // register ourselves as observer of all 0x20x can frames for UQM
    canHandlerEv->attach(this, 0x200, 0x7f0, false);

    tickHandler->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_CODAUQM);
}

void CodaMotorController::handleCanFrame(CAN_FRAME *frame) {
    int invTemp, rotorTemp, statorTemp;

    //TODO: running should only be set to true if the controller reports that its power-stageis up and running.
    running = true;

    if (Logger::isDebug()) {
        Logger::debug(CODAUQM, "msg: %X   %X   %X   %X   %X   %X   %X   %X  %X", frame->id, frame->data.bytes[0], frame->data.bytes[1],
            frame->data.bytes[2], frame->data.bytes[3], frame->data.bytes[4], frame->data.bytes[5], frame->data.bytes[6], frame->data.bytes[7]);
    }

    switch (frame->id) {

    case 0x209:  //Accurate Feedback Message
        torqueActual = ((((frame->data.bytes[1] * 256) + frame->data.bytes[0]) - 32128));
        dcVoltage = (((frame->data.bytes[3] * 256) + frame->data.bytes[2]) - 32128);
        dcCurrent = (((frame->data.bytes[5] * 256) + frame->data.bytes[4]) - 32128);
        speedActual = abs((((frame->data.bytes[7] * 256) + frame->data.bytes[6]) - 32128) / 2);
        if (Logger::isDebug()) {
            Logger::debug(CODAUQM, "Actual Torque: %d DC Voltage: %d Amps: %d RPM: %d", torqueActual / 10, dcVoltage / 10, dcCurrent / 10, speedActual);
        }
        reportActivity();
        break;

    case 0x20A:    //System Status Message
        Logger::debug(CODAUQM, "20A System Status Message Received");
        reportActivity();
        break;

    case 0x20B:    //Emergency Fuel Cutback Message
        Logger::debug(CODAUQM, "20B Emergency Fuel Cutback Message Received");
        reportActivity();
        break;

    case 0x20C:    //Reserved Message
        Logger::debug(CODAUQM, "20C Reserved Message Received");
        reportActivity();
        break;

    case 0x20D:    //Limited Torque Percentage Message
        Logger::debug(CODAUQM, "20D Limited Torque Percentage Message Received");
        reportActivity();
        break;

    case 0x20E:     //Temperature Feedback Message
        invTemp = frame->data.bytes[2];
        rotorTemp = frame->data.bytes[3];
        statorTemp = frame->data.bytes[4];
        temperatureController = (invTemp - 40) * 10;
        temperatureMotor = (max(rotorTemp, statorTemp) - 40) * 10;
        if (Logger::isDebug()) {
            Logger::debug(CODAUQM, "Inverter temp: %d Motor temp: %d", temperatureController, temperatureMotor);
        }
        reportActivity();
        break;

    case 0x20F:    //CAN Watchdog Status Message
        Logger::debug(CODAUQM, "20F CAN Watchdog status error");
        status->warning = true;
        running = false;
        sendCmd2(); //If we get a Watchdog status, we need to respond with Watchdog reset
        reportActivity();
        break;
    }
}

void CodaMotorController::handleTick() {
    MotorController::handleTick(); //kick the ball up to papa
    sendCmd1();   //Send our lone torque command
}

/*
 UQM only HAS a single command CAN bus frame - address 0x204  Everything is stuffed into this one frame. It has a 5 byte payload.

 Byte 1 - always set to 0

 Byte 2 - Command Byte
 Left four bits contain enable disable and forward reverse
 Bits 7/6 DISABLED =01
 Bits 7/6 ENABLE =10
 Bits 5/4 REVERSE=01
 Bits 5/4 FORWARD=10
 Example:  Enabled and Forward: 1010

 Right four bits (3 to 0) is a sequence counter that counts 0000 to 0111 and back to 0000

 Byte 3 LSB of Torque command value.

 Byte 4 MSB of Torque command value   Offset is 32128

 Byte 5 Security CRC byte.

 Bytes must be sent IN SEQUENCE and the CRC byte must be appropriate for bytes 2, 3, and 4.
 Values above 32128 are positive torque.  Values below 32128 are negative torque
 */

void CodaMotorController::sendCmd1() {
    CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *) getConfiguration();

    CAN_FRAME output;
    canHandlerEv->prepareOutputFrame(&output, 0x204);

    if ((ready || running) && powerOn) {
        output.data.bytes[1] = 0x80; //1000 0000
    } else {
        output.data.bytes[1] = 0x40; //0100 0000
    }

    if (getGear() == DRIVE && !config->invertDirection) {
        output.data.bytes[1] |= 0x20; //xx10 0000
    } else {
        output.data.bytes[1] |= 0x10; //xx01 0000
    }

    sequence += 1; //Increment sequence
    if (sequence == 8) {
        sequence = 0;
    } //If we reach 8, go to zero
    output.data.bytes[1] |= sequence; //This should retain left four and add sequence count to right four bits.
    //Requested throttle is [-1000, 1000]
    //Two byte torque request in 0.1NM Can be positive or negative

    uint16_t torqueCommand = 32128; //Set our zero offset value -torque=0
    if (speedActual < config->speedMax) {
        torqueCommand += getTorqueRequested();
    } //If actual rpm less than max rpm, add torque command to offset
    else {
        torqueCommand += getTorqueRequested() / 2;
    }  //If at RPM limit, cut torque command in half.

    output.data.bytes[3] = (torqueCommand & 0xFF00) >> 8;  //Stow torque command in bytes 2 and 3.
    output.data.bytes[2] = (torqueCommand & 0x00FF);
    output.data.bytes[4] = genCodaCRC(output.data.bytes[1], output.data.bytes[2], output.data.bytes[3]); //Calculate security byte

    canHandlerEv->sendFrame(output);  //Mail it.

    if (Logger::isDebug()) {
        Logger::debug(CODAUQM, "Torque command: %X   %X  ControlByte: %X  LSB %X  MSB: %X  CRC: %X", output.id, output.data.bytes[0],
            output.data.bytes[1], output.data.bytes[2], output.data.bytes[3], output.data.bytes[4]);
    }

}

void CodaMotorController::sendCmd2() {
    CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *) getConfiguration();

    /*In the CODA CAN bus capture logs, this command, defined in the UQM manual as a
     207 watchdog reset, is sent every 480msec.  It also always occurs after the last count of
     a sequence ie 57, 67, 97, or A7.  But this may just be coincidence.
     By the book, this is to be sent in response to a watchdog timeout.
     We send this in response to receipt of a 20F Watchdog status.
     */

    CAN_FRAME output;
    canHandlerEv->prepareOutputFrame(&output, 0x207);
    output.data.bytes[0] = 0xa5; //This is simply three given values.  The 5A appears to be
    output.data.bytes[1] = 0xa5; //the important one.
    output.data.bytes[2] = 0x5a;

    canHandlerEv->sendFrame(output);
    if (Logger::isDebug()) {
        Logger::debug(CODAUQM, "Watchdog reset: %X  %X  %X", output.data.bytes[0], output.data.bytes[1], output.data.bytes[2]);
    }

    status->warning = false;
}

DeviceId CodaMotorController::getId() {
    return (CODAUQM);
}

void CodaMotorController::loadConfiguration() {
    CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *) getConfiguration();

    if (!config) {
        config = new CodaMotorControllerConfiguration();
        setConfiguration(config);
    }

    MotorController::loadConfiguration(); // call parent
}

void CodaMotorController::saveConfiguration() {
    MotorController::saveConfiguration();
}

uint8_t CodaMotorController::genCodaCRC(uint8_t cmd, uint8_t torq_lsb, uint8_t torq_msb) {
    int counter;
    uint8_t crc;
    uint16_t temp_torq = torq_lsb + (256 * torq_msb);
    crc = 0x7F; //7F is the answer if bytes 3 and 4 are zero. We build up from there.

    //this can be done a little more efficiently but this is clearer to read
    if (((cmd & 0xA0) == 0xA0) || ((cmd & 0x60) == 0x60))
        temp_torq += 1;

    //Not sure why this happens except to obfuscate the result
    if ((temp_torq % 4) == 3)
        temp_torq += 4;

    //increment over the bits within the torque command
    //and applies a particular XOR for each set bit.
    for (counter = 0; counter < 16; counter++) {
        if ((temp_torq & (1 << counter)) == (1 << counter))
            crc = (byte) (crc ^ swizzleTable[counter]);
    }
    return (crc);
}
