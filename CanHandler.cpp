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

CanHandler *CanHandler::canHandlerEV = NULL;
CanHandler *CanHandler::canHandlerCar = NULL;

CanHandler::CanHandler(CanBusNode canBusNode) {
	this->canBusNode = canBusNode;

	// assign the correct bus instance to the pointer
	if (canBusNode == CAN_BUS_CAR)
		bus = &CAN2;
	else
		bus = &CAN;
}

CanHandler* CanHandler::getInstanceEV()
{
	if (canHandlerEV == NULL)
		canHandlerEV = new CanHandler(CAN_BUS_EV);
	return canHandlerEV;
}

CanHandler* CanHandler::getInstanceCar()
{
	if (canHandlerCar == NULL)
		canHandlerCar = new CanHandler(CAN_BUS_CAR);
	return canHandlerCar;
}

/*
 * Initialization of the CAN bus
 */
void CanHandler::initialize() {
	// Initialize the canbus at the specified baudrate
	bus->init(SystemCoreClock, (canBusNode == CAN_BUS_EV ? CFG_CAN0_SPEED : CFG_CAN1_SPEED));

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

	NVIC_EnableIRQ(canBusNode == CAN_BUS_EV ? CAN0_IRQn : CAN1_IRQn); //tell the nested interrupt controller to turn on our interrupt

	Logger::info("CAN%d init ok", (canBusNode == CAN_BUS_EV ? 0 : 1));
}

void CanHandler::logFrame(RX_CAN_FRAME& frame) {
	if (Logger::getLogLevel() == Logger::Debug) {
		Logger::debug("CAN: dlc=%X fid=%X id=%X ide=%X rtr=%X data=%X,%X,%X,%X,%X,%X,%X,%X",
				frame.dlc, frame.fid, frame.id, frame.ide, frame.rtr,
				frame.data[0], frame.data[1], frame.data[2], frame.data[3],
				frame.data[4], frame.data[5], frame.data[6], frame.data[7]);
	}
}

void CanHandler::setFilter(uint8_t mailbox, uint32_t acceptMask, uint32_t id, bool extended) {
}

/*
 * \brief If a message is available, read it and forward it to registered devices.
 *
 * \param frame	the container to receive the data into
 */
void CanHandler::processInput() {
	static RX_CAN_FRAME frame;

	if (bus->rx_avail()) {
		bus->get_rx_buff(&frame);
//		logFrame(frame);

		//TODO: properly call all devices which are registered
		DeviceManager::getInstance()->getMotorController()->handleCanFrame(frame);
	}
}

bool CanHandler::sendFrame(uint8_t mailbox, TX_CAN_FRAME& frame) {
	bus->mailbox_set_id(mailbox, frame.id, false);
	bus->mailbox_set_datalen(mailbox, frame.dlc);
	for (uint8_t cnt = 0; cnt < 8; cnt++)
		bus->mailbox_set_databyte(mailbox, cnt, frame.data[cnt]);
	bus->global_send_transfer_cmd((0x1u << mailbox));

	return true;
}

bool CanHandler::sendFrame(TX_CAN_FRAME& frame) {
	sendFrame(5, frame);
}
