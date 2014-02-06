/*
 * CanHandler.h

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

#ifndef CAN_HANDLER_H_
#define CAN_HANDLER_H_

#include <Arduino.h>
#include "config.h"
#include "due_can.h"
#include <due_wire.h>
#include "variant.h"
#include <DueTimer.h>
#include "Logger.h"
#include "DeviceManager.h"

class Device;

class CanObserver {
public:
	virtual void handleCanFrame(CAN_FRAME *frame);
};

class CanHandler {
public:
	enum CanBusNode {
		CAN_BUS_EV, // CAN0 is intended to be connected to the EV bus (controller, charger, etc.)
		CAN_BUS_CAR // CAN1 is intended to be connected to the car's high speed bus (the one with the ECU)
	};

	void initialize();
	void attach(CanObserver *observer, uint32_t id, uint32_t mask, bool extended);
	void detach(CanObserver *observer, uint32_t id, uint32_t mask);
	void process();
	void sendFrame(CAN_FRAME& frame);
	static CanHandler *getInstanceCar();
	static CanHandler *getInstanceEV();
protected:

private:
	struct CanObserverData {
		uint32_t id;	// what id to listen to
		uint32_t mask;	// the CAN frame mask to listen to
		bool extended;	// are extended frames expected
		uint8_t mailbox;	// which mailbox is this observer assigned to
		CanObserver *observer;	// the observer object (e.g. a device)
	};
	static CanHandler *canHandlerEV;	// singleton reference to the EV instance (CAN0)
	static CanHandler *canHandlerCar;	// singleton reference to the car instance (CAN1)

	CanBusNode canBusNode;	// indicator to which can bus this instance is assigned to
	CANRaw *bus;	// the can bus instance which this CanHandler instance is assigned to
	CanObserverData observerData[CFG_CAN_NUM_OBSERVERS];	// Can observers

	CanHandler(CanBusNode busNumber);
	void logFrame(CAN_FRAME& frame);
	int8_t findFreeObserverData();
	int8_t findFreeMailbox();
};

#endif /* CAN_HANDLER_H_ */
