/*
 * TickHandler.cpp
 *
 * Class to which TickObserver objects can register to be triggered
 * on a certain interval.
 * TickObserver with the same interval are grouped to the same timer
 * and triggered in sequence per timer interrupt.
 *
 * NOTE: The initialize() method must be called before a observer is registered !
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

TickHandler tickHandler;

TickHandler::TickHandler()
{
    for (int i = 0; i < NUM_TIMERS; i++) {
        timerEntry[i].interval = 0;

        for (int j = 0; j < CFG_TIMER_NUM_OBSERVERS; j++) {
            timerEntry[i].observer[j] = NULL;
        }
    }
    bufferHead = bufferTail = 0;
}

/**
 * Register an observer to be triggered in a certain interval.
 * TickObservers with the same interval are grouped to one timer to save timers.
 * A TickObserver may be registered multiple times with different intervals.
 *
 * First a timer with the same interval is looked up. If none found, a free one is
 * used. Then a free TickObserver slot (of max CFG_MAX_TICK_OBSERVERS) is looked up. If all went
 * well, the timer is configured and (re)started.
 */
void TickHandler::attach(TickObserver* observer, uint32_t interval)
{
    if (isAttached(observer, interval)) {
        Logger::warn("TickObserver %#x is already attached with interval %d", observer, interval);
        return;
    }

    int timer = findTimer(interval);

    if (timer == -1) {
        timer = findTimer(0);   // no timer with given tick interval exist -> look for unused (interval == 0)

        if (timer == -1) {
            Logger::error("No free timer available for interval=%d", interval);
            return;
        }

        timerEntry[timer].interval = interval;
    }

    int observerIndex = findObserver(timer, 0);

    if (observerIndex == -1) {
        Logger::error("No free observer slot for timer %d with interval %d", timer, timerEntry[timer].interval);
        return;
    }

    timerEntry[timer].observer[observerIndex] = observer;
    Logger::debug("attached TickObserver (%#x) as number %d to timer %d, %lu interval", observer, observerIndex, timer, interval);

    switch (timer) { // restarting a timer which would already be running is no problem (see DueTimer.cpp)
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

/*
 * Check if a observer is attached to this handler.
 *
 * \param observer - observer object to search
 * \param interval - interval of the observer to search
 */
bool TickHandler::isAttached(TickObserver* observer, uint32_t interval)
{
    for (int timer = 0; timer < NUM_TIMERS; timer++) {
        for (int observerIndex = 0; observerIndex < CFG_TIMER_NUM_OBSERVERS; observerIndex++) {
            if (timerEntry[timer].observer[observerIndex] == observer &&
                    timerEntry[timer].interval == interval) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Remove an observer from all timers where it was registered.
 */
void TickHandler::detach(TickObserver* observer)
{
    for (int timer = 0; timer < NUM_TIMERS; timer++) {
        for (int observerIndex = 0; observerIndex < CFG_TIMER_NUM_OBSERVERS; observerIndex++) {
            if (timerEntry[timer].observer[observerIndex] == observer) {
                Logger::debug("removing TickObserver (%#x) as number %d from timer %d", observer, observerIndex, timer);
                timerEntry[timer].observer[observerIndex] = NULL;
            }
        }
    }
}

/**
 * Find a timer with a specified interval.
 */
int TickHandler::findTimer(long interval)
{
    for (int i = 0; i < NUM_TIMERS; i++) {
        if (timerEntry[i].interval == interval) {
            return i;
        }
    }

    return -1;
}

/*
 * Find a TickObserver in the list of a specific timer.
 */
int TickHandler::findObserver(int timer, TickObserver *observer)
{
    for (int i = 0; i < CFG_TIMER_NUM_OBSERVERS; i++) {
        if (timerEntry[timer].observer[i] == observer) {
            return i;
        }
    }

    return -1;
}

/*
 * Process all queued tick observers in tickBuffer and call their handleTick() method.
 * The entries were enqueued during an interrupt by handleInterrupt().
 */
void TickHandler::process()
{
    while (bufferHead != bufferTail) {
        if (tickBuffer[bufferTail] == NULL) {
            Logger::error("tickBuffer pointer mismatch");
        } else {
//            Logger::debug("tickHandler->process, bufferHead=%d bufferTail=%d", bufferHead, bufferTail);
            tickBuffer[bufferTail]->handleTick();
            tickBuffer[bufferTail] = NULL;
        }
        bufferTail = (bufferTail + 1) % CFG_TIMER_BUFFER_SIZE;
    }
}

void TickHandler::cleanBuffer()
{
    bufferHead = bufferTail = 0;
}

/*
 * Handle the interrupt of any timer.
 * All the registered TickObservers of the timer are added to tickBuffer (queue) to be processed outside of an interrupt (loop)
 */
void TickHandler::handleInterrupt(int timerNumber)
{
    for (int i = 0; i < CFG_TIMER_NUM_OBSERVERS; i++) {
        if (timerEntry[timerNumber].observer[i] != NULL) {
            tickBuffer[bufferHead] = timerEntry[timerNumber].observer[i];
            bufferHead = (bufferHead + 1) % CFG_TIMER_BUFFER_SIZE;
//            Logger::debug("tickHandler->handle bufferHead=%d, bufferTail=%d, observer=%d", bufferHead, bufferTail, timerEntry[timerNumber].observer[i]);
        }
    }
}

/*
 * Interrupt function for Timer0
 */
void timer0Interrupt()
{
    tickHandler.handleInterrupt(0);
}
/*
 * Interrupt function for Timer1
 */
void timer1Interrupt()
{
    tickHandler.handleInterrupt(1);
}
/*
 * Interrupt function for Timer2
 */
void timer2Interrupt()
{
    tickHandler.handleInterrupt(2);
}
/*
 * Interrupt function for Timer3
 */
void timer3Interrupt()
{
    tickHandler.handleInterrupt(3);
}
/*
 * Interrupt function for Timer4
 */
void timer4Interrupt()
{
    tickHandler.handleInterrupt(4);
}
/*
 * Interrupt function for Timer5
 */
void timer5Interrupt()
{
    tickHandler.handleInterrupt(5);
}
/*
 * Interrupt function for Timer6
 */
void timer6Interrupt()
{
    tickHandler.handleInterrupt(6);
}
/*
 * Interrupt function for Timer7
 */
void timer7Interrupt()
{
    tickHandler.handleInterrupt(7);
}
/*
 * Interrupt function for Timer8
 */
void timer8Interrupt()
{
    tickHandler.handleInterrupt(8);
}

/*
 * Default implementation of the TickObserver method. Must be overwritten
 * by every sub-class.
 */
void TickObserver::handleTick()
{
    Logger::error("TickObserver does not implement handleTick()");
}
