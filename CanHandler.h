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

typedef struct
{
	uint32_t id;		// EID if ide set, SID otherwise
	uint32_t fid;		// family ID (RX only)
	uint8_t rtr;		// Remote Transmission Request
	uint8_t ide;		// Extended ID flag
	uint8_t dlc;		// Number of data bytes
	uint8_t data[8];	// Data bytes
} CANFrame;

class Device;

class CanHandler {
public:
	CanHandler(uint32_t baudRate);
	CanHandler(uint8_t busNumber, uint32_t baudRate);
	void setFilter(uint8_t mailbox, uint32_t acceptMask, uint32_t id, bool extended);
	bool readFrame(CANFrame& frame);
	bool readFrame(uint8_t mailbox, CANFrame& frame);
	bool sendFrame(CANFrame& frame);
	bool sendFrame(uint8_t mailbox, CANFrame& frame);

protected:
	void init(uint8_t busNumber, uint32_t baudRate);

private:
	struct MailboxEntry {
		uint32_t id;
		uint32_t mask;
		Device *device[CFG_CAN_MAX_DEVICES_PER_MAILBOX]; // array of pointers to devices with this id/mask set
	};
	RX_CAN_FRAME rx_frame;
	CANRaw *bus;
	static MailboxEntry mailboxEntry[CFG_CAN0_NUM_RX_MAILBOXES]; // array of mailbox entries (8 as there are 8 timers)
	static int findMailbox(uint32_t id, uint32_t mask);
	static int findDevice(int mailboxNumber, Device *device);
	void logFrame(CANFrame& frame);
};

#endif /* CAN_HANDLER_H_ */
