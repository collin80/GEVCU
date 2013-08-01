/*
 * CanThrottle.cpp
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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_CAN_THROTTLE
#include "CanThrottle.h"

CanThrottle::CanThrottle() : Throttle() {
}

void CanThrottle::setup() {
	TickHandler::getInstance()->detach(this);

	CanHandler::getInstanceCar()->attach(this, CAN_THROTTLE_RESPONSE_ID, CAN_THROTTLE_RESPONSE_MASK, false);

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_CAN_THROTTLE);
}

/*
 * Send a request to the ECU.
 *
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe0, 0x22, 0xee, 0xcb]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e0 id=0x7e0 ide=0x00 rtr=0x00 data=0x03,0x22,0xEE,0xCB,0x00,0x00,0x00,0x00
 *
 */
void CanThrottle::handleTick()
{
	static TX_CAN_FRAME frame;
	frame.id = CAN_THROTTLE_REQUEST_ID;
	frame.dlc = 8;
	memcpy(frame.data, requestFrame, 8);
	CanHandler::getInstanceCar()->sendFrame(frame);
}

/*
 * Handle the response of the ECU. Log the data and convert the response
 * value to a percentage to display on the LCD.
 *
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00
 *
 * Note how vida represents the ID as part of the data (0x7e8 -->
 * 0x07, 0xe8) and how it ommits the first data byte which represents
 * the number of remaining bytes in the frame.
 */
void CanThrottle::handleCanFrame(RX_CAN_FRAME *frame) {
	if (frame->id == CAN_THROTTLE_RESPONSE_ID) {
		Logger::info("CAN Throttle response: %f%%", (float) frame->data[CAN_THROTTLE_DATA_BYTE] * 100.0 / 255.0);
	}
}

Device::DeviceId CanThrottle::getId() {
	return CANACCELPEDAL;
}

#endif // CFG_ENABLE_DEVICE_CAN_THROTTLE
