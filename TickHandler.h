/*
 * TickHandler.h
 *
 * Observer class where tickables can register to be triggered
 * on a certain interval.
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
		Tickable *tickable[CFG_TIMER_MAX_TICKABLES]; // array of pointers to tickables with this interval
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
