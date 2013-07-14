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

#include "device.h"
#include "config.h"

class TickHandler {

private:
	static Device *tickDevice[CFG_MAX_TICK_DEVICES]; // array which holds the registered TickDevices
	static int findTimer(Device *);

protected:

public:
	TickHandler();
	static void registerDevice(Device *, uint32_t);
	static void unregisterDevice(Device *);
	static void handleInterrupt(int);
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
