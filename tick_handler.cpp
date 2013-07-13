/*
 * tick_handler.cpp
 *
 * Observer class to which devices can register to be triggered
 * on a certain interval.
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#include "tick_handler.h"

TickHandler::TickHandler() {
	for (int i = 0; i < CFG_MAX_TICK_DEVICES; i++) {
		tickDevice[i] = NULL;
	}
}

void TickHandler::registerDevice(Device* device, uint32_t tickInterval) {
	uint32_t frequency = 1000000ul / tickInterval;
	int timer = findTimer(NULL);
	tickDevice[timer] = device;
	switch (timer) {
	case 0:
		Timer0.attachInterrupt(timer0Interrupt).start(tickInterval);
		break;
	case 1:
		Timer1.attachInterrupt(timer1Interrupt).start(tickInterval);
		break;
	case 2:
		Timer2.attachInterrupt(timer2Interrupt).start(tickInterval);
		break;
	case 3:
		Timer3.attachInterrupt(timer3Interrupt).start(tickInterval);
		break;
	case 4:
		Timer4.attachInterrupt(timer4Interrupt).start(tickInterval);
		break;
	case 5:
		Timer5.attachInterrupt(timer5Interrupt).start(tickInterval);
		break;
	case 6:
		Timer6.attachInterrupt(timer6Interrupt).start(tickInterval);
		break;
	case 7:
		Timer7.attachInterrupt(timer7Interrupt).start(tickInterval);
		break;
	case 8:
		Timer8.attachInterrupt(timer8Interrupt).start(tickInterval);
		break;
	case -1:
		Logger::error("Could not register tick device with interval %d", tickInterval);
	}
}

void TickHandler::unregisterDevice(Device* device) {
	int timer = findTimer(device);
	switch (timer) {
	case 0:
		Timer0.stop();
		break;
	case 1:
		Timer1.stop();
		break;
	case 2:
		Timer2.stop();
		break;
	case 3:
		Timer3.stop();
		break;
	case 4:
		Timer4.stop();
		break;
	case 5:
		Timer5.stop();
		break;
	case 6:
		Timer6.stop();
		break;
	case 7:
		Timer7.stop();
		break;
	case 8:
		Timer8.stop();
		break;
	}
	if (timer > 0)
		tickDevice[timer] = NULL;
}

int TickHandler::findTimer(Device *device) {
	for (int i = 0; i < CFG_MAX_TICK_DEVICES; i++) {
		if (tickDevice[i] == device)
			return i;
	}
	return -1;
}

void TickHandler::handleInterrupt(int device) {
	tickDevice[device]->handleTick();
}

void timer0Int() {
	TickHandler::handleInterrupt(0);
}
void timer1Interrupt() {
	TickHandler::handleInterrupt(1);
}
void timer2Int() {
	TickHandler::handleInterrupt(2);
}
void timer3Int() {
	TickHandler::handleInterrupt(3);
}
void timer4Int() {
	TickHandler::handleInterrupt(4);
}
void timer5Int() {
	TickHandler::handleInterrupt(5);
}
void timer6Int() {
	TickHandler::handleInterrupt(6);
}
void timer7Int() {
	TickHandler::handleInterrupt(7);
}
void timer8Int() {
	TickHandler::handleInterrupt(8);
}
