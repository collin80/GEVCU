/*
 * can_throttle.h
 *
 *  Created on: 18.06.2013
 *      Author: Michael Neuweiler
 */

#ifndef CAN_THROTTLE_H_
#define CAN_THROTTLE_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "TickHandler.h"

#define CAN_THROTTLE_REQUEST_ID 0x7e0  // the can bus id of the throttle level request
#define CAN_THROTTLE_RESPONSE_ID 0x7e8 // the can bus id of the throttle level response
#define CAN_THROTTLE_DATA_BYTE 4 // the number of the data byte containing the throttle level
#define CAN_THROTTLE_REQUEST_DELAY 200 // milliseconds to wait between sending throttle requests

class CanThrottle: public Throttle {
public:
	CanThrottle();
	void setup();
	void handleTick();
	Device::DeviceId getId();

protected:

private:
	uint32_t lastRequestTime;
};

#endif /* CAN_THROTTLE_H_ */
