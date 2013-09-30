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

#define CAN_ID_CONTROL		0x210 // send commands (DMC_CTRL)
#define CAN_ID_LIMIT		0x211 // send limitations (DMC_LIM)
#define CAN_ID_CONTROL_2	0x212 // send commands (DMC_CTRL2)

// CAN bus id's for frames received from DMC5

#define CAN_ID_STATUS			0x258 // receive (limit) status message (DMC_TRQS)	 	01001011000
#define CAN_ID_ACTUAL_VALUES	0x259 // receive actual values (DMC_ACTV)				01001011001
#define CAN_ID_ERRORS			0x25a // receive error codes (DMC_ERR) 					01001011010
#define CAN_ID_TORQUE_LIMIT		0x268 // receive torque limit information (DMC_TRQS2) 	01001101000
#define CAN_MASK_1				0x7cc // mask for above id's							11111001100
#define CAN_MASKED_ID_1			0x248 // masked id for id's from 0x258 to 0x268			01001001000

#define CAN_ID_TEMP			0x458 // receive temperature information (DMC_TEMP)			10001011000
#define CAN_MASK_2			0x7ff // mask for above id's								11111111111
#define CAN_MASKED_ID_2		0x458 // masked id for id's from 0x258 to 0x268				10001011000

class BrusaMotorController: public MotorController, CanObserver {
public:
	// Message id=0x258, DMC_TRQS
	// The value is composed of 2 bytes: (data[1] << 0) | (data[0] << 8)
	enum Status {
		motorModelLimitation		= 1 << 0,  // 0x0001
		mechanicalPowerLimitation	= 1 << 1,  // 0x0002
		maxTorqueLimitation			= 1 << 2,  // 0x0004
		acCurrentLimitation			= 1 << 3,  // 0x0008
		temperatureLimitation		= 1 << 4,  // 0x0010
		speedLimitation				= 1 << 5,  // 0x0020
		voltageLimitation			= 1 << 6,  // 0x0040
		currentLimitation			= 1 << 7,  // 0x0080
		torqueLimitation			= 1 << 8,  // 0x0100
		errorFlag					= 1 << 9,  // 0x0200, see DMC_ERR
		warningFlag					= 1 << 10, // 0x0400, see DMC_ERR
		slewRateLimitation			= 1 << 12, // 0x1000
		motorTemperatureLimitation	= 1 << 13, // 0x2000
		stateRunning				= 1 << 14, // 0x4000
		stateReady					= 1 << 15  // 0x8000
	};

	// Message id=0x25a, DMC_ERR
	// The value is composed of 2 bytes: (data[7] << 0) | (data[6] << 8)
	enum Warning {
		systemCheckActive					= 1 << 0,  // 0x0001
		externalShutdownPathAw2Off			= 1 << 1,  // 0x0002
		externalShutdownPathAw1Off			= 1 << 2,  // 0x0004
		oscillationLimitControllerActive	= 1 << 3,  // 0x0008
		driverShutdownPathActive			= 1 << 10, // 0x0400
		powerMismatchDetected				= 1 << 11, // 0x0800
		speedSensorSignal					= 1 << 12, // 0x1000
		hvUndervoltage						= 1 << 13, // 0x2000
		maximumModulationLimiter			= 1 << 14, // 0x4000
		temperatureSensor					= 1 << 15, // 0x8000
	};

	// Message id=0x25a, DMC_ERR
	// The error value is composed of 4 bytes : (data[1] << 0) | (data[0] << 8) | (data[5] << 16) | (data[4] << 24)
	enum Error {
		speedSensorSupply			= 1 << 0,  // 0x00000001, data[1]
		speedSensor					= 1 << 1,  // 0x00000002, data[1]
		canLimitMessageInvalid		= 1 << 2,  // 0x00000004, data[1]
		canControlMessageInvalid	= 1 << 3,  // 0x00000008, data[1]
		canLimitMessageLost			= 1 << 4,  // 0x00000010, data[1]
		overvoltageSkyConverter		= 1 << 5,  // 0x00000020, data[1]
		voltageMeasurement			= 1 << 6,  // 0x00000040, data[1]
		shortCircuit				= 1 << 7,  // 0x00000080, data[1]
		canControlMessageLost		= 1 << 8,  // 0x00000100, data[0]
		overtemp					= 1 << 9,  // 0x00000200, data[0]
		overtempMotor				= 1 << 10, // 0x00000400, data[0]
		overspeed					= 1 << 11, // 0x00000800, data[0]
		undervoltage				= 1 << 12, // 0x00001000, data[0]
		overvoltage					= 1 << 13, // 0x00002000, data[0]
		overcurrent					= 1 << 14, // 0x00004000, data[0]
		initalisation				= 1 << 15, // 0x00008000, data[0]
		analogInput					= 1 << 16, // 0x00010000, data[5]
		driverShutdown				= 1 << 22, // 0x00400000, data[5]
		powerMismatch				= 1 << 23, // 0x00800000, data[5]
		canControl2MessageLost		= 1 << 24, // 0x01000000, data[4]
		motorEeprom					= 1 << 25, // 0x02000000, data[4]
		storage						= 1 << 26, // 0x04000000, data[4]
		enablePinSignalLost			= 1 << 27, // 0x08000000, data[4]
		canCommunicationStartup		= 1 << 28, // 0x10000000, data[4]
		internalSupply				= 1 << 29, // 0x20000000, data[4]
		acOvercurrent				= 1 << 30, // 0x40000000, data[4]
		osTrap						= 1 << 31  // 0x80000000, data[4]
	};
	
	// Message id=0x210, DMC_CTRL
	// The value is composed of 1 byte : data[0]
	enum Control {
		enablePositiveTorqueSpeed	= 1 << 0, // 0x01
		enableNegativeTorqueSpeed	= 1 << 1, // 0x02
		clearErrorLatch				= 1 << 3, // 0x08
		enableOscillationLimiter	= 1 << 5, // 0x20
		enableSpeedMode				= 1 << 6, // 0x40
		enablePowerStage			= 1 << 7  // 0x80
	};

	void handleTick();
	void handleCanFrame(RX_CAN_FRAME *frame);
	void setup();
	BrusaMotorController();
	DeviceId getId();

private:
	// DMC_TRQS
//	boolean dmcReady; // indicates that the controller is ready, meaning no error is present and HV is available
//	boolean dmcRunning; // indicates that the controller's powerstage is operating
//	boolean dmcError; // indicates the presence of an error (refer to errorBitField, reported in separate message), enabling powerstage is not possible
//	boolean dmcWarning; // indicates the presence of a warning (refer to warningBitField, reported in separate message)
//	int16_t torqueAvailable; // the maximum available torque in 0.01Nm -> divide by 100 to get Nm
//	int16_t torqueActual; // the actual torque in 0.01Nm -> divide by 100 to get Nm
//	int16_t speedActual; // the actual speed of the motor in rpm

	// DMC_ACTV
//	uint16_t dcVoltage; // DC voltage in 0.1 Volts -> divide value by 10 to get Volts
//	int16_t dcCurrent; // DC current in 0.1 Amps -> divide value by 10 to get Amps
//	uint16_t acCurrent; // AC current in 0.25 Amps -> divide value by 4 to get Amps
//	int16_t mechanicalPower; // mechanical power of the motor in 0.016 kW -> divide value by 62.5 to get kW

	// DMC_TEMP
//	int16_t temperatureInverter; // the temperature of the controller powerstage in 0.5�C -> divide by 2 to get �C
//	int16_t temperatureMotor; // the motor temperature in 0.5�C
//	uint8_t temperatureSystem; // the temperature of the controller system in �C with offset of +50�C -> subtract 50�C

	// DMC_ERR
	uint32_t errorBitField; // bits holding the errors defined in enum Error
	uint16_t warningBitField; // bits holding the warnings defined in enum Warning
	uint16_t statusBitField; // bits holding the status flags defined in enum Status

	// DMC_TRQS2
	int16_t maxPositiveTorque; // max positive available torque in 0.01Nm -> divide by 100 to get Nm
	int16_t minNegativeTorque; // minimum negative available torque in 0.01Nm
	uint8_t limiterStateNumber; // state number of active limiter


	int tickCounter; // count how many times handleTick() was called
	PowerMode powerMode; // the desired power mode
	uint8_t controlBitField; // the control bit field to send via DMC_CTRL in data[0]
	TX_CAN_FRAME outputFrame; // the output CAN frame;

	void sendControl();
	void sendControl2();
	void sendLimits();
	void prepareOutputFrame(uint32_t);
};

#endif /* BRUSAMOTORCONTROLLER_H_ */
