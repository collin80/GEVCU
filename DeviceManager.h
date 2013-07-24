/*
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

#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "Device.h"
#include "config.h"
#include "Throttle.h"
#include "MotorController.h"

#define MSG_HARD_FAULT		0x2000; //something really bad happened. Shutdown to safe state IMMEDIATELY!
#define MSG_SOFT_FAULT		0x2200; //something unpleasant happened. Try to shutdown gracefully.

class DeviceManager {
private:
	Device *devices[CFG_MAX_TICKABLES];

protected:

public:
	void addDevice(Device *newdevice);
	void addTickHandler(Device *newdevice, uint32_t freq);
	void addCANHandler(Device *newdevice, uint32_t mask, uint32_t id, bool ext, uint8_t canbus);
	void sendMessage(Device::DeviceType devType, Device::DeviceId devId, uint32_t msgType, void* message);
	uint8_t getNumThrottles();
	uint8_t getNumControllers();
	uint8_t getNumBMS();
	uint8_t getNumChargers();
	uint8_t getNumDisplays();
	Throttle getThrottle(uint8_t which);
	MotorController getMotorCtrl(uint8_t which);
};

#endif
