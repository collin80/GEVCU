/*
 * BrusaMotorController.h
 *
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

#ifndef BRUSAMOTORCONTROLLER_H_
#define BRUSAMOTORCONTROLLER_H_

#include <Arduino.h>
#include "config.h"
#include "MotorController.h"
#include "sys_io.h"
#include "TickHandler.h"
#include "CanHandler.h"

// CAN bus id's for frames sent to DMC5

#define CAN_ID_CONTROL 0x210 // send commands (DMC_CTRL)
#define CAN_ID_LIMIT 0x211 // send limitations (DMC_LIM)
#define CAN_ID_CONTROL_2 0x212 // send commands (DMC_CTRL2)

// CAN bus id's for frames received from DMC5

#define CAN_ID_STATUS 0x258 // receive status message (DMC_TRQS) 					01001011000
#define CAN_ID_ACTUAL_VALUES 0x259 // receive actual values (DMC_ACTV)				01001011001
#define CAN_ID_ERRORS 0x25a // receive error codes (DMC_ERR) 						01001011010
#define CAN_ID_TORQUE_LIMIT 0x268 // receive torque limit information (DMC_TRQS2) 	01001101000
#define CAN_MASK_1 0x7cc // mask for above id's										11111001100
#define CAN_MASKED_ID_1 0x248 // masked id for id's from 0x258 to 0x268				01001001000

#define CAN_ID_TEMP 0x458 // receive temperature information (DMC_TEMP)				10001011000
#define CAN_MASK_2 0x7ff // mask for above id's										11111111111
#define CAN_MASKED_ID_2 0x458 // masked id for id's from 0x258 to 0x268				10001011000

class BrusaMotorController: public MotorController, CanObserver {
public:
	enum OperationState {
		unknown,
	    start,
	    standby,
	    ready,
	    torque,
	    speed,
	    error
	};

	enum Warning {
		systemCheckActive = 0x0001,
		externalShutdownPathAw2Off = 0x0002,
		externalShutdownPathAw1Off = 0x0004,
		oscillationLimitControllerActive = 0x0008,
		driverShutdownPathActive = 0x0400,
		powerMismatchDetected = 0x0800,
		speedSensorSignal = 0x1000,
		hvUndervoltage = 0x2000,
		maximumModulationLimiter = 0x4000,
		temperatureSensor = 0x8000,

		generalLimitation, //TODO: define correct codes
		maxTorqueLimitation,
		slewRateLimitation,
		mechanicalPowerLimitation,
		speedLimitation,
		currentLimitation,
		voltageLimitation,
		motorTemperatureLimitation,
		temperatureLimitation,
		acCurrentLimitation,
		motorModelLimitation
	};
	
	enum Error {
		speedSensorSupply = 0x00000001,
		speedSensor = 0x00000002,
		canLimitMessageInvalid = 0x00000004,
		canControlMessageInvalid = 0x00000008,
		canLimitMessageLost = 0x00000010,
		overvoltageSkyConverter = 0x00000020,
		voltageMeasurement = 0x00000040,
		shortCircuit = 0x00000080,
		canControlMessageLost = 0x00000100,
		overtemp = 0x00000200,
		overtempMotor = 0x00000400,
		overspeed = 0x00000800,
		undervoltage = 0x00001000,
		overvoltage = 0x00002000,
		overcurrent = 0x00004000,
		initalisation = 0x00008000,
		analogInput = 0x00010000,
		driverShutdown = 0x00400000,
		powerMismatch = 0x00800000,
		canControl2MessageLost = 0x01000000,
		motorEeprom = 0x02000000,
		storage = 0x04000000,
		kl15Lost = 0x08000000,
		canCommunicationStartup = 0x10000000,
		internalSupply = 0x20000000,
		acOvercurrent = 0x40000000,
		osTrap = 0x80000000
	};
	
public:
	void handleTick();
	void handleCanFrame(RX_CAN_FRAME *frame);
	void setup();

	BrusaMotorController();
	Device::DeviceId getId();

private:
	OperationState operationState; //the op state we want
	OperationState actualState; //what the controller is reporting it is

	
	uint32_t torqueRequested;
	uint32_t torqueActual;
	uint32_t speedRequested;
	uint32_t speedActual;
	uint32_t voltageMinimal;
	uint32_t voltageMaximal;
	uint32_t voltageActual;
	uint32_t currentMinimal;
	uint32_t currentMaximal;
	uint32_t currentActual;
	
};



#endif /* BRUSAMOTORCONTROLLER_H_ */
