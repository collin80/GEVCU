/*
 * can.cpp
 *
 * Wrapper class to access different CAN devices.
 * Currrently the MCP2515 and Arduino Due are supported.
 *
 *  Created: 4/21/2013
 *   Author: Michael Neuweiler, Collin Kidder
 */

#include "can_handler.h"

CANHandler::CANHandler() {
	init();
}

#if defined(__arm__) // Arduino Due specific implementation

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
		CAN.mailbox_set_accept_mask(count, 0x7F0, false);
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

	Serial.println("CAN INIT OK");

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

#elif defined(__AVR__) // Machina specific implementation

Frame rx_frame;

// Create CAN object with pins as defined
MCP2515 CAN(CFG_MACHINA_CAN_PIN_CS, CFG_MACHINA_CAN_PIN_RESET, CFG_MACHINA_CAN_PIN_INT);

/*
 * Initiallize the MCP2515 interrupt handler
 */
void CANInterruptHandler() {
	CAN.intHandler();
}

/*
 * Initialize the MCP2515 (Machina) CAN bus
 */
void CANHandler::init() {
	// Set up SPI Communication
	// dataMode can be SPI_MODE0 or SPI_MODE3 only for MCP2515
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.setDataMode(SPI_MODE0);
	SPI.setBitOrder(MSBFIRST);
	SPI.begin();

	// Initialize MCP2515 CAN controller at the specified speed and clock frequency
	// (Note:  This is the oscillator attached to the MCP2515, not the Arduino oscillator)
	// clock in MHz
	if(CAN.Init(CFG_MACHINA_CAN_SPEED, 16))
	{
		Serial.println("MCP2515 Init OK ...");
	} else {
		Serial.println("MCP2515 Init Failed ...");
	}

	attachInterrupt(6, CANInterruptHandler, FALLING);
	CAN.InitFilters(false);

	//Setup CANBUS comm to allow DMOC command and status messages through
	CAN.SetRXMask(MASK0, 0x7F0, 0);//match all but bottom four bits, use standard frames
	CAN.SetRXFilter(FILTER0, 0x230, 0);//allows 0x230 - 0x23F
	CAN.SetRXFilter(FILTER1, 0x650, 0);//allows 0x650 - 0x65F
}

void CANHandler::setFilter() {

}

bool CANHandler::readFrame(CANFrame& message) {
	bool messageReceived = CAN.GetRXFrame(rx_frame);

	message.id = rx_frame.id;
	message.fid = rx_frame.srr; //TODO: is this correct ?
	message.rtr = rx_frame.rtr;
	message.ide = rx_frame.ide;
	message.dlc = rx_frame.dlc;
	for (int i=0; i < 8; i++)
	message.data[i] = rx_frame.data[i];

	return messageReceived;
}

bool CANHandler::sendFrame(CANFrame& message) {
	rx_frame.id = message.id;
	rx_frame.srr = 0;
	rx_frame.rtr = message.rtr;
	rx_frame.ide = message.ide;
	rx_frame.dlc = message.dlc;
	for (int i=0; i < 8; i++)
	rx_frame.data[i] = message.data[i];

	CAN.EnqueueTX(rx_frame);
	return true;
}

bool CANHandler::sendFrame(int mailbox, CANFrame& message) {
	sendFrame(message);
}

#endif
