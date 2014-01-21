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

/*
 * Private constructor of the can handler
 */
CanHandler::CanHandler(CanBusNode canBusNode) {
	this->canBusNode = canBusNode;

	// assign the correct bus instance to the pointer
	if (canBusNode == CAN_BUS_CAR)
		bus = &CAN2;
	else
		bus = &CAN;

	for (int i; i < CFG_CAN_NUM_OBSERVERS; i++)
		observerData[i].observer = NULL;
}

/*
 * Retrieve the singleton instance of the CanHandler which is responsible
 * for the EV can bus (CAN0)
 *
 * \retval the CanHandler instance for the EV can bus
 */
CanHandler* CanHandler::getInstanceEV()
{
	if (canHandlerEV == NULL)
		canHandlerEV = new CanHandler(CAN_BUS_EV);
	return canHandlerEV;
}

/*
 * Retrieve the singleton instance of the CanHandler which is responsible
 * for the car can bus (CAN1)
 *
 * \retval the CanHandler instance for the car can bus
 */
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
	bus->init(canBusNode == CAN_BUS_EV ? CFG_CAN0_SPEED : CFG_CAN1_SPEED);

	//Mailboxes are default set up initialized with one MB for TX and the rest for RX
	//That's OK with us so no need to initialize those things there.

	Logger::info("CAN%d init ok", (canBusNode == CAN_BUS_EV ? 0 : 1));
}

/*
 * Attach a CanObserver. Can frames which match the id/mask will be forwarded to the observer
 * via the method handleCanFrame(RX_CAN_FRAME).
 * Sets up a can bus mailbox if necessary.
 *
 *  \param observer - the observer object to register (must implement CanObserver class)
 *  \param id - the id of the can frame to listen to
 *  \param mask - the mask to be applied to the frames
 *  \param extended - set if extended frames must be supported
 */
void CanHandler::attach(CanObserver* observer, uint32_t id, uint32_t mask, bool extended) {
	int8_t pos = findFreeObserverData();
	if (pos == -1) {
		Logger::error("no free space in CanHandler::observerData, increase its size via CFG_CAN_NUM_OBSERVERS");
		return;
	}

	int mailbox = bus->findFreeRXMailbox();
	if (mailbox == -1) {
		Logger::error("no free CAN mailbox on bus %d", canBusNode);
		return;
	}

	observerData[pos].id = id;
	observerData[pos].mask = mask;
	observerData[pos].extended = extended;
	observerData[pos].mailbox = mailbox;
	observerData[pos].observer = observer;

	bus->setRXFilter((uint8_t)mailbox, id, mask, extended);

	Logger::debug("attached CanObserver (%X) for id=%X, mask=%X, mailbox=%d", observer, id, mask, mailbox);
}

/*
 * Detaches a previously attached observer from this handler.
 *
 * \param observer - observer object to detach
 * \param id - id of the observer to detach (required as one CanObserver may register itself several times)
 * \param mask - mask of the observer to detach (dito)
 */
void CanHandler::detach(CanObserver* observer, uint32_t id, uint32_t mask) {
	for (int i = 0; i < CFG_CAN_NUM_OBSERVERS; i++) {
		if (observerData[i].observer == observer &&
				observerData[i].id == id &&
				observerData[i].mask == mask) {
			observerData[i].observer = NULL;

			//TODO: if no more observers on same mailbox, disable its interrupt, reset mailbox
		}
	}
}

/*
 * Logs the content of a received can frame
 *
 * \param frame - the received can frame to log
 */
void CanHandler::logFrame(CAN_FRAME& frame) {
	if (Logger::isDebug()) {
		Logger::debug("CAN: dlc=%X fid=%X id=%X ide=%X rtr=%X data=%X,%X,%X,%X,%X,%X,%X,%X",
				frame.length, frame.fid, frame.id, frame.extended, frame.rtr,
				frame.data.bytes[0], frame.data.bytes[1], frame.data.bytes[2], frame.data.bytes[3],
				frame.data.bytes[4], frame.data.bytes[5], frame.data.bytes[6], frame.data.bytes[7]);
	}
}

/*
 * Find a observerData entry which is not in use.
 *
 * \retval array index of the next unused entry in observerData[]
 */
int8_t CanHandler::findFreeObserverData() {
	for (int i = 0; i < CFG_CAN_NUM_OBSERVERS; i++) {
		if (observerData[i].observer == NULL)
			return i;
	}
	return -1;
}

/*
 * Find a unused can mailbox according to entries in observerData[].
 *
 * \retval the mailbox index of the next unused mailbox
 */
int8_t CanHandler::findFreeMailbox() {
	uint8_t numRxMailboxes = (canBusNode == CAN_BUS_EV ? CFG_CAN0_NUM_RX_MAILBOXES : CFG_CAN1_NUM_RX_MAILBOXES);
	for (uint8_t i = 0; i < numRxMailboxes; i++) {
		bool used = false;
		for (uint8_t j = 0; j < CFG_CAN_NUM_OBSERVERS; j++) {
			if (observerData[j].observer != NULL && observerData[j].mailbox == i)
				used = true;
		}
		if (!used)
			return i;
	}
	return -1;
}

/*
 * If a message is available, read it and forward it to registered observers.
 */
void CanHandler::process() {
	static CAN_FRAME frame;

	if (bus->rx_avail()) {
		bus->get_rx_buff(frame);
//		logFrame(frame);

		for (int i = 0; i < CFG_CAN_NUM_OBSERVERS; i++) {
			if (observerData[i].observer != NULL) {
				// Apply mask to frame.id and observer.id. If they match, forward the frame to the observer
				if ((frame.id & observerData[i].mask) == (observerData[i].id & observerData[i].mask))
					observerData[i].observer->handleCanFrame(&frame);
			}
		}
	}
}

//Allow the canbus driver to figure out the proper mailbox to use 
//(whatever happens to be open) or queue it to send (if nothing is open)
void CanHandler::sendFrame(CAN_FRAME& frame) {
	bus->sendFrame(frame);
}

/*
 * Default implementation of the CanObserver method. Must be overwritten
 * by every sub-class.
 */
void CanObserver::handleCanFrame(CAN_FRAME *frame) {
	Logger::error("CanObserver does not implement handleCanFrame(), frame.id=%d", frame->id);
}
