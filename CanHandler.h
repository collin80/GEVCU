/*
 * CanHandler.h
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
