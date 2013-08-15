/*
 * BrusaMotorController.cpp
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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
#include "BrusaMotorController.h"

/*
Warning:
At high speed disable the DMC_EnableRq can be dangerous because a field weakening current is
needed to achieve zero torque. Switching off the DMC in such a situation will make heavy regenerat-
ing torque that can't be controlled.
*/

BrusaMotorController::BrusaMotorController() {
	operationState = unknown;
	actualState = unknown;
	maxTorque = 500; //in tenths so 50Nm max torque. This is plenty for testing
	maxRPM = 6000; //also plenty for a bench test

	torqueRequested = 0;
	torqueActual = 0;
	speedRequested = 0;
	speedActual = 0;
	voltageMinimal = 0;
	voltageMaximal = 0;
	voltageActual = 0;
	currentMinimal = 0;
	currentMaximal = 0;
	currentActual = 0;
}

void BrusaMotorController::setup() {
	TickHandler::getInstance()->detach(this);
	MotorController::setup(); // run the parent class version of this function

	// register ourselves as observer of 0x258-0x268 and 0x458 can frames
	CanHandler::getInstanceEV()->attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
	CanHandler::getInstanceEV()->attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER);
}

void BrusaMotorController::handleTick() {
	// send CTRL every 20ms : 20 00 2E E0 00 00 00 00
	// send CTRL_2 every 100ms : 00 00 00 00 00 00 00 00
	// send LIMIT every 200ms : 0D 70 11 C6 00 00 00 00
}

void BrusaMotorController::handleCanFrame(RX_CAN_FRAME* frame) {

	//TODO: remove debug log
	Logger::debug("BrusaMotorController CAN: dlc=%X fid=%X id=%X ide=%X rtr=%X data=%X,%X,%X,%X,%X,%X,%X,%X",
			frame->dlc, frame->fid, frame->id, frame->ide, frame->rtr,
			frame->data[0], frame->data[1], frame->data[2], frame->data[3],
			frame->data[4], frame->data[5], frame->data[6], frame->data[7]);

	Logger::debug("binary representation: data=%B,%B,%B,%B,%B,%B,%B,%B",
			frame->data[0], frame->data[1], frame->data[2], frame->data[3],
			frame->data[4], frame->data[5], frame->data[6], frame->data[7]);

	uint64_t value1 = (frame->data[7]<<0) | (frame->data[6]<<8) | (frame->data[5]<<16) | (frame->data[4]<<24) | (frame->data[3]<<32) | (frame->data[2]<<40) | (frame->data[1]<<48) | (frame->data[0]<<56);
	uint64_t value2 = (frame->data[0]<<0) | (frame->data[1]<<8) | (frame->data[2]<<16) | (frame->data[3]<<24) | (frame->data[4]<<32) | (frame->data[5]<<40) | (frame->data[6]<<48) | (frame->data[7]<<56);

	Logger::debug("value1: %B %b, value2: %b %b", value1 >> 32, value1 & 0xFFFFFFFF, value2 >> 32, value2 & 0xFFFFFFFF);

	switch (frame->id) {
	case CAN_ID_STATUS:
		Logger::debug("status message received");
		break;
	case CAN_ID_ACTUAL_VALUES:
		Logger::debug("actual values message received");
		break;
	case CAN_ID_ERRORS:
		Logger::debug("error message received");
		break;
	case CAN_ID_TORQUE_LIMIT:
		Logger::debug("torque limit message received");
		break;
	case CAN_ID_TEMP:
		Logger::debug("temp message received");
		break;
	default:
		Logger::debug("BrusaMotorController: received unknown frame id %d", frame->id);
	}
}

Device::DeviceId BrusaMotorController::getId() {
	return Device::BRUSA_DMC5;
}

/*
void swapByteOrder(unsigned short& us)
{
    us = (us >> 8) |
         (us << 8);
}

void swapByteOrder(unsigned int& ui)
{
    ui = (ui >> 24) |
         ((ui<<8) & 0x00FF0000) |
         ((ui>>8) & 0x0000FF00) |
         (ui << 24);
}

void swapByteOrder(unsigned long long& ull)
{
    ull = (ull >> 56) |
          ((ull<<40) & 0x00FF000000000000) |
          ((ull<<24) & 0x0000FF0000000000) |
          ((ull<<8) & 0x000000FF00000000) |
          ((ull>>8) & 0x00000000FF000000) |
          ((ull>>24) & 0x0000000000FF0000) |
          ((ull>>40) & 0x000000000000FF00) |
          (ull << 56);
}
*/

#endif
