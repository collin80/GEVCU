#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "device.h"
#include "config.h"

#define MSG_THROTTLE_POS	0x1000; //send throttle position as tenths of a percent - message is signed 16bit int
#define MSG_HARD_FAULT		0x2000; //something really bad happened. Shutdown to safe state IMMEDIATELY!
#define MSG_SOFT_FAULT		0x2200; //something unpleasant happened. Try to shutdown gracefully.

class DEVICEMGR {
private:
	DEVICE *devices[CFG_MAX_DEVICES];

protected:

public:
	void addDevice(DEVICE *newdevice);
	void addTickHandler(DEVICE *newdevice, uint32_t freq);
	void addCANHandler(DEVICE *newdevice, uint32_t mask, uint32_t id, bool ext);
	void sendMessage(DEVICE::DEVTYPE devType, DEVICE::DEVID devId, uint32_t msgType, void* message);
};




#endif