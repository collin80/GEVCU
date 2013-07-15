/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_HEARTBEAT
#include "HeartbeatDevice.h"

volatile int8_t tickReady;

HeartbeatDevice::HeartbeatDevice() {
	led = false;
}

void HeartbeatDevice::setupDevice() {
	TickHandler::unregisterDevice(this);

	TickHandler::registerDevice(this, CFG_TICK_INTERVAL_HEARTBEAT);
}

void HeartbeatDevice::handleTick() {
	tickReady = true;

	SerialUSB.print('.');
	if (led) {
		digitalWrite(BLINK_LED, HIGH);
	} else {
		digitalWrite(BLINK_LED, LOW);
	}
	led = !led;
}

Device::DeviceType HeartbeatDevice::getDeviceType() {
	return Device::DEVICE_MISC;
}

Device::DeviceId HeartbeatDevice::getDeviceID() {
	return Device::HEARTBEAT;
}

#endif //CFG_ENABLE_DEVICE_HEART_BEAT
