/*
 * MotorController.h
  *
 * Parent class for all motor controllers.
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
 
#ifndef MOTORCTRL_H_
#define MOTORCTRL_H_

#include <Arduino.h>
#include "config.h"
#include "Device.h"
#include "Throttle.h"
#include "DeviceManager.h"
#include "sys_io.h"

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
	DeviceType getType();
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
	void setMaxRpm(uint16_t maxRPM);
	void setMaxTorque(uint16_t maxTorque);
	void setPrechargeC(uint16_t c);
	void setPrechargeR(uint16_t r);
	void setNominalV(uint16_t v);
	void setPrechargeRelay(uint8_t relay);
	void setMainRelay(uint8_t relay);
	void setReversePercent(uint8_t perc);
	signed int getMotorTemp();
	uint16_t getRequestedRpm();
	uint16_t getRequestedTorque();
	void saveEEPROM();

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
	uint16_t prechargeC; //capacitance of motor controller input in uf
	uint16_t prechargeR; //resistance of precharge resistor in tenths of ohm
	uint16_t prechargeTime; //time in ms that precharge should last
	uint16_t prechargeSoFar; //how long we have precharged so far
	uint16_t nominalVolt; //nominal pack voltage in tenths of a volt
	uint8_t prechargeRelay; //# of output to use for this relay or 255 if there is no relay
	uint8_t mainContactorRelay; //# of output to use for this relay or 255 if there is no relay
	uint8_t reversePercent;
	bool donePrecharge; //already completed the precharge cycle?
};

#endif
