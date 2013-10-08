/*
 * DeviceManager.h
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

#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "config.h"
#include "Throttle.h"
#include "MotorController.h"
#include "CanHandler.h"
#include "Device.h"
#include "Sys_Messages.h"

class MotorController; // cyclic reference between MotorController and DeviceManager

class DeviceManager {
public:
	static DeviceManager *getInstance();
	void addDevice(Device *device);
	void removeDevice(Device *device);
//	void addTickObserver(TickObserver *observer, uint32_t frequency);
//	void addCanObserver(CanObserver *observer, uint32_t id, uint32_t mask, bool extended, CanHandler::CanBusNode canBus);
	void sendMessage(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, void* message);
	void setParameter(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, char *key, char *value);
	void setParameter(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, char *key, uint32_t value);
	uint8_t getNumThrottles();
	uint8_t getNumControllers();
	uint8_t getNumBMS();
	uint8_t getNumChargers();
	uint8_t getNumDisplays();
	Throttle *getAccelerator();
	Throttle *getBrake();
	MotorController *getMotorController();
	Device *getDeviceByID(DeviceId);
	Device *getDeviceByType(DeviceType);

protected:

private:
	DeviceManager();	// private constructor
	static DeviceManager *deviceManager;

	Device *devices[CFG_DEV_MGR_MAX_DEVICES];
	Throttle *throttle;
	Throttle *brake;
	MotorController *motorController;

	int8_t findDevice(Device *device);
	uint8_t countDeviceType(DeviceType deviceType);
};

#endif
