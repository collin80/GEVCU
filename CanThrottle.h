/*
 * CanThrottle.h
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
	CanThrottle(CanHandler *canHandler);
	void setup();
	void handleTick();
	Device::DeviceId getId();
	int getThrottle();

protected:
	signed int outputThrottle; //the final signed throttle. [-1000, 1000] in tenths of a percent of maximum

private:
	uint32_t lastRequestTime;
};

#endif /* CAN_THROTTLE_H_ */
