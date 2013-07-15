/*
 * can_throttle.cpp
 *
 *  Created on: 18.06.2013
 *      Author: Michael Neuweiler
 */

#include "CanThrottle.h"

CanThrottle::CanThrottle()
{
}

void CanThrottle::setupDevice()
{
	lastRequestTime = 0;

	//Initialize mailbox 0 to receive messages from the ECU Id
	CAN.mailbox_init(0);
	CAN.mailbox_set_mode(0, CAN_MB_RX_MODE);
	CAN.mailbox_set_accept_mask(0, CAN_THROTTLE_RESPONSE_ID, false);
	CAN.mailbox_set_id(0, CAN_THROTTLE_RESPONSE_ID, false);

	// Initialize mailbox 1 to send data to the ECU.
	CAN.mailbox_init(1);
	CAN.mailbox_set_mode(1, CAN_MB_TX_MODE);
	CAN.mailbox_set_priority(1, 10);
	CAN.mailbox_set_accept_mask(1, 0x7FF, false);
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
	Logger::debug("sending CAN throttle request to ECU");
	CAN.mailbox_set_id(1, CAN_THROTTLE_REQUEST_ID, false);
	CAN.mailbox_set_datalen(1, 8);
	CAN.mailbox_set_datal(1, 0xcbee2203);
	CAN.mailbox_set_datah(1, 0x00000000);

	CAN.global_send_transfer_cmd(CAN_TCR_MB1);
}

int CanThrottle::getThrottle()
{
}

/*
 * Handle the response of the ECU. Log the data on the serial port
 * and convert the response value to a percentage to display on
 * the LCD.
 *
 * Trace log of Vida: [0x00, 0x00, 0x07, 0xe8, 0x62, 0xee, 0xcb, 0x14]
 * Trace log of CANMonitor: dlc=0x08 fid=0x7e8 id=0x7e8 ide=0x00 rtr=0x00 data=0x04,0x62,0xEE,0xCB,0x14,0x00,0x00,0x00
 *
 * Note how vida represents the ID as part of the data (0x7e8 -->
 * 0x07, 0xe8) and how it ommits the first data byte which represents
 * the number of remaining bytes in the frame.
 */
void handleResponse(CANFrame& message) {
	if (message.id == CAN_THROTTLE_RESPONSE_ID) {
		SerialUSB.print("CAN Throttle: ");
		SerialUSB.print((float) message.data[CAN_THROTTLE_DATA_BYTE] * 100.0 / 255.0);
		SerialUSB.println("%");
	}
}

Device::DeviceType CanThrottle::getDeviceType()
{
}

Device::DeviceId CanThrottle::getDeviceID()
{
}
