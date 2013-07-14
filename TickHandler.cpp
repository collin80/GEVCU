/*
 * TickHandler.cpp
 *
 * Observer class to which devices can register to be triggered
 * on a certain interval.
 * Devices with the same interval are grouped to the same timer
 * and triggered in sequence per timer interrupt.
 *
 * NOTE: The initialize() method must be called before a device is registered !
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#include "TickHandler.h"

TickHandler::TimerEntry TickHandler::timerEntry[NUM_TIMERS] = {};

/**
 * Initializes all TimerEntries and their device lists.
 * This method must be called before any device is registered !
 */
void TickHandler::initialize() {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
		timerEntry[timerNumber].interval = 0;
		for (int deviceNumber = 0; deviceNumber < CFG_MAX_DEVICES; deviceNumber++) {
			timerEntry[timerNumber].device[deviceNumber] = NULL;
		}
	}
}

/**
 * Register a device to be ticked in a certain interval.
 * Devices with the same interval are grouped to one timer device to save timers.
 * A device may be registered multiple times with different intervals.
 *
 * First a timer with the same interval is looked up. If none found, a free one is
 * used. Then a free device slot (of max CFG_MAX_DEVICES) is looked up. If all went
 * well, the timer is configured and (re)started.
 */
void TickHandler::registerDevice(Device* device, uint32_t interval) {
	int timerNumber = findTimer(interval);
	if (timerNumber == -1) {
		timerNumber = findTimer(0);	// no timer with given tick interval exsist -> look for unused (interval == 0)
		if(timerNumber == -1) {
			Logger::error("No free timer available for interval=%d", interval);
			return;
		}
		timerEntry[timerNumber].interval = interval;
	}

	int deviceNumber = findDevice(timerNumber, 0);
	if (deviceNumber == -1) {
		Logger::error("No free device slot for timer %d with interval %d", timerNumber, timerEntry[timerNumber].interval);
		return;
	}
	timerEntry[timerNumber].device[deviceNumber] = device;
	Logger::debug("register device %d as number %d to timer %d, %dus interval", device, deviceNumber, timerNumber, interval);

	switch (timerNumber) { // restarting a timer which would already be running is no problem (see DueTimer.cpp)
	case 0:
		Timer0.setPeriod(interval).attachInterrupt(timer0Interrupt).start();
		break;
	case 1:
		Timer1.setPeriod(interval).attachInterrupt(timer1Interrupt).start();
		break;
	case 2:
		Timer2.setPeriod(interval).attachInterrupt(timer2Interrupt).start();
		break;
	case 3:
		Timer3.setPeriod(interval).attachInterrupt(timer3Interrupt).start();
		break;
	case 4:
		Timer4.setPeriod(interval).attachInterrupt(timer4Interrupt).start();
		break;
	case 5:
		Timer5.setPeriod(interval).attachInterrupt(timer5Interrupt).start();
		break;
	case 6:
		Timer6.setPeriod(interval).attachInterrupt(timer6Interrupt).start();
		break;
	case 7:
		Timer7.setPeriod(interval).attachInterrupt(timer7Interrupt).start();
		break;
	case 8:
		Timer8.setPeriod(interval).attachInterrupt(timer8Interrupt).start();
		break;
	}
}

/**
 * Remove a device from all timers where it was registered.
 */
void TickHandler::unregisterDevice(Device* device) {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
		for (int deviceNumber = 0; deviceNumber < CFG_MAX_DEVICES; deviceNumber++) {
			if (timerEntry[timerNumber].device[deviceNumber] == device)
				timerEntry[timerNumber].device[deviceNumber] = NULL;
		}
	}
}

/**
 * Find a timer with a specified interval.
 */
int TickHandler::findTimer(long interval) {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
//Logger::debug("findtimer: timer=%d, interval=%d , device[0]=%d", timerNumber, timerEntry[timerNumber].interval, timerEntry[timerNumber].device[0]);
		if (timerEntry[timerNumber].interval == interval)
			return timerNumber;
	}
	return -1;
}

/*
 * Find a device in the list of a specific timer.
 */
int TickHandler::findDevice(int timerNumber, Device *device) {
	for (int deviceNumber = 0; deviceNumber < CFG_MAX_DEVICES; deviceNumber++) {
//Logger::debug("findDevice: device=%d , device[%d]=%d", device, deviceNumber, timerEntry[timerNumber].device[deviceNumber]);
		if (timerEntry[timerNumber].device[deviceNumber] == device)
			return deviceNumber;
	}
	return -1;
}

/*
 * Handle the interrupt of any timer.
 * All the registered devices of the timer are called.
 */
void TickHandler::handleInterrupt(int timerNumber) {
	for (int deviceNumber = 0; deviceNumber < CFG_MAX_DEVICES; deviceNumber++) {
		if (timerEntry[timerNumber].device[deviceNumber] != NULL) {
			timerEntry[timerNumber].device[deviceNumber]->handleTick();
		}
	}
}

/*
 * Interrupt function for Timer0
 */
void timer0Interrupt() {
	TickHandler::handleInterrupt(0);
}
/*
 * Interrupt function for Timer1
 */
void timer1Interrupt() {
	TickHandler::handleInterrupt(1);
}
/*
 * Interrupt function for Timer2
 */
void timer2Interrupt() {
	TickHandler::handleInterrupt(2);
}
/*
 * Interrupt function for Timer3
 */
void timer3Interrupt() {
	TickHandler::handleInterrupt(3);
}
/*
 * Interrupt function for Timer4
 */
void timer4Interrupt() {
	TickHandler::handleInterrupt(4);
}
/*
 * Interrupt function for Timer5
 */
void timer5Interrupt() {
	TickHandler::handleInterrupt(5);
}
/*
 * Interrupt function for Timer6
 */
void timer6Interrupt() {
	TickHandler::handleInterrupt(6);
}
/*
 * Interrupt function for Timer7
 */
void timer7Interrupt() {
	TickHandler::handleInterrupt(7);
}
/*
 * Interrupt function for Timer8
 */
void timer8Interrupt() {
	TickHandler::handleInterrupt(8);
}
