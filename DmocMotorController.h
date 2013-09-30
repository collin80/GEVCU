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
#include "sys_io.h"
#include "TickHandler.h"
#include "CanHandler.h"

class DmocMotorController: public MotorController, CanObserver {
public:
	enum Gears {
		NEUTRAL = 0,
		DRIVE = 1,
		REVERSE = 2,
		ERROR = 3
	};

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

	enum OperationState {
		DISABLED = 0,
		STANDBY = 1,
		ENABLE = 2,
		POWERDOWN = 3
	};

public:
	void handleTick();
	void handleCanFrame(RX_CAN_FRAME *frame);
	void setup();
	void setOpState(OperationState op);
	void setGear(Gears gear);

	DmocMotorController();
	DeviceId getId();

	uint32_t getTickInterval();

private:
	Gears selectedGear;
	Gears actualGear;
	OperationState operationState; //the op state we want
	OperationState actualState; //what the controller is reporting it is
	int step;
	byte online; //counter for whether DMOC appears to be operating
	byte alive;

	void sendCmd1();
	void sendCmd2();
	void sendCmd3();
	void sendCmd4();
	void sendCmd5();
	byte calcChecksum(TX_CAN_FRAME thisFrame);

};

#endif /* DMOC_H_ */

