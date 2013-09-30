/*
 * BrusaMotorController.cpp
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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
#include "BrusaMotorController.h"

/*
 Warning:
 At high speed disable the DMC_EnableRq can be dangerous because a field weakening current is
 needed to achieve zero torque. Switching off the DMC in such a situation will make heavy regenerat-
 ing torque that can't be controlled.
 */

BrusaMotorController::BrusaMotorController() : MotorController() {
	torqueAvailable = 0;
	maxPositiveTorque = 0;
	minNegativeTorque = 0;

	errorBitField = 0;
	warningBitField = 0;
	statusBitField = 0;
	limiterStateNumber = 0;

	tickCounter = 0;

	torqueMax = 20; // TODO: only for testing, in tenths Nm, so 2Nm max torque, remove for production use
	speedMax = 2000; // TODO: only for testing, remove for production use
}

void BrusaMotorController::setup() {
	TickHandler::getInstance()->detach(this);
	MotorController::setup(); // run the parent class version of this function

	// register ourselves as observer of 0x258-0x268 and 0x458 can frames
	CanHandler::getInstanceEV()->attach(this, CAN_MASKED_ID_1, CAN_MASK_1, false);
	CanHandler::getInstanceEV()->attach(this, CAN_MASKED_ID_2, CAN_MASK_2, false);

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA);
}

void BrusaMotorController::handleTick() {
	tickCounter++;

	sendControl();	// send CTRL every 20ms : 20 00 2E E0 00 00 00 00
	if (tickCounter > 4) {
		sendControl2();	// send CTRL_2 every 100ms : 00 00 00 00 00 00 00 00
		sendLimits();	// send LIMIT every 100ms : 0D 70 11 C6 00 00 00 00
		tickCounter = 0;
	}
}

void BrusaMotorController::sendControl() {
	prepareOutputFrame(CAN_ID_CONTROL);


	//TODO: remove ramp testing
	torqueRequested = 20;
	if (speedActual < 100)
		speedRequested = 1000;
	if (speedActual > 950)
		speedRequested = 50;



	outputFrame.data[0] = enablePositiveTorqueSpeed | enableNegativeTorqueSpeed;
	if (faulted) {
		outputFrame.data[0] |= clearErrorLatch;
	} else {
		if (running || speedActual > 1000) { // see warning about field weakening current to prevent uncontrollable regen
			outputFrame.data[0] |= enablePowerStage;
			if (running) {
				// outputFrame.data[0] |= enableOscillationLimiter;
				if (powerMode == modeSpeed)
					outputFrame.data[0] |= enableSpeedMode;

				// TODO: differ between torque/speed mode
				// TODO: check for maxRPM and maxTorque

				// set the speed in rpm
				outputFrame.data[2] = (speedRequested & 0xFF00) >> 8;
				outputFrame.data[3] = (speedRequested & 0x00FF);

				// set the torque in 0.01Nm (GEVCU uses 0.1Nm -> multiply by 10)
				outputFrame.data[4] = ((torqueRequested * 10) & 0xFF00) >> 8;
				outputFrame.data[5] = ((torqueRequested * 10) & 0x00FF);
			}
		}
	}

	CanHandler::getInstanceEV()->sendFrame(outputFrame);
}

void BrusaMotorController::sendControl2() {
	//TODO: move variables + initialization
	uint16_t torqueSlewRate = 0; // for torque mode only: slew rate of torque value, 0=disabled, in 0.01Nm/sec
	uint16_t speedSlewRate = 0; //  for speed mode only: slew rate of speed value, 0=disabled, in rpm/sec
	uint16_t maxMechanicalPowerMotor = 50000; // maximal mechanical power of motor in 4W steps
	uint16_t maxMechanicalPowerRegen = 50000; // maximal mechanical power of regen in 4W steps

	prepareOutputFrame(CAN_ID_CONTROL_2);
	outputFrame.data[0] = (torqueSlewRate & 0xFF00) >> 8;
	outputFrame.data[1] = (torqueSlewRate & 0x00FF);
	outputFrame.data[2] = (speedSlewRate & 0xFF00) >> 8;
	outputFrame.data[3] = (speedSlewRate & 0x00FF);
	outputFrame.data[4] = (maxMechanicalPowerMotor & 0xFF00) >> 8;
	outputFrame.data[5] = (maxMechanicalPowerMotor & 0x00FF);
	outputFrame.data[6] = (maxMechanicalPowerRegen & 0xFF00) >> 8;
	outputFrame.data[7] = (maxMechanicalPowerRegen & 0x00FF);

	CanHandler::getInstanceEV()->sendFrame(outputFrame);
}

void BrusaMotorController::sendLimits() {
	//TODO: move variables + initialization
	uint16_t dcVoltLimitMotor = 1000; // minimum DC voltage limit for motoring in 0.1V
	uint16_t dcVoltLimitRegen = 1000; //  maximum DC voltage limit for regen in 0.1V
	uint16_t dcCurrentLimitMotor = 0; // current limit for motoring in 0.1A
	uint16_t dcCurrentLimitRegen = 0; // current limit for regen in 0.1A

	prepareOutputFrame(CAN_ID_LIMIT);
	outputFrame.data[0] = (dcVoltLimitMotor & 0xFF00) >> 8;
	outputFrame.data[1] = (dcVoltLimitMotor & 0x00FF);
	outputFrame.data[2] = (dcVoltLimitRegen & 0xFF00) >> 8;
	outputFrame.data[3] = (dcVoltLimitRegen & 0x00FF);
	outputFrame.data[4] = (dcCurrentLimitMotor & 0xFF00) >> 8;
	outputFrame.data[5] = (dcCurrentLimitMotor & 0x00FF);
	outputFrame.data[6] = (dcCurrentLimitRegen & 0xFF00) >> 8;
	outputFrame.data[7] = (dcCurrentLimitRegen & 0x00FF);

	CanHandler::getInstanceEV()->sendFrame(outputFrame);
}

void BrusaMotorController::prepareOutputFrame(uint32_t id) {
	outputFrame.dlc = 8;
	outputFrame.id = id;
	outputFrame.ide = 0;
	outputFrame.rtr = 0;

	outputFrame.data[1] = 0;
	outputFrame.data[2] = 0;
	outputFrame.data[3] = 0;
	outputFrame.data[4] = 0;
	outputFrame.data[5] = 0;
	outputFrame.data[6] = 0;
	outputFrame.data[7] = 0;
}

void BrusaMotorController::handleCanFrame(RX_CAN_FRAME* frame) {
	switch (frame->id) {
	case CAN_ID_STATUS:
		statusBitField = frame->data[1] | (frame->data[0] << 8);
		torqueAvailable = (frame->data[3] | (frame->data[2] << 8)) / 10;
		torqueActual = (frame->data[5] | (frame->data[4] << 8)) / 10;
		speedActual = frame->data[7] | (frame->data[6] << 8);

		if(Logger::isDebug())
			Logger::debug(BRUSA_DMC5, "status: %X, torque avail: %fNm, actual torque: %fNm, speed actual: %drpm", statusBitField, (float)torqueAvailable/100.0F, (float)torqueActual/100.0F, speedActual);

		ready = (statusBitField & stateReady) != 0 ? true : false;
		if (ready)
			Logger::info(BRUSA_DMC5, "ready");

		running = (statusBitField & stateRunning) != 0 ? true : false;
		if (running)
			Logger::info(BRUSA_DMC5, "running");

		faulted = (statusBitField & errorFlag) != 0 ? true : false;
		if (faulted)
			Logger::error(BRUSA_DMC5, "error is present, see error message");

		warning = (statusBitField & warningFlag) != 0 ? true : false;
		if (warning)
			Logger::warn(BRUSA_DMC5, "warning is present, see warning message");

		if (statusBitField & motorModelLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by motor model");
		if (statusBitField & mechanicalPowerLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by mechanical power");
		if (statusBitField & maxTorqueLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by max torque");
		if (statusBitField & acCurrentLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by AC current");
		if (statusBitField & temperatureLimitation)
			Logger::warn(BRUSA_DMC5, "torque limit by temperature");
		if (statusBitField & speedLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by speed");
		if (statusBitField & voltageLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by DC voltage");
		if (statusBitField & currentLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by DC current");
		if (statusBitField & torqueLimitation)
			Logger::info(BRUSA_DMC5, "torque limitation is active");
		if (statusBitField & slewRateLimitation)
			Logger::info(BRUSA_DMC5, "torque limit by slew rate");
		if (statusBitField & motorTemperatureLimitation)
			Logger::warn(BRUSA_DMC5, "torque limit by motor temperature");
		break;

	case CAN_ID_ACTUAL_VALUES:
		dcVoltage = frame->data[1] | (frame->data[0] << 8);
		dcCurrent = frame->data[3] | (frame->data[2] << 8);
		acCurrent = (frame->data[5] | (frame->data[4] << 8)) / 2.5;
		mechanicalPower = (frame->data[7] | (frame->data[6] << 8)) / 6.25;

		if (Logger::isDebug())
			Logger::debug(BRUSA_DMC5, "actual values: DC Volts: %fV, DC current: %fA, AC current: %fA, mechPower: %fkW", (float)dcVoltage / 10.0F, (float)dcCurrent / 10.0F, (float)acCurrent / 10.0F, (float)mechanicalPower / 10.0F);
		break;

	case CAN_ID_ERRORS:
		errorBitField = frame->data[1] | (frame->data[0] << 8) | (frame->data[5] << 16) | (frame->data[4] << 24);
		warningBitField = frame->data[7] | (frame->data[6] << 8);

		if(Logger::isDebug())
			Logger::debug(BRUSA_DMC5, "errors: %X, warning: %X", errorBitField, warningBitField);

		//TODO: DMC_CompatibilityWarnings not evaluated at this point. check if needed

		// errors
		if (errorBitField & speedSensorSupply)
			Logger::error(BRUSA_DMC5, "speed sensor supply");
		if (errorBitField & speedSensor)
			Logger::error(BRUSA_DMC5, "speed sensor");
		if (errorBitField & canLimitMessageInvalid)
			Logger::error(BRUSA_DMC5, "can limit message invalid");
		if (errorBitField & canControlMessageInvalid)
			Logger::error(BRUSA_DMC5, "can control message invalid");
		if (errorBitField & canLimitMessageLost)
			Logger::error(BRUSA_DMC5, "can limit message lost");
		if (errorBitField & overvoltageSkyConverter)
			Logger::error(BRUSA_DMC5, "overvoltage sky converter");
		if (errorBitField & voltageMeasurement)
			Logger::error(BRUSA_DMC5, "voltage measurement");
		if (errorBitField & shortCircuit)
			Logger::error(BRUSA_DMC5, "short circuit");
		if (errorBitField & canControlMessageLost)
			Logger::error(BRUSA_DMC5, "can control message lost");
		if (errorBitField & overtemp)
			Logger::error(BRUSA_DMC5, "overtemp");
		if (errorBitField & overtempMotor)
			Logger::error(BRUSA_DMC5, "overtemp motor");
		if (errorBitField & overspeed)
			Logger::error(BRUSA_DMC5, "overspeed");
		if (errorBitField & undervoltage)
			Logger::error(BRUSA_DMC5, "undervoltage");
		if (errorBitField & overvoltage)
			Logger::error(BRUSA_DMC5, "overvoltage");
		if (errorBitField & overcurrent)
			Logger::error(BRUSA_DMC5, "overcurrent");
		if (errorBitField & initalisation)
			Logger::error(BRUSA_DMC5, "initalisation");
		if (errorBitField & analogInput)
			Logger::error(BRUSA_DMC5, "analogInput");
		if (errorBitField & driverShutdown)
			Logger::error(BRUSA_DMC5, "driver shutdown");
		if (errorBitField & powerMismatch)
			Logger::error(BRUSA_DMC5, "power mismatch");
		if (errorBitField & canControl2MessageLost)
			Logger::error(BRUSA_DMC5, "can Control2 message lost");
		if (errorBitField & motorEeprom)
			Logger::error(BRUSA_DMC5, "motor Eeprom");
		if (errorBitField & storage)
			Logger::error(BRUSA_DMC5, "storage");
		if (errorBitField & enablePinSignalLost)
			Logger::error(BRUSA_DMC5, "lost signal on enable pin");
		if (errorBitField & canCommunicationStartup)
			Logger::error(BRUSA_DMC5, "can communication startup");
		if (errorBitField & internalSupply)
			Logger::error(BRUSA_DMC5, "internal supply");
		if (errorBitField & acOvercurrent)
			Logger::error(BRUSA_DMC5, "AC Overcurrent");
		if (errorBitField & osTrap)
			Logger::error(BRUSA_DMC5, "OS trap");

		// warnings
		if (warningBitField & systemCheckActive)
			Logger::warn(BRUSA_DMC5, "system check active");
		if (warningBitField & externalShutdownPathAw2Off)
			Logger::warn(BRUSA_DMC5, "external shutdown path Aw2 off");
		if (warningBitField & externalShutdownPathAw1Off)
			Logger::warn(BRUSA_DMC5, "external shutdown path Aw1 off");
		if (warningBitField & oscillationLimitControllerActive)
			Logger::warn(BRUSA_DMC5, "oscillation limit controller active");
		if (warningBitField & driverShutdownPathActive)
			Logger::warn(BRUSA_DMC5, "driver shutdown path active");
		if (warningBitField & powerMismatchDetected)
			Logger::warn(BRUSA_DMC5, "power mismatch detected");
		if (warningBitField & speedSensorSignal)
			Logger::warn(BRUSA_DMC5, "speed sensor signal");
		if (warningBitField & hvUndervoltage)
			Logger::warn(BRUSA_DMC5, "HV undervoltage");
		if (warningBitField & maximumModulationLimiter)
			Logger::warn(BRUSA_DMC5, "maximum modulation limiter");
		if (warningBitField & temperatureSensor)
			Logger::warn(BRUSA_DMC5, "temperature sensor");
		break;

	case CAN_ID_TORQUE_LIMIT:
		maxPositiveTorque = (frame->data[1] | (frame->data[0] << 8)) / 10;
		minNegativeTorque = (frame->data[3] | (frame->data[2] << 8)) / 10;
		limiterStateNumber = frame->data[4];

		if (Logger::isDebug())
			Logger::debug(BRUSA_DMC5, "torque limit: max positive: %fNm, min negative: %fNm", (float) maxPositiveTorque / 10.0F, (float) minNegativeTorque / 10.0F, limiterStateNumber);
		break;

	case CAN_ID_TEMP:
		temperatureInverter = (frame->data[1] | (frame->data[0] << 8)) * 5;
		temperatureMotor = (frame->data[3] | (frame->data[2] << 8)) * 5;
		temperatureSystem = (frame->data[4] - 50) * 10;

		if (Logger::isDebug())
			Logger::debug(BRUSA_DMC5, "temperature: inverter: %fC, motor: %fC, system: %fC", (float)temperatureInverter / 10.0F, (float)temperatureMotor / 10.0F, (float)temperatureSystem / 10.0F);
		break;

	default:
		Logger::debug(BRUSA_DMC5, "received unknown frame id %X", frame->id);
	}
}

DeviceId BrusaMotorController::getId() {
	return BRUSA_DMC5;
}

#endif
