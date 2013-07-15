/*
 * timer.h
 *
 * Created: 1/6/2013 8:26:58 PM
 *  Author: Collin Kidder
 */

#ifndef TIMER_H_
#define TIMER_H_

#include "config.h"
#include "TickHandler.h"

extern volatile int8_t tickReady; // TODO: remove once all devices use DeviceManager to register ticks

class HeartbeatDevice: public Device {
public:
	HeartbeatDevice();
	void setupDevice();
	void handleTick();
	Device::DeviceType getDeviceType();
	Device::DeviceId getDeviceID();

protected:

private:
	bool led;
};

#endif /* TIMER_H_ */
