/*
 * can_handler.h
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
