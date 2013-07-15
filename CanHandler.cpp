/*
 * can_handler.cpp
 *
 * Devices may register to this handler in order to receive CAN frames (publish/subscribe)
 * and they can also use this class to send messages.
 *
 *  Created: 4/21/2013
 *   Author: Michael Neuweiler, Collin Kidder
 */

#include "CanHandler.h"

CanHandler::CanHandler(uint32_t baudRate) {
	init(0, baudRate);
}

// CAN0 is intended to be connected to the EV bus (controller, charger, etc.)
// CAN1 is intended to be connected to the car's high speed bus (the one with the ECU)
CanHandler::CanHandler(uint8_t busNumber, uint32_t baudRate) {
	init(busNumber, baudRate);
}

void CanHandler::logFrame(CANFrame& message) {
	if (Logger::getLogLevel() == Logger::Debug) {
		Logger::debug("CAN: dlc=%X fid=%X id=%X ide=%X rtr=%X data=%X,%X,%X,%X,%X,%X,%X,%X",
				message.dlc, message.fid, message.id, message.ide, message.rtr,
				message.data[0], message.data[1], message.data[2], message.data[3],
				message.data[4], message.data[5], message.data[6], message.data[7]);
	}
}

/*
 * Initialization of the CAN buses
 */
void CanHandler::init(uint8_t busNumber, uint32_t baudRate) {
	IRQn busIrq;

	// assign the correct bus instance to the pointer
	if (busNumber == 1) {
		bus = &CAN2;
		busIrq = CAN1_IRQn;
	} else {
		bus = &CAN;
		busIrq = CAN0_IRQn;
	}

	// Initialize the canbus at the specified baudrate
	bus->init(SystemCoreClock, baudRate);

	// Disable all CAN0 & CAN1 interrupts
	bus->disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);

	bus->reset_all_mailbox();

	//Now, the canbus device has 8 mailboxes which can freely be assigned to either RX or TX.
	//The firmware does a lot of both so 5 boxes are for RX, 3 for TX.
	for (uint8_t count = 0; count < 5; count++) {
		bus->mailbox_init(count);
		bus->mailbox_set_mode(count, CAN_MB_RX_MODE);
		bus->mailbox_set_accept_mask(count, 0x7F0, false); //pay attention to everything but the last hex digit
	}

	//First three mailboxes listen for 0x23x frames, last two listen for 0x65x frames
	bus->mailbox_set_id(0, 0x230, false);
	bus->mailbox_set_id(1, 0x230, false);
	bus->mailbox_set_id(2, 0x230, false);
	bus->mailbox_set_id(3, 0x650, false);
	bus->mailbox_set_id(4, 0x650, false);

	for (uint8_t count = 5; count < 8; count++) {
		bus->mailbox_init(count);
		bus->mailbox_set_mode(count, CAN_MB_TX_MODE);
		bus->mailbox_set_priority(count, 10);
		bus->mailbox_set_accept_mask(count, 0x7FF, false);
	}

	//Enable interrupts for the RX boxes. TX interrupts aren't wired up yet
	bus->enable_interrupt(CAN_IER_MB0 | CAN_IER_MB1 | CAN_IER_MB2 | CAN_IER_MB3 | CAN_IER_MB4);

	NVIC_EnableIRQ(busIrq); //tell the nested interrupt controller to turn on our interrupt

	SerialUSB.print("CAN");
	SerialUSB.print(busNumber);
	SerialUSB.println(" INIT OK");
}

void CanHandler::setFilter(uint8_t mailbox, uint32_t acceptMask, uint32_t id, bool extended) {
}

/*
 * \brief If a message is available, read it and send it back in a generalized CAN frame.
 *
 * \param message	the container to receive the data into
 *
 * \retval 	if a message was available and read
 */
bool CanHandler::readFrame(CANFrame& message) {
	if (bus->rx_avail()) {
		bus->get_rx_buff(&rx_frame);

		message.id = rx_frame.id;
		message.fid = rx_frame.fid;
		message.rtr = rx_frame.rtr;
		message.ide = rx_frame.ide;
		message.dlc = rx_frame.dlc;
		memcpy(message.data, rx_frame.data, 8 * sizeof(uint8_t));

		return true;
	}
	return false;
}

bool CanHandler::readFrame(uint8_t mailbox, CANFrame& frame) {
}

bool CanHandler::sendFrame(uint8_t mailbox, CANFrame& message) {
	bus->mailbox_set_id(mailbox, message.id, false);
	bus->mailbox_set_datalen(mailbox, message.dlc);
	for (uint8_t cnt = 0; cnt < 8; cnt++)
		bus->mailbox_set_databyte(mailbox, cnt, message.data[cnt]);
	bus->global_send_transfer_cmd((0x1u << mailbox));

	return true;
}

bool CanHandler::sendFrame(CANFrame& message) {
	sendFrame(5, message);
}
