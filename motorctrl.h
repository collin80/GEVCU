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

#define MOTORCTL_INPUT_DRIVE_EN    3
#define MOTORCTL_INPUT_FORWARD     4
#define MOTORCTL_INPUT_REVERSE     5
#define MOTORCTL_INPUT_LIMP        6

class MOTORCTRL : public DEVICE {
	
	public:
	DEVICE::DEVTYPE getDeviceType();
	virtual DEVICE::DEVID getDeviceID();
        virtual void setupDevice();
       	virtual void handleTick();
	int getThrottle();
	void setThrottle(int newthrottle);
	bool isRunning();
	bool isFaulted();

        enum GEARSWITCH {
            GS_NEUTRAL,
            GS_FORWARD,
            GS_REVERSE,
            GS_FAULT
        };
        
        MOTORCTRL(CANRaw *canlib);
	
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
        GEARSWITCH GearSwitch;
};

#endif
