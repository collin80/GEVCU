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

class CanHandler {

private:
	RX_CAN_FRAME rx_frame;
	CANRaw *bus;
	void logFrame(CANFrame&);

protected:
	void init(uint8_t, uint32_t);

public:
	CanHandler(uint32_t);
	CanHandler(uint8_t, uint32_t);
	void setFilter(uint8_t, uint32_t, uint32_t, bool);
	bool readFrame(CANFrame&);
	bool readFrame(uint8_t, CANFrame&);
	bool sendFrame(CANFrame&);
	bool sendFrame(uint8_t, CANFrame&);
};

#endif /* CAN_HANDLER_H_ */
