#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "Device.h"
#include "config.h"
#include "Throttle.h"
#include "MotorController.h"

#define MSG_HARD_FAULT		0x2000; //something really bad happened. Shutdown to safe state IMMEDIATELY!
#define MSG_SOFT_FAULT		0x2200; //something unpleasant happened. Try to shutdown gracefully.

class MotorController; // cyclic reference between MotorController and DeviceManager

class DeviceManager {
public:
	static DeviceManager *getInstance();
	void addDevice(Device *device);
	void removeDevice(Device *device);
	void addTickHandler(Device *device, uint32_t frequency);
	void addCanHandler(Device *device, uint32_t mask, uint32_t id, bool extended, uint8_t busNumber);
	void sendMessage(Device::DeviceType deviceType, Device::DeviceId deviceId, uint32_t msgType, void* message);
	uint8_t getNumThrottles();
	uint8_t getNumControllers();
	uint8_t getNumBMS();
	uint8_t getNumChargers();
	uint8_t getNumDisplays();
	Throttle *getAccelerator();
	Throttle *getBrake();
	MotorController *getMotorController();

protected:

private:
	DeviceManager();	// private constructor
	static DeviceManager *deviceManager;

	Device *devices[CFG_DEV_MGR_MAX_DEVICES];
	Throttle *throttle;
	Throttle *brake;
	MotorController *motorController;

	int8_t findDevice(Device *device);
	uint8_t countDeviceType(Device::DeviceType deviceType);
};

#endif
