/*
 * device.h
 *
 * Created: 1/20/2013 10:14:51 PM
 *  Author: Collin
 */ 


#ifndef DEVICE_H_
#define DEVICE_H_

#include "MCP2515.h"

enum {
	DEVICE_ANY,
	DEVICE_MOTORCTRL,
	DEVICE_BMS,
	DEVICE_CHARGER	
};

class DEVICE {
	protected:
	MCP2515 * can;
	
	public:
	virtual void handleFrame(Frame& frame);
	virtual void handleTick();
	DEVICE(MCP2515 *canlib);	
};

#endif /* DEVICE_H_ */