/*
 * pedal_can.h
 *
 *  Created on: 18.06.2013
 *      Author: Michael Neuweiler
 */

#ifndef CAN_THROTTLE_H_
#define CAN_THROTTLE_H_

#include <Arduino.h>
#include "config.h"
#include "throttle.h"

#define CAN_THROTTLE_REQUEST_ID 0x7e0  // the can bus id of the throttle level request
#define CAN_THROTTLE_RESPONSE_ID 0x7e8 // the can bus id of the throttle level response
#define CAN_THROTTLE_DATA_BYTE 4 // the number of the data byte containing the throttle level
#define CAN_THROTTLE_REQUEST_DELAY 200 // milliseconds to wait between sending throttle requests

class CANThrottle: public Throttle {
public:

private:
	uint32_t lastRequestTime;

public:
	volatile void handleTick();
	void setupDevice();
	Device::DeviceType getDeviceType();
	Device::DeviceId getDeviceID();
	int getThrottle();
	CANThrottle();

protected:
	signed int outputThrottle; //the final signed throttle. [-1000, 1000] in tenths of a percent of maximum
};

#endif /* CAN_THROTTLE_H_ */
