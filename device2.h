/*
 * Device.h
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

#ifndef DEVICE_H_
#define DEVICE_H_

#include <Arduino.h>
#include "config.h"
#include "Tickable.h"
#include "eeprom_layout.h"
#include "CanHandler.h"
#include "PrefHandler.h"

class Device: public Tickable {
public:
	enum DeviceType {
		DEVICE_ANY,
		DEVICE_MOTORCTRL,
		DEVICE_BMS,
		DEVICE_CHARGER,
		DEVICE_DISPLAY,
		DEVICE_THROTTLE,
		DEVICE_BRAKE,
		DEVICE_MISC,
		DEVICE_NONE
	};
	enum DeviceId { //unique device ID for every piece of hardware possible
		DMOC645 = 0x1000,
		BRUSACHARGE = 0x1010,
		TCCHCHARGE = 0x1020,
		POTACCELPEDAL = 0x1030,
		POTBRAKEPEDAL = 0x1031,
		CANACCELPEDAL = 0x1032,
		INVALID = 0xFFFF
	};
	Device();
	Device(CanHandler *canHandler);
	virtual void setup();
	virtual void handleTick();
	virtual void handleCanFrame(CANFrame& frame);
	virtual void handleMessage(uint32_t msgType, void* message);
	virtual DeviceType getType();
	virtual DeviceId getId();

protected:
	CanHandler *canHandler;
	PrefHandler *prefsHandler;

private:
};

#endif /* DEVICE_H_ */
