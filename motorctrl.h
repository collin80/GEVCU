/*
 * motorctrl.h
  *
 * Parent class for all motor controllers.
 *
 * Created: 2/04/2013
 *  Author: Collin Kidder
 */ 
 
 #ifndef MOTORCTRL_H_
#define MOTORCTRL_H_

#include "device.h"

class MOTORCTRL : public DEVICE {
	
	public:
	DEVICE::DEVTYPE getDeviceType();
	virtual DEVICE::DEVID getDeviceID();
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
