/*
 * TickHandler.h
 *
 * Class where TickObservers can register to be triggered
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
#include <DueTimer.h>
#include "Logger.h"

#define NUM_TIMERS 9

class TickObserver {
public:
	virtual void handleTick();
};


class TickHandler {
public:
	static TickHandler *getInstance();
	void attach(TickObserver *observer, uint32_t interval);
	void detach(TickObserver *observer);
	void handleInterrupt(int timerNumber); // must be public when from the non-class functions
#ifdef CFG_TIMER_USE_QUEUING
	void cleanBuffer();
	void process();
#endif

protected:

private:
	struct TimerEntry {
		long interval; // interval of timer
		TickObserver *observer[CFG_TIMER_NUM_OBSERVERS]; // array of pointers to observers with this interval
	};
	TimerEntry timerEntry[NUM_TIMERS]; // array of timer entries (9 as there are 9 timers)
	static TickHandler *tickHandler;
#ifdef CFG_TIMER_USE_QUEUING
	TickObserver *tickBuffer[CFG_TIMER_BUFFER_SIZE];
	volatile uint16_t bufferHead, bufferTail;
#endif

	TickHandler();
	int findTimer(long interval);
	int findObserver(int timerNumber, TickObserver *observer);
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
