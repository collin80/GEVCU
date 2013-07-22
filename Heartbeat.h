/*
 * Heartbeat.h
 *
 * Created: 1/6/2013 8:26:58 PM
 *  Author: Collin Kidder
 */

#ifndef HEARTBEAT_H_
#define HEARTBEAT_H_

#include "config.h"
#include "TickHandler.h"

class Heartbeat: public Tickable {
public:
	Heartbeat();
	void setup();
	void handleTick();

protected:

private:
	bool led;
};

#endif /* HEARTBEAT_H_ */
