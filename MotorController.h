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

#include <Arduino.h>
#include "config.h"
#include "device.h"
#include "Throttle.h"
#include "DeviceManager.h"

#define MOTORCTL_INPUT_DRIVE_EN    3
#define MOTORCTL_INPUT_FORWARD     4
#define MOTORCTL_INPUT_REVERSE     5
#define MOTORCTL_INPUT_LIMP        6

class MotorController : public Device {
	
	public:
    enum GearSwitch {
        GS_NEUTRAL,
        GS_FORWARD,
        GS_REVERSE,
        GS_FAULT
    };
    MotorController();
	Device::DeviceType getType();
    virtual void setup();
    void handleTick();
	int getThrottle();
	void setThrottle(int newthrottle);
	bool isRunning();
	bool isFaulted();
	uint16_t getActualRpm();
	uint16_t getActualTorque();
	GearSwitch getGearSwitch();
	signed int getInverterTemp();
	uint16_t getMaxRpm();
	uint16_t getMaxTorque();
	signed int getMotorTemp();
	uint16_t getRequestedRpm();
	uint16_t getRequestedTorque();

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
    uint16_t maxTorque;	//maximum torque in 0.1 Nm
    uint16_t maxRPM; //in RPM
    GearSwitch gearSwitch;
};

#endif
