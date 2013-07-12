/*
 * can.cpp
 *
 * Wrapper class to access different CAN devices.
 * Currrently the Arduino Due is supported.
 *
 *  Created: 4/21/2013
 *   Author: Michael Neuweiler, Collin Kidder
 */

#include "can_handler.h"

CANHandler::CANHandler() {
	init();
}

RX_CAN_FRAME rx_frame;

/*
 * Initialization of the CAN buses
 */
void CANHandler::init() {
	// Initialize CAN0 and CAN1, baudrate at 500Kb/s
	// CAN0 is intended to be connected to the EV bus (controller, charger, etc.)
	// CAN1 is intended to be connected to the car's high speed bus (the one with the ECU)
	CAN.init(SystemCoreClock, CFG_DUE_CAN0_SPEED);
	CAN2.init(SystemCoreClock, CFG_DUE_CAN1_SPEED);

	// Disable all CAN0 & CAN1 interrupts
	CAN.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);
	CAN2.disable_interrupt(CAN_DISABLE_ALL_INTERRUPT_MASK);

	CAN.reset_all_mailbox();
	CAN2.reset_all_mailbox();

	//Now, each canbus device has 8 mailboxes which can freely be assigned to either RX or TX.
	//The firmware does a lot of both so 5 boxes are for RX, 3 for TX.

	for (uint8_t count = 0; count < 5; count++) {
		CAN.mailbox_init(count);
		CAN.mailbox_set_mode(count, CAN_MB_RX_MODE);
		CAN.mailbox_set_accept_mask(count, 0x7F0, false); //pay attention to everything but the last hex digit
	}
	//First three mailboxes listen for 0x23x frames, last two listen for 0x65x frames
	CAN.mailbox_set_id(0, 0x230, false);
	CAN.mailbox_set_id(1, 0x230, false);
	CAN.mailbox_set_id(2, 0x230, false);
	CAN.mailbox_set_id(3, 0x650, false);
	CAN.mailbox_set_id(4, 0x650, false);

	for (uint8_t count = 5; count < 8; count++) {
		CAN.mailbox_init(count);
		CAN.mailbox_set_mode(count, CAN_MB_TX_MODE);
		CAN.mailbox_set_priority(count, 10);
		CAN.mailbox_set_accept_mask(count, 0x7FF, false);
	}

	//Enable interrupts for the RX boxes. TX interrupts aren't wired up yet
	CAN.enable_interrupt(CAN_IER_MB0 | CAN_IER_MB1 | CAN_IER_MB2 | CAN_IER_MB3 | CAN_IER_MB4);

	SerialUSB.println("CAN INIT OK");

	NVIC_EnableIRQ(CAN0_IRQn); //tell the nested interrupt controller to turn on our interrupt
}

/*
 * If a message is available, read it and send it back in a generalized CAN frame.
 *
 * returns if a message was available and read
 */
bool CANHandler::readFrame(CANFrame& message) {
	if (CAN.rx_avail()) {
		CAN.get_rx_buff(&rx_frame);

		message.id = rx_frame.id;
		message.fid = rx_frame.fid;
		message.rtr = rx_frame.rtr;
		message.ide = rx_frame.ide;
		message.dlc = rx_frame.dlc;
		for (int i = 0; i < 8; i++)
			message.data[i] = rx_frame.data[i];

		return true;
	}
	return false;
}

bool CANHandler::sendFrame(CANFrame& message) {
	sendFrame(0, message);
}

bool CANHandler::sendFrame(int mailbox, CANFrame& message) {
	CAN.mailbox_set_id(mailbox, message.id, false);
	CAN.mailbox_set_datalen(mailbox, message.dlc);
	for (uint8_t cnt = 0; cnt < 8; cnt++)
		CAN.mailbox_set_databyte(mailbox, cnt, message.data[cnt]);
	CAN.global_send_transfer_cmd((0x1u << mailbox));

	return true;
}
