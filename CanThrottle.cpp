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

	carType = Volvo_S80_Gas; //TODO: find a better way to configure the car type

	txFrame.dlc = 0x08;
	txFrame.ide = 0x00;
	txFrame.priority = 10;
	txFrame.rtr = 0x00;
	uint8_t txData[8];

	switch (Volvo_S80_Gas) {
	case Volvo_S80_Gas:
		// Request: dlc=0x08 fid=0x7e0 id=0x7e0 ide=0x00 rtr=0x00 data=0x03,0x22,0xEE,0xCB,0x00,0x00,0x00,0x00 (vida: [0x00, 0x00, 0x07, 0xe0, 0x22, 0xee, 0xcb])
		// Raw response: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00 (vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14])
		txFrame.id = 0x7e0;
		txData = { 0x03, 0x22, 0xee, 0xcb, 0x00, 0x00, 0x00, 0x00 };
		rxMask = rxId = 0x7e8;
		break;
	case Volvo_V50_Diesel:
		// Request: dlc=0x08 fid=0xFFFFE id=0x3FFFE ide=0x01 rtr=0x00 data=0xCD,0x11,0xA6,0x00,0x24,0x01,0x00,0x00 ([0x00, 0xf, 0xff, 0xfe, 0xcd, 0x11, 0xa6, 0x00, 0x24, 0x01, 0x00, 0x00])
		// Response: dlc=0x08 fid=0x400021 id=0x21 ide=0x01 rtr=0x00 data=0xCE,0x11,0xE6,0x00,0x24,0x03,0xFD,0x00 (vida: [0x00, 0x40, 0x00, 0x21, 0xce, 0x11, 0xe6, 0x00, 0x24, 0x03, 0xfd, 0x00])
		txFrame.ide = 0x01;
		txFrame.id = 0x3FFFE;
		txData = { 0xce, 0x11, 0xe6, 0x00, 0x24, 0x03, 0xfd, 0x00 };
		rxMask = rxId = 0x21;
		break;
	default:
		Logger::error("CanThrottle: no valid car type defined.");
	}
	memcpy(txFrame.data, txData, 8);
}

void CanThrottle::setup() {
	TickHandler::getInstance()->detach(this);
	CanHandler::getInstanceCar()->attach(this, rxId, rxMask, txFrame.ide > 0 ? true : false);
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_CAN_THROTTLE);
}

/*
 * Send a request to the ECU.
 *
 */
void CanThrottle::handleTick() {
	CanHandler::getInstanceCar()->sendFrame(txFrame);
}

/*
 * Handle the response of the ECU and calculate the throttle value
 *
 */
void CanThrottle::handleCanFrame(RX_CAN_FRAME *frame) {
	if (frame->id == rxId) {
		switch (carType) {
			case Volvo_S80_Gas:
				level = frame->data[4] * 1000 / 255;
				break;
			case Volvo_V50_Diesel:
				level = (frame->data[5] + 1) * frame->data[6] * 1000 / 1020;
				break;
		}
	}
}

Device::DeviceId CanThrottle::getId() {
	return CANACCELPEDAL;
}

#endif // CFG_ENABLE_DEVICE_CAN_THROTTLE
