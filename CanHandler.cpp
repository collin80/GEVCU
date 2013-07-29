/*
 * CanHandler.cpp
 *
 * Devices may register to this handler in order to receive CAN frames (publish/subscribe)
 * and they can also use this class to send messages.
 *
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
	for (uint8_t count = 0; count < 7; count++) {
		bus->mailbox_init(count);
		bus->mailbox_set_mode(count, CAN_MB_RX_MODE);
		bus->mailbox_set_accept_mask(count, 0x7F0, false); //pay attention to everything but the last hex digit
	}

	//First three mailboxes listen for 0x23x frames, last two listen for 0x65x frames
	//TODO: These should not be hard coded.
	bus->mailbox_set_id(0, 0x230, false);
	bus->mailbox_set_id(1, 0x230, false);
	bus->mailbox_set_id(2, 0x230, false);
	bus->mailbox_set_id(3, 0x650, false);
	bus->mailbox_set_id(4, 0x650, false);
	bus->mailbox_set_id(5, 0x230, false);
	bus->mailbox_set_id(6, 0x650, false);

	//for (uint8_t count = 5; count < 8; count++) {
		bus->mailbox_init(7);
		bus->mailbox_set_mode(7, CAN_MB_TX_MODE);
		bus->mailbox_set_priority(7, 10);
		bus->mailbox_set_accept_mask(7, 0x7FF, false);
	//}

	//Enable interrupts for RX boxes. TX interrupts are enabled later
	bus->enable_interrupt(CAN_IER_MB0 | CAN_IER_MB1 | CAN_IER_MB2 | CAN_IER_MB3 | CAN_IER_MB4 | CAN_IER_MB5 | CAN_IER_MB6);
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

//force trying to send on the given mailbox. Don't do this. Please, I beg you.
bool CanHandler::sendFrame(uint8_t mailbox, TX_CAN_FRAME& frame) {
	bus->mailbox_set_id(mailbox, frame.id, false);
	bus->mailbox_set_datalen(mailbox, frame.dlc);
	for (uint8_t cnt = 0; cnt < 8; cnt++)
		bus->mailbox_set_databyte(mailbox, cnt, frame.data[cnt]);
	bus->global_send_transfer_cmd((0x1u << mailbox));

	return true;
}

//Allow the canbus driver to figure out the proper mailbox to use 
//(whatever happens to be open) or queue it to send (if nothing is open)
bool CanHandler::sendFrame(TX_CAN_FRAME& frame) {
	bus->sendFrame(frame);
}
