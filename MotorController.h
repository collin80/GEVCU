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

class MotorController: public Device {

public:
	enum Gears {
		NEUTRAL = 0,
		DRIVE = 1,
		REVERSE = 2,
		ERROR = 3,
	};

	enum PowerMode {
		modeTorque,
		modeSpeed
	};

    enum OperationState {
		DISABLED = 0,
		STANDBY = 1,
		ENABLE = 2,
		POWERDOWN = 3
	};

    MotorController();
	DeviceType getType();
    void setup();
    void handleTick();
	uint32_t getTickInterval();

	void loadConfiguration();
	void saveConfiguration();

	void coolingcheck();
        void checkBrakeLight();
        void checkReverseLight();
        void checkEnableInput();
        void checkReverseInput();
        void checkPrecharge();

        void brakecheck();
	bool isReady();
	bool isRunning();
	bool isFaulted();
	bool isWarning();

uint32_t getStatusBitfield1();
uint32_t getStatusBitfield2();
uint32_t getStatusBitfield3();
uint32_t getStatusBitfield4();

uint32_t statusBitfield1; // bitfield variable for use of the specific implementation
uint32_t statusBitfield2;
uint32_t statusBitfield3;
uint32_t statusBitfield4;




	void setPowerMode(PowerMode mode);
          PowerMode getPowerMode();
        void setOpState(OperationState op) ;
          OperationState getOpState() ;
        void setSelectedGear(Gears gear);
          Gears getSelectedGear();
	
	int16_t getThrottle();
	int8_t getCoolFan();
    int8_t getCoolOn();
    int8_t getCoolOff();
    int8_t getBrakeLight();
    int8_t getRevLight();
    int8_t getEnableIn();
    int8_t getReverseIn();
    int16_t getselectedGear();
    int16_t getprechargeR();
    int16_t getnominalVolt();
    int8_t getprechargeRelay();
    int8_t getmainContactorRelay();
    int16_t getSpeedRequested();
    int16_t getSpeedActual();
    int16_t getTorqueRequested();
    int16_t getTorqueActual();
    int16_t getTorqueAvailable();
   
	uint16_t getDcVoltage();
	int16_t getDcCurrent();
	uint16_t getAcCurrent();
	uint32_t getKiloWattHours();
	int16_t getMechanicalPower();
	int16_t getTemperatureMotor();
	int16_t getTemperatureInverter();
	int16_t getTemperatureSystem();

	
        int milliseconds  ;
        int seconds;
        int minutes;
        int hours ;

	

protected:
	bool ready; // indicates if the controller is ready to enable the power stage
	bool running; // indicates if the power stage of the inverter is operative
	bool faulted; // indicates a error condition is present in the controller
	bool warning; // indicates a warning condition is present in the controller
	bool coolflag;
        bool testenableinput;
         bool testreverseinput;


	Gears selectedGear;

	PowerMode powerMode;
        OperationState operationState; //the op state we want
	
	int16_t throttleRequested; // -1000 to 1000 (per mille of throttle level)
	int16_t speedRequested; // in rpm
	int16_t speedActual; // in rpm
	int16_t torqueRequested; // in 0.1 Nm
	int16_t torqueActual; // in 0.1 Nm
	int16_t torqueAvailable; // the maximum available torque in 0.1Nm

	uint16_t dcVoltage; // DC voltage in 0.1 Volts
	int16_t dcCurrent; // DC current in 0.1 Amps
	uint16_t acCurrent; // AC current in 0.1 Amps
	uint32_t kiloWattHours;
	int16_t mechanicalPower; // mechanical power of the motor 0.1 kW
	int16_t temperatureMotor; // temperature of motor in 0.1 degree C
	int16_t temperatureInverter; // temperature of inverter power stage in 0.1 degree C
	int16_t temperatureSystem; // temperature of controller in 0.1 degree C

	
	uint16_t nominalVolts; //nominal pack voltage in 1/10 of a volt

	uint16_t prechargeTime; //time in ms that precharge should last
	uint32_t milliStamp; //how long we have precharged so far
	bool donePrecharge; //already completed the precharge cycle?
	bool prelay;
	uint32_t skipcounter;
};

class MotorControllerConfiguration : public DeviceConfiguration {
public:
	uint16_t speedMax; // in rpm
	uint16_t torqueMax;	// maximum torque in 0.1 Nm
	uint16_t torqueSlewRate; // for torque mode only: slew rate of torque value, 0=disabled, in 0.1Nm/sec
	uint16_t speedSlewRate; //  for speed mode only: slew rate of speed value, 0=disabled, in rpm/sec
	MotorController::PowerMode motorMode; //should we use torque or speed mode?
	uint8_t reversePercent;
	uint16_t kilowattHrs;
	uint16_t prechargeR; //resistance of precharge resistor in tenths of ohm
	uint16_t nominalVolt; //nominal pack voltage in tenths of a volt
	uint8_t prechargeRelay; //# of output to use for this relay or 255 if there is no relay
	uint8_t mainContactorRelay; //# of output to use for this relay or 255 if there is no relay
	uint8_t coolFan;
	uint8_t coolOn;
	uint8_t coolOff;
	uint8_t brakeLight;
	uint8_t revLight;
	uint8_t enableIn;
	uint8_t reverseIn;
};


#endif
