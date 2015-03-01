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

#ifndef DEVICE_TYPES_H_
#define DEVICE_TYPES_H_

enum DeviceType {
	DEVICE_ANY = 0,
	DEVICE_MOTORCTRL = 1,
	DEVICE_BMS = 2,
	DEVICE_CHARGER = 3,
	DEVICE_DISPLAY = 4,
	DEVICE_THROTTLE = 5,
	DEVICE_BRAKE = 6,
	DEVICE_MISC = 7,
	DEVICE_WIFI = 8,
	DEVICE_NONE = 100
};

enum DeviceId { //unique device ID for every piece of hardware possible
	DMOC645 = 0x1000,
	BRUSA_DMC5 = 0x1001,
    CODAUQM = 0x1002,
	BRUSACHARGE = 0x1010,
	TCCHCHARGE = 0x1020,
	THROTTLE = 0x1030,
	POTACCELPEDAL = 0x1031,
	POTBRAKEPEDAL = 0x1032,
	CANACCELPEDAL = 0x1033,
	CANBRAKEPEDAL = 0x1034,
	ICHIP2128 = 0x1040,
	THINKBMS = 0x2000,
	FAULTSYS = 0x4000,
	RX8DASH = 0x4100,
	SYSTEM = 0x5000,
	HEARTBEAT = 0x5001,
	MEMCACHE = 0x5002,
	PIDLISTENER = 0x6000,
	ELM327EMU = 0x6500,
	INVALID = 0xFFFF
};

#endif /* DEVICE_TYPES_H_ */
