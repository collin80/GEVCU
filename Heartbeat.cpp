/*
 * HeartbeatDevice.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_HEARTBEAT
#include "Heartbeat.h"

Heartbeat::Heartbeat() {
	led = false;
}

void Heartbeat::setup() {
	TickHandler::remove(this);

	TickHandler::add(this, CFG_TICK_INTERVAL_HEARTBEAT);
}

void Heartbeat::handleTick() {
	SerialUSB.print('.');
	if (led) {
		digitalWrite(BLINK_LED, HIGH);
	} else {
		digitalWrite(BLINK_LED, LOW);
	}
	led = !led;

//	if (throttleDebug) {
//		Logger::debug("A0: %d, A1: %d, A2: %d, A3: %d", getAnalog(0), getAnalog(1), getAnalog(2), getAnalog(3));
//		Logger::debug("D0: %d, D1: %d, D2: %d, D3: %d", getDigital(0), getDigital(1), getDigital(2), getDigital(3));
//		Logger::debug("Throttle: %d", accelerator->getThrottle());
//		Logger::debug("Brake   : %d", brake->getThrottle());
//	}
}

#endif //CFG_ENABLE_DEVICE_HEART_BEAT
