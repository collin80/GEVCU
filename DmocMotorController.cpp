/*
 * DmocMotorController.cpp
 *
 * Interface to the DMOC - Handles sending of commands and reception of status frames
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

/*
 Notes on things to add very soon:
 The DMOC code should be a finite state machine which tracks what state the controller is in as opposed to the desired state and properly
 transitions states. For instance, if we're in disabled mode and we want to get to enabled we've got to first set standby and wait
 for the controller to signal that it has gotten there and then switch to enabled and wait until the controller has gotten there
 then we can apply torque commands.

 Also, the code should take into consideration the RPM for regen purposes. Of course, the controller probably already does that.

 Standby torque needs to be available for some vehicles when the vehicle is placed in enabled and forward or reverse.

 Should probably add EEPROM config options to support max output power and max regen power (in watts). The dmoc supports it
 and I'll bet  other controllers do as well. The rest can feel free to ignore it.
 */

#include "DmocMotorController.h"

DmocMotorController::DmocMotorController() : MotorController()
{
    prefsHandler = new PrefHandler(DMOC645);

    step = SPEED_TORQUE;
    alive = 0;
    commonName = "DMOC645 Inverter";
}

void DmocMotorController::setup()
{
    MotorController::setup(); // run the parent class version of this function

    // register ourselves as observer of 0x23x and 0x65x can frames
    canHandlerEv.attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
    canHandlerEv.attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);

    tickHandler.attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC);
}

/**
 * Tear down the controller in a safe way.
 */
void DmocMotorController::tearDown()
{
    MotorController::tearDown();

    canHandlerEv.detach(this, CAN_MASKED_ID_1, CAN_MASK_1);
    canHandlerEv.detach(this, CAN_MASKED_ID_2, CAN_MASK_2);

    sendCmd1();
    sendCmd2();
    sendCmd3();
}

/**
 * act on messages the super-class does not react upon, like state change
 * to ready or running which should enable/disable the power-stage of the controller
 */
void DmocMotorController::handleStateChange(Status::SystemState oldState, Status::SystemState newState)
{
    MotorController::handleStateChange(oldState, newState);

    // for safety reasons at power off first request 0 torque - this allows the controller to dissipate residual fields first
    if (!powerOn) {
        sendCmd1();
        sendCmd2();
        sendCmd3();
    }
}

/*
 Finally, the firmware actually processes some of the status messages from the DmocMotorController
 However, currently the alive and checksum bytes aren't checked for validity.
 To be valid a frame has to have a different alive value than the last value we saw
 and also the checksum must match the one we calculate. Right now we'll just assume
 everything has gone according to plan.
 */

void DmocMotorController::handleCanFrame(CAN_FRAME *frame)
{
    int invTemp, rotorTemp, statorTemp, actualState;

    switch (frame->id) {
    case CAN_ID_TEMPERATURE:
        rotorTemp = frame->data.bytes[0];
        invTemp = frame->data.bytes[1];
        statorTemp = frame->data.bytes[2];
        temperatureController = (invTemp - 40) * 10;
        temperatureMotor = (max(rotorTemp, statorTemp) - 40) * 10;
        reportActivity();
        break;

    case CAN_ID_TORQUE:
        torqueActual = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]) - 30000;
        reportActivity();
        break;

    case CAN_ID_STATUS:
        speedActual = abs(((frame->data.bytes[0] * 256) + frame->data.bytes[1]) - 20000);

        // 0: Initializing, 1: disabled, 2: ready (standby), 3: enabled, 4: Power Down
        // 5: Fault, 6: Critical Fault, 7: LOS
        actualState = frame->data.bytes[6] >> 4;
        if (actualState == 2 || actualState == 3) {
            ready = true;
            running = (actualState == 3 ? true : false);
        } else {
            if (ready || running) {
                switch (actualState) {
                case 5:
                    Logger::error(this, "Inverter reports fault");
                    break;
                case 6:
                    Logger::error(this, "Inverter reports critical fault");
                    break;
                case 7:
                    Logger::error(this, "Inverter reports LOS");
                    break;
                }
            }
            ready = false;
            running = false;
        }
        reportActivity();
        break;

    //case 0x23E: //electrical status
        //gives volts and amps for D and Q but does the firmware really care?
        //break;

    case CAN_ID_HV_STATUS:
        dcVoltage = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]);
        dcCurrent = ((frame->data.bytes[2] * 256) + frame->data.bytes[3]) - 5000; //offset is 500A, unit = .1A
        reportActivity();
        break;
    }
}

/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
 rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
 */
void DmocMotorController::handleTick()
{
    DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *) getConfiguration();
    MotorController::handleTick();

    step = CHAL_RESP;
    sendCmd1();
    sendCmd2();
    sendCmd3();
    //sendCmd4();
    //sendCmd5();
}

//Commanded RPM plus state of key and gear selector
void DmocMotorController::sendCmd1()
{
    DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *) getConfiguration();
    OperationState newstate;
    CAN_FRAME output;

    canHandlerEv.prepareOutputFrame(&output, CAN_ID_COMMAND);
    alive = (alive + 2) & 0x0F;

    uint16_t speedCommand = 20000;
    if (getSpeedRequested() != 0 && powerOn && running && getGear() != NEUTRAL && config->powerMode == modeSpeed) {
        speedCommand += getSpeedRequested();
    }

    output.data.bytes[0] = (speedCommand & 0xFF00) >> 8;
    output.data.bytes[1] = (speedCommand & 0x00FF);
    output.data.bytes[5] = ON; //key state

    //handle proper state transitions
    newstate = DISABLE;
    if (!ready && powerOn) {
        newstate = STANDBY;
    }
    if ((ready || running) && powerOn) {
        newstate = ENABLE;
    }
    if (!powerOn) {
        newstate = POWERDOWN;
    }

    Gears gear = getGear();
    if (running) {
       if(config->invertDirection) {
           gear = (gear == DRIVE ? REVERSE : DRIVE);
       }
    } else { //force neutral gear until the system is enabled.
        gear = NEUTRAL;
    }

    output.data.bytes[6] = alive + ((byte) gear << 4) + ((byte) newstate << 6);

    output.data.bytes[7] = calcChecksum(output);

    canHandlerEv.sendFrame(output);
}

//Torque limits
void DmocMotorController::sendCmd2()
{
    DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *) getConfiguration();
    CAN_FRAME output;

    canHandlerEv.prepareOutputFrame(&output, CAN_ID_LIMIT);

    if (config->powerMode == modeTorque) {
        //30000 is the base point where torque = 0
        //torqueRequested and torqueMax is in tenths Nm like it should be.
        uint16_t torqueCommand = 3000; //set offset  for zero torque commanded
        if (running) { //don't even try sending torque commands until the DMOC reports it is ready
            int16_t torqueRequested = (speedActual < config->speedMax ? getTorqueRequested() : getTorqueRequested() / 1.3);

            if (config->invertDirection ^ getGear() == REVERSE) {
                torqueRequested *= -1;
            }
            torqueCommand += torqueRequested;
        }

        //data 0-1 is upper limit, 2-3 is lower limit. They are set to same value to lock torque to this value
        output.data.bytes[0] = (torqueCommand & 0xFF00) >> 8;
        output.data.bytes[1] = (torqueCommand & 0x00FF);
        output.data.bytes[2] = output.data.bytes[0];
        output.data.bytes[3] = output.data.bytes[1];
    } else { //RPM mode so request max torque as upper limit and zero torque as lower limit
        output.data.bytes[0] = ((30000L + config->torqueMax) & 0xFF00) >> 8;
        output.data.bytes[1] = ((30000L + config->torqueMax) & 0x00FF);
        output.data.bytes[2] = 0x75;
        output.data.bytes[3] = 0x30;
    }

    //what the hell is standby torque? Does it keep the transmission spinning for automatics? I don't know.
    output.data.bytes[4] = 0x75; //msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
    output.data.bytes[5] = 0x30; //lsb
    output.data.bytes[6] = alive;
    output.data.bytes[7] = calcChecksum(output);

    canHandlerEv.sendFrame(output);
}

//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DmocMotorController::sendCmd3()
{
    DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *) getConfiguration();
    CAN_FRAME output;
    canHandlerEv.prepareOutputFrame(&output, CAN_ID_LIMIT2);

    int regenCalc = 65000 - (config->maxMechanicalPowerRegen * 25);
    int accelCalc = (config->maxMechanicalPowerMotor * 25);
    output.data.bytes[0] = ((regenCalc & 0xFF00) >> 8);  //msb of regen watt limit
    output.data.bytes[1] = (regenCalc & 0xFF); //lsb
    output.data.bytes[2] = ((accelCalc & 0xFF00) >> 8);  //msb of acceleration limit
    output.data.bytes[3] = (accelCalc & 0xFF); //lsb
    output.data.bytes[4] = 0; //not used
    output.data.bytes[5] = 60; //20 degrees celsius
    output.data.bytes[6] = alive;
    output.data.bytes[7] = calcChecksum(output);

    canHandlerEv.sendFrame(output);
}

//challenge/response frame 1 - Really doesn't contain anything we need I dont think
void DmocMotorController::sendCmd4()
{
    CAN_FRAME output;
    canHandlerEv.prepareOutputFrame(&output, CAN_ID_CHALLENGE);
    output.data.bytes[0] = 37; //i don't know what all these values are
    output.data.bytes[1] = 11; //they're just copied from real traffic
    output.data.bytes[4] = 6;
    output.data.bytes[5] = 1;
    output.data.bytes[6] = alive;
    output.data.bytes[7] = calcChecksum(output);

    canHandlerEv.sendFrame(output);
}

//Another C/R frame but this one also specifies which shifter position we're in
void DmocMotorController::sendCmd5()
{
    CAN_FRAME output;
    canHandlerEv.prepareOutputFrame(&output, CAN_ID_CHALLENGE2);
    output.data.bytes[0] = 2;
    output.data.bytes[1] = 127;

    if (powerOn && getGear() != NEUTRAL) {
        output.data.bytes[3] = 52;
        output.data.bytes[4] = 26;
        output.data.bytes[5] = 59; //drive
    } else {
        output.data.bytes[3] = 39;
        output.data.bytes[4] = 19;
        output.data.bytes[5] = 55; //neutral
    }

    //--PRND12
    output.data.bytes[6] = alive;
    output.data.bytes[7] = calcChecksum(output);

    canHandlerEv.sendFrame(output);
}

//this might look stupid. You might not believe this is real. It is. This is how you
//calculate the checksum for the DMOC frames.
byte DmocMotorController::calcChecksum(CAN_FRAME thisFrame)
{
    byte cs;
    byte i;
    cs = thisFrame.id;

    for (i = 0; i < 7; i++) {
        cs += thisFrame.data.bytes[i];
    }

    i = cs + 3;
    cs = ((int) 256 - i);
    return cs;
}

DeviceId DmocMotorController::getId()
{
    return (DMOC645);
}

void DmocMotorController::loadConfiguration()
{
    DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *) getConfiguration();

    if (!config) {
        config = new DmocMotorControllerConfiguration();
        setConfiguration(config);
    }

    MotorController::loadConfiguration(); // call parent
}

void DmocMotorController::saveConfiguration()
{
    MotorController::saveConfiguration();
}
