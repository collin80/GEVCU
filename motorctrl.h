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
        signed int motorTemp; //temperature of motor in tenths of degree C
        signed int inverterTemp; //temperature of inverter in tenths deg C
        uint16_t requestedTorque; //in tenths of Nm
        uint16_t requestedRPM; //in RPM
        uint16_t actualTorque; //in tenths Nm
        uint16_t actualRPM; //in RPM
        uint16_t MaxTorque;	//maximum torque in 0.1 Nm
        uint16_t MaxRPM; //in RPM
};

#endif
