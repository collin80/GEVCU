/*
 * can.h
 *
 *  Created: 4/21/2013
 *   Author: Michael Neuweiler
 */

#ifndef CAN_HANDLER_H_
#define CAN_HANDLER_H_

#include <Arduino.h>
#include "config.h"
#include "due_can.h"
#include <due_wire.h>
#include "variant.h"
#include <DueTimer.h>
#include "sys_io.h"

typedef struct
{
	uint32_t id;		// EID if ide set, SID otherwise
	uint32_t fid;		// family ID (RX only)
	uint8_t rtr;		// Remote Transmission Request
	uint8_t ide;		// Extended ID flag
	uint8_t dlc;		// Number of data bytes
	uint8_t data[8];	// Data bytes
} CANFrame;

class CANHandler {

protected:
	void init();
public:
	CANHandler();
	void setFilter();
	bool readFrame(CANFrame&);
	bool sendFrame(CANFrame&);
	bool sendFrame(int mailbox, CANFrame&);
};

#endif /* CAN_HANDLER_H_ */
