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

class CanHandler {
public:
	enum CanBusNode {
		CAN_BUS_EV, // CAN0 is intended to be connected to the EV bus (controller, charger, etc.)
		CAN_BUS_CAR // CAN1 is intended to be connected to the car's high speed bus (the one with the ECU)
	};

	void initialize();
	void setFilter(uint8_t mailbox, uint32_t acceptMask, uint32_t id, bool extended);
	void processInput();
	bool sendFrame(TX_CAN_FRAME& frame);
	bool sendFrame(uint8_t mailbox, TX_CAN_FRAME& frame);
	static CanHandler *getInstanceCar();
	static CanHandler *getInstanceEV();

protected:

private:
	struct MailboxEntry {
		uint32_t id;
		uint32_t mask;
		Device *device[CFG_CAN_MAX_DEVICES_PER_MAILBOX]; // array of pointers to devices with this id/mask set
	};
	static CanHandler *canHandlerEV;
	static CanHandler *canHandlerCar;

	CanBusNode canBusNode;
	CANRaw *bus;
	MailboxEntry mailboxEntry[CFG_CAN0_NUM_RX_MAILBOXES]; // array of mailbox entries (8 as there are 8 timers)

	CanHandler(CanBusNode busNumber);
	int findMailbox(uint32_t id, uint32_t mask);
	int findDevice(int mailboxNumber, Device *device);
	void logFrame(RX_CAN_FRAME& frame);
};

#endif /* CAN_HANDLER_H_ */
