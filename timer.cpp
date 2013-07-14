/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */

#include "timer.h"

volatile int8_t tickReady;
volatile int8_t agingTimer;

/*
 Make the timer interrupt every specified number of microseconds
 */
HeartbeatDevice::HeartbeatDevice(long microSeconds) {
	dotTick = 0;
	led = false;
	TickHandler::registerDevice(this, microSeconds);
}

void HeartbeatDevice::handleTick() {
	tickReady = true;
	agingTimer++;

	if (dotTick == 0) {
		SerialUSB.print('.'); //print . every 256 ticks (2.56 seconds)
		if (led) {
			digitalWrite(BLINKLED, HIGH);
		}
		else {
			digitalWrite(BLINKLED, LOW);
		}
		led = !led;
	}
	dotTick = dotTick + 1;
}
