/*
 * TickHandler.cpp
 *
 * Observer class to which tickables can register to be triggered
 * on a certain interval.
 * Tickables with the same interval are grouped to the same timer
 * and triggered in sequence per timer interrupt.
 *
 * NOTE: The initialize() method must be called before a tickable is registered !
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */

#include "TickHandler.h"

TickHandler::TimerEntry TickHandler::timerEntry[NUM_TIMERS] = {};

/**
 * Initializes all TimerEntries and their tickable lists.
 * This method must be called before any tickable is registered !
 */
void TickHandler::initialize() {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
		timerEntry[timerNumber].interval = 0;
		for (int tickableNumber = 0; tickableNumber < CFG_TIMER_MAX_TICKABLES; tickableNumber++) {
			timerEntry[timerNumber].tickable[tickableNumber] = NULL;
		}
	}
}

/**
 * Register a tickable to be ticked in a certain interval.
 * Tickables with the same interval are grouped to one timer to save timers.
 * A tickable may be registered multiple times with different intervals.
 *
 * First a timer with the same interval is looked up. If none found, a free one is
 * used. Then a free tickable slot (of max CFG_MAX_TICKABLES) is looked up. If all went
 * well, the timer is configured and (re)started.
 */
void TickHandler::add(Tickable* tickable, uint32_t interval) {
	int timerNumber = findTimer(interval);
	if (timerNumber == -1) {
		timerNumber = findTimer(0);	// no timer with given tick interval exsist -> look for unused (interval == 0)
		if(timerNumber == -1) {
			Logger::error("No free timer available for interval=%d", interval);
			return;
		}
		timerEntry[timerNumber].interval = interval;
	}

	int tickableNumber = findTickable(timerNumber, 0);
	if (tickableNumber == -1) {
		Logger::error("No free tickable slot for timer %d with interval %d", timerNumber, timerEntry[timerNumber].interval);
		return;
	}
	timerEntry[timerNumber].tickable[tickableNumber] = tickable;
	Logger::debug("register tickable %d as number %d to timer %d, %dus interval", tickable, tickableNumber, timerNumber, interval);

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
 * Remove a tickable from all timers where it was registered.
 */
void TickHandler::remove(Tickable* tickable) {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
		for (int tickableNumber = 0; tickableNumber < CFG_TIMER_MAX_TICKABLES; tickableNumber++) {
			if (timerEntry[timerNumber].tickable[tickableNumber] == tickable)
				timerEntry[timerNumber].tickable[tickableNumber] = NULL;
		}
	}
}

/**
 * Find a timer with a specified interval.
 */
int TickHandler::findTimer(long interval) {
	for (int timerNumber = 0; timerNumber < NUM_TIMERS; timerNumber++) {
//Logger::debug("findtimer: timer=%d, interval=%d , tickable[0]=%d", timerNumber, timerEntry[timerNumber].interval, timerEntry[timerNumber].tickable[0]);
		if (timerEntry[timerNumber].interval == interval)
			return timerNumber;
	}
	return -1;
}

/*
 * Find a tickable in the list of a specific timer.
 */
int TickHandler::findTickable(int timerNumber, Tickable *tickable) {
	for (int tickableNumber = 0; tickableNumber < CFG_TIMER_MAX_TICKABLES; tickableNumber++) {
//Logger::debug("findTickable: tickable=%d , tickable[%d]=%d", tickable, tickableNumber, timerEntry[timerNumber].tickable[tickableNumber]);
		if (timerEntry[timerNumber].tickable[tickableNumber] == tickable)
			return tickableNumber;
	}
	return -1;
}

/*
 * Handle the interrupt of any timer.
 * All the registered tickables of the timer are called.
 */
void TickHandler::handleInterrupt(int timerNumber) {
	for (int tickableNumber = 0; tickableNumber < CFG_TIMER_MAX_TICKABLES; tickableNumber++) {
		if (timerEntry[timerNumber].tickable[tickableNumber] != NULL) {
			timerEntry[timerNumber].tickable[tickableNumber]->handleTick();
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
