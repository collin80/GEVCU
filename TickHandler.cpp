/*
 * tick_handler.cpp
 *
 * Observer class to which devices can register to be triggered
 * on a certain interval.
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#include "TickHandler.h"

TickHandler::TickHandler() {
	for (int i = 0; i < CFG_MAX_TICK_DEVICES; i++) {
		tickDevice[i] = NULL;
	}
}

void TickHandler::registerDevice(Device* device, uint32_t tickInterval) {
	int timer = findTimer(NULL);
	tickDevice[timer] = device;
	switch (timer) {
	case 0:
		Timer0.setPeriod(tickInterval).attachInterrupt(timer0Interrupt).start();
		break;
	case 1:
		Timer1.setPeriod(tickInterval).attachInterrupt(timer1Interrupt).start();
		break;
	case 2:
		Timer2.setPeriod(tickInterval).attachInterrupt(timer2Interrupt).start();
		break;
	case 3:
		Timer3.setPeriod(tickInterval).attachInterrupt(timer3Interrupt).start();
		break;
	case 4:
		Timer4.setPeriod(tickInterval).attachInterrupt(timer4Interrupt).start();
		break;
	case 5:
		Timer5.setPeriod(tickInterval).attachInterrupt(timer5Interrupt).start();
		break;
	case 6:
		Timer6.setPeriod(tickInterval).attachInterrupt(timer6Interrupt).start();
		break;
	case 7:
		Timer7.setPeriod(tickInterval).attachInterrupt(timer7Interrupt).start();
		break;
	case 8:
		Timer8.setPeriod(tickInterval).attachInterrupt(timer8Interrupt).start();
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

void timer0Interrupt() {
	TickHandler::handleInterrupt(0);
}
void timer1Interrupt() {
	TickHandler::handleInterrupt(1);
}
void timer2Interrupt() {
	TickHandler::handleInterrupt(2);
}
void timer3Interrupt() {
	TickHandler::handleInterrupt(3);
}
void timer4Interrupt() {
	TickHandler::handleInterrupt(4);
}
void timer5Interrupt() {
	TickHandler::handleInterrupt(5);
}
void timer6Interrupt() {
	TickHandler::handleInterrupt(6);
}
void timer7Interrupt() {
	TickHandler::handleInterrupt(7);
}
void timer8Interrupt() {
	TickHandler::handleInterrupt(8);
}
