/*
 * TickHandler.h
 *
 * Observer class where tickables can register to be triggered
 * on a certain interval.
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#ifndef TICKHANDLER_H_
#define TICKHANDLER_H_

#include "config.h"
#include "Tickable.h"
#include <DueTimer.h>
#include "Logger.h"

#define NUM_TIMERS 9

class TickHandler {
public:
	static void initialize();
	static void add(Tickable *tickable, uint32_t interval);
	static void remove(Tickable *tickable);
	static void handleInterrupt(int timerNumber); // must be public when from the non-class functions

protected:

private:
	struct TimerEntry {
		long interval; // interval of timer
		Tickable *tickable[CFG_MAX_TICKABLES]; // array of pointers to tickables with this interval
	};
	static TimerEntry timerEntry[NUM_TIMERS]; // array of timer entries (9 as there are 9 timers)
	static int findTimer(long interval);
	static int findTickable(int timerNumber, Tickable *tickable);
};

void timer0Interrupt();
void timer1Interrupt();
void timer2Interrupt();
void timer3Interrupt();
void timer4Interrupt();
void timer5Interrupt();
void timer6Interrupt();
void timer7Interrupt();
void timer8Interrupt();

#endif /* TICKHANDLER_H_ */
