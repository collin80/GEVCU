/*
 * motorctrl.h
  *
 * Parent class for all motor controllers.
 *
 * Created: 2/04/2013
 *  Author: Collin
 */ 
 
 #ifndef MOTORCTRL_H_
#define MOTORCTRL_H_

#include "device.h"

class MOTORCTRL : public DEVICE {
	
	public:
	int getDeviceType();
	virtual int getDeviceID();
	int getThrottle();
	void setThrottle(int newthrottle);
	bool isRunning();
	bool isFaulted();
	
	MOTORCTRL(MCP2515 *canlib);
	
	protected:
	int requestedThrottle;
	bool running;
        bool faulted;
};

#endif
