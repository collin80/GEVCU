#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "device.h"
#include "config.h"
#include "throttle.h"
#include "MotorController.h"

#define MSG_HARD_FAULT		0x2000; //something really bad happened. Shutdown to safe state IMMEDIATELY!
#define MSG_SOFT_FAULT		0x2200; //something unpleasant happened. Try to shutdown gracefully.

class DeviceManager {
private:
	Device *devices[CFG_MAX_DEVICES];

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
