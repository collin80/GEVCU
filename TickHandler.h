/*
 * TickHandler.h
 *
 * Observer class where devices can register to be triggered
 * on a certain interval.
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#ifndef TICKHANDLER_H_
#define TICKHANDLER_H_

#include "config.h"
#include "device.h"

#define NUM_TIMERS 9

class TickHandler {
public:
	static void initialize();
	static void registerDevice(Device *device, uint32_t interval);
	static void unregisterDevice(Device *device);
	static void handleInterrupt(int timerNumber); // must be public when from the non-class functions

protected:

private:
	struct TimerEntry {
		long interval; // interval of timer
		Device *device[CFG_MAX_DEVICES]; // array of pointers to devices with this interval
	};
	static TimerEntry timerEntry[NUM_TIMERS]; // array of timer entries (9 as there are 9 timers)
	static int findTimer(long interval);
	static int findDevice(int timerNumber, Device *device);
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
