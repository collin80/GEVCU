/*
 * ichip_2128.cpp
 *
 * Class to interface with the ichip 2128 based wifi adapter we're using on our board
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

#include "ichip_2128.h"

/*
 * Initialization of hardware and parameters
 */
void ICHIPWIFI::setup() {

	prefsHandler = new PrefHandler(ICHIP2128);

	TickHandler::getInstance()->detach(this);

	tickCounter = 0;
	ibWritePtr = 0;
	serialInterface->begin(115200);

	paramCache.brakeNotAvailable = true;

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 */
void ICHIPWIFI::sendCmd(String cmd) {
	serialInterface->write(Constants::ichipCommandPrefix);
	serialInterface->print(cmd);
	serialInterface->write(13);
	loop(); // parse the response
}

/*
 * Periodic updates of parameters to ichip RAM.
 * Also query for changed parameters of the config page.
 */
void ICHIPWIFI::handleTick() {
	MotorController* motorController = DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	uint32_t ms = millis();
	tickCounter++;

	// Do a delayed parameter load once about a second after startup
	if ( ms > 1000 && ms < 1500) {
		loadParameters();
	}

	// make small slices so the main loop is not blocked for too long
	if (tickCounter == 1) {
		if (motorController) {

			// just update this every second or so
			if ( ms > paramCache.timeRunning + 60000 ) {
				paramCache.timeRunning = ms;
				setParam(Constants::timeRunning, getTimeRunning());
			}
			if ( paramCache.torqueRequested != motorController->getTorqueRequested() ) {
				paramCache.torqueRequested = motorController->getTorqueRequested();
				paramCache.torqueRequested -= 30000;
				setParam(Constants::torqueRequested, paramCache.torqueRequested / 10.0f, 1);
			}
			if ( paramCache.torqueActual != motorController->getTorqueActual() ) {
				paramCache.torqueActual = motorController->getTorqueActual();
				setParam(Constants::torqueActual, paramCache.torqueActual / 10.0f, 1);
			}
		}
		if (accelerator) {
			if ( paramCache.throttle != accelerator->getLevel() ) {
				paramCache.throttle = accelerator->getLevel();
				setParam(Constants::throttle, paramCache.throttle / 10.0f, 1);
			}
		}
		if (brake) {
			if ( paramCache.brake != brake->getLevel() ) {
				paramCache.brake = brake->getLevel();
				paramCache.brakeNotAvailable = false;
				setParam(Constants::brake, paramCache.brake / 10.0f, 1);
			}
		} else {
			if ( paramCache.brakeNotAvailable == true ) {
				paramCache.brakeNotAvailable = false; // no need to keep sending this
				setParam(Constants::brake, Constants::notAvailable);
			}
		}
	} else if (tickCounter == 2) {
		if (motorController) {
			if ( paramCache.speedRequested != motorController->getSpeedRequested() ) {
				paramCache.speedRequested = motorController->getSpeedRequested();
				setParam(Constants::speedRequested, paramCache.speedRequested);
			}
			if ( paramCache.speedActual != motorController->getSpeedActual() ) {
				paramCache.speedActual = motorController->getSpeedActual();
				setParam(Constants::speedActual, paramCache.speedActual);
			}
			if ( paramCache.dcVoltage != motorController->getDcVoltage() ) {
				paramCache.dcVoltage = motorController->getDcVoltage();
				setParam(Constants::dcVoltage, paramCache.dcVoltage / 10.0f, 1);
			}
			if ( paramCache.dcCurrent != motorController->getDcCurrent() ) {
				paramCache.dcCurrent = motorController->getDcCurrent();
				setParam(Constants::dcCurrent, paramCache.dcCurrent / 10.0f, 1);
			}
		}
	} else if (tickCounter == 3) {
		if (motorController) {
			if ( paramCache.acCurrent != motorController->getAcCurrent() ) {
				paramCache.acCurrent = motorController->getAcCurrent();
				setParam(Constants::acCurrent, paramCache.acCurrent / 10.0f, 1);
			}
			if ( paramCache.bitfield1 != motorController->getStatusBitfield1() ) {
				paramCache.bitfield1 = motorController->getStatusBitfield1();
				setParam(Constants::bitfield1, paramCache.bitfield1);
			}
			if ( paramCache.bitfield2 != motorController->getStatusBitfield2() ) {
				paramCache.bitfield2 = motorController->getStatusBitfield2();
				setParam(Constants::bitfield2, paramCache.bitfield2);
			}
			if ( paramCache.bitfield3 != motorController->getStatusBitfield3() ) {
				paramCache.bitfield3 = motorController->getStatusBitfield3();
				setParam(Constants::bitfield3, paramCache.bitfield3);
			}
			if ( paramCache.bitfield4 != motorController->getStatusBitfield4() ) {
				paramCache.bitfield4 = motorController->getStatusBitfield4();
				setParam(Constants::bitfield4, paramCache.bitfield4);
			}
		}
	} else if (tickCounter == 4) {
		if (motorController) {
			if ( paramCache.running != motorController->isRunning() ) {
				paramCache.running = motorController->isRunning();
				setParam(Constants::running, (paramCache.running ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.faulted != motorController->isFaulted() ) {
				paramCache.faulted = motorController->isFaulted();
				setParam(Constants::faulted, (paramCache.faulted ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.warning != motorController->isWarning() ) {
				paramCache.warning = motorController->isWarning();
				setParam(Constants::warning, (paramCache.warning ? Constants::trueStr : Constants::falseStr));
			}
			if ( paramCache.gear != motorController->getGearSwitch() ) {
				paramCache.gear = motorController->getGearSwitch();
				setParam(Constants::gear, (uint16_t) paramCache.gear);
			}
		}
	} else if (tickCounter > 4) {
		if (motorController) {
			if ( paramCache.tempMotor != motorController->getTemperatureMotor() ) {
				paramCache.tempMotor = motorController->getTemperatureMotor();
				setParam(Constants::tempMotor, paramCache.tempMotor / 10.0f, 1);
			}
			if ( paramCache.tempInverter != motorController->getTemperatureInverter() ) {
				paramCache.tempInverter = motorController->getTemperatureInverter();
				setParam(Constants::tempInverter, paramCache.tempInverter / 10.0f, 1);
			}
			if ( paramCache.tempSystem != motorController->getTemperatureSystem() ) {
				paramCache.tempSystem = motorController->getTemperatureSystem();
				setParam(Constants::tempSystem, paramCache.tempSystem / 10.0f, 1);
			}
			//if ( paramCache.mechPower != motorController->getMechanicalPower() ) {
				paramCache.mechPower = motorController->getMechanicalPower();
				setParam(Constants::mechPower, paramCache.mechPower / 10.0f, 1);
			//}
		}
		tickCounter = 0;
		getNextParam();
	}
}

/*
 * Calculate the runtime in hh:mm:ss
 */
char *ICHIPWIFI::getTimeRunning() {
	uint32_t ms = millis();
	int seconds = (int) (ms / 1000) % 60;
	int minutes = (int) ((ms / (1000 * 60)) % 60);
	int hours = (int) ((ms / (1000 * 3600)) % 24);
	sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
	return buffer;
}

/*
 * Handle a message sent by the DeviceManager.
 * Currently MSG_SET_PARAM is supported. A array of two char * has to be included
 * in the message.
 */
void ICHIPWIFI::handleMessage(uint32_t messageType, void* message) {
	Device::handleMessage(messageType, message);

	switch (messageType) {
	case MSG_SET_PARAM: {
		char **params = (char **)message;
		setParam((char *)params[0], (char *)params[1]);
		break;
	}
	case MSG_CONFIG_CHANGE:
		loadParameters();
		break;
	case MSG_COMMAND:
		sendCmd((char *)message);
		break;
	}
}

/*
 * Determine if a parameter has changed
 * The result will be processed in loop() -> processParameterChange()
 */
void ICHIPWIFI::getNextParam() {
	sendCmd("WNXT"); //send command to get next changed parameter
}

/*
 * Try to retrieve the value of the given parameter.
 */
void ICHIPWIFI::getParamById(String paramName) {
	serialInterface->write(Constants::ichipCommandPrefix);
	serialInterface->print(paramName);
	serialInterface->print("?");
	serialInterface->write(13);
}

/*
 * Set a parameter to the given string value
 */
void ICHIPWIFI::setParam(String paramName, String value) {
	serialInterface->write(Constants::ichipCommandPrefix);
	serialInterface->print(paramName);
	serialInterface->write("=\"");
	serialInterface->print(value);
	serialInterface->write("\"\r");
}

/*
 * Set a parameter to the given int32 value
 */
void ICHIPWIFI::setParam(String paramName, int32_t value) {
	sprintf(buffer, "%l", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given uint32 value
 */
void ICHIPWIFI::setParam(String paramName, uint32_t value) {
	sprintf(buffer, "%lu", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given int16 value
 */
void ICHIPWIFI::setParam(String paramName, int16_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given unit16 value
 */
void ICHIPWIFI::setParam(String paramName, uint16_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}

/*
 * Set a parameter to the given unit16 value
 */
void ICHIPWIFI::setParam(String paramName, uint8_t value) {
	sprintf(buffer, "%d", value);
	setParam(paramName, buffer);
}
/*
 * Set a parameter to the given float value
 */
void ICHIPWIFI::setParam(String paramName, float value, int precision) {
	char format[10];
	sprintf(format, "%%.%df", precision);
	sprintf(buffer, format, value);
	setParam(paramName, buffer);
}

/*
 * Constructor. Assign serial interface to use for ichip communication
 */
ICHIPWIFI::ICHIPWIFI() {
	prefsHandler = new PrefHandler(ICHIP2128);

	uint8_t sys_type;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
	if (sys_type == 3 || sys_type == 4)
		serialInterface = &Serial2;
	else
		serialInterface = &Serial3; //default is serial 3 because that should be what our shield really uses
}

/*
 * Constructor. Pass serial interface to use for ichip communication
 */
ICHIPWIFI::ICHIPWIFI(USARTClass *which) {
	prefsHandler = new PrefHandler(ICHIP2128);
	serialInterface = which;
}

/*
 * Called in the main loop (hopefully) in order to process serial input waiting for us
 * from the wifi module. It should always terminate its answers with 13 so buffer
 * until we get 13 (CR) and then process it.
 * But, for now just echo stuff to our serial port for debugging
 */

void ICHIPWIFI::loop() {
	int incoming;
	while (serialInterface->available()) {
		incoming = serialInterface->read();
		if (incoming != -1) { //and there is no reason it should be -1
			if (incoming == 13 || ibWritePtr > 126) { // on CR or full buffer, process the line
				incomingBuffer[ibWritePtr] = 0; //null terminate the string
				ibWritePtr = 0; //reset the write pointer
				if (strchr(incomingBuffer, '=') && (strncmp(incomingBuffer, Constants::ichipCommandPrefix, 4) != 0))
					processParameterChange(incomingBuffer);
				else if (strchr(incomingBuffer, ','))
					Logger::info(ICHIP2128, incomingBuffer);
				else if (Logger::isDebug())
					Logger::debug(ICHIP2128, incomingBuffer);
			} else { // add more characters
				if (incoming != 10) // don't add a LF character
					incomingBuffer[ibWritePtr++] = (char) incoming;
			}
		} else
			return;
	}
}

/*
 * Process the parameter update from ichip we received as a response to AT+iWNXT.
 * The response usually looks like this : key="value", so the key can be isolated
 * by looking for the '=' sign and the leading/trailing '"' have to be ignored.
 */
void ICHIPWIFI::processParameterChange(char *key) {
	PotThrottleConfiguration *acceleratorConfig = NULL;
	PotThrottleConfiguration *brakeConfig = NULL;
	MotorControllerConfiguration *motorConfig = NULL;
	bool parameterFound = true;

	char *value = strchr(key, '=');
	if (!value)
		return;

	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();

	if (accelerator)
		acceleratorConfig = (PotThrottleConfiguration *)accelerator->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *)brake->getConfiguration();
	if(motorController)
		motorConfig = (MotorControllerConfiguration *)motorController->getConfiguration();

	value[0] = 0; // replace the '=' sign with a 0
	value++;
	if (value[0] == '"')
		value++; // if the value starts with a '"', advance one character
	if (value[strlen(value) - 1] == '"')
		value[strlen(value) - 1] = 0; // if the value ends with a '"' character, replace it with 0

	if (!strcmp(key, Constants::numThrottlePots) && acceleratorConfig) {
		acceleratorConfig->numberPotMeters = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleSubType) && acceleratorConfig) {
		acceleratorConfig->throttleSubType = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMin1) && acceleratorConfig) {
		acceleratorConfig->minimumLevel1 = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMin2) && acceleratorConfig) {
		acceleratorConfig->minimumLevel2 = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMax1) && acceleratorConfig) {
		acceleratorConfig->maximumLevel1 = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMax2) && acceleratorConfig) {
		acceleratorConfig->maximumLevel2 = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleRegenMax) && acceleratorConfig) {
		acceleratorConfig->positionRegenMaximum = atol(value) * 10;
	} else if (!strcmp(key, Constants::throttleRegenMin) && acceleratorConfig) {
		acceleratorConfig->positionRegenMinimum = atol(value) * 10;
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleFwd) && acceleratorConfig) {
		acceleratorConfig->positionForwardMotionStart = atol(value) * 10;
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMap) && acceleratorConfig) {
		acceleratorConfig->positionHalfPower = atol(value) * 10;
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMinRegen) && acceleratorConfig) {
		acceleratorConfig->minimumRegen = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleMaxRegen) && acceleratorConfig) {
		acceleratorConfig->maximumRegen = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::throttleCreep) && acceleratorConfig) {
		acceleratorConfig->creep = atol(value);
		accelerator->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMin) && brakeConfig) {
		brakeConfig->minimumLevel1 = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMax) && brakeConfig) {
		brakeConfig->maximumLevel1 = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMinRegen) && brakeConfig) {
		brakeConfig->minimumRegen = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::brakeMaxRegen) && brakeConfig) {
		brakeConfig->maximumRegen = atol(value);
		brake->saveConfiguration();
	} else if (!strcmp(key, Constants::speedMax) && motorConfig) {
		motorConfig->speedMax = atol(value);
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::torqueMax) && motorConfig) {
		motorConfig->torqueMax = atol(value) * 10;
		motorController->saveConfiguration();
	} else if (!strcmp(key, Constants::logLevel)) {
		extern PrefHandler *sysPrefs;
		uint8_t loglevel = atol(value);
		Logger::setLoglevel((Logger::LogLevel)loglevel);
		sysPrefs->write(EESYS_LOG_LEVEL, loglevel);
	} else {
		parameterFound = false;
	}
	if (parameterFound) {
		Logger::info(ICHIP2128, "parameter change: %s", key);
		getNextParam(); // try to get another one immediately
	}
}

/*
 * Get parameters from devices and forward them to ichip.
 * This is required to initially set-up the
 */
void ICHIPWIFI::loadParameters() {
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	PotThrottleConfiguration *acceleratorConfig = NULL;
	PotThrottleConfiguration *brakeConfig = NULL;
	MotorControllerConfiguration *motorConfig = NULL;

	Logger::info("loading config params to ichip/wifi");

	if (accelerator)
		acceleratorConfig = (PotThrottleConfiguration *)accelerator->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *)brake->getConfiguration();
	if (motorController)
		motorConfig = (MotorControllerConfiguration *)motorController->getConfiguration();

	if (acceleratorConfig) {
		setParam(Constants::numThrottlePots, acceleratorConfig->numberPotMeters);
		setParam(Constants::throttleSubType, acceleratorConfig->throttleSubType);
		setParam(Constants::throttleMin1, acceleratorConfig->minimumLevel1);
		setParam(Constants::throttleMin2, acceleratorConfig->minimumLevel2);
		setParam(Constants::throttleMax1, acceleratorConfig->maximumLevel1);
		setParam(Constants::throttleMax2, acceleratorConfig->maximumLevel2);
		setParam(Constants::throttleRegenMax, (uint16_t)(acceleratorConfig->positionRegenMaximum / 10));
		setParam(Constants::throttleRegenMin, (uint16_t)(acceleratorConfig->positionRegenMinimum / 10));
		setParam(Constants::throttleFwd, (uint16_t)(acceleratorConfig->positionForwardMotionStart / 10));
		setParam(Constants::throttleMap, (uint16_t)(acceleratorConfig->positionHalfPower / 10));
		setParam(Constants::throttleMinRegen, acceleratorConfig->minimumRegen);
		setParam(Constants::throttleMaxRegen, acceleratorConfig->maximumRegen);
		setParam(Constants::throttleCreep, acceleratorConfig->creep);
	}
	if (brakeConfig) {
		setParam(Constants::brakeMin, brakeConfig->minimumLevel1);
		setParam(Constants::brakeMax, brakeConfig->maximumLevel1);
		setParam(Constants::brakeMinRegen, brakeConfig->minimumRegen);
		setParam(Constants::brakeMaxRegen, brakeConfig->maximumRegen);
	}
	if (motorConfig) {
		setParam(Constants::speedMax, motorConfig->speedMax);
		setParam(Constants::torqueMax, (uint16_t)(motorConfig->torqueMax / 10)); // skip the tenth's
	}
	setParam(Constants::logLevel, (uint8_t)Logger::getLogLevel());
}

DeviceType ICHIPWIFI::getType() {
	return DEVICE_WIFI;
}

DeviceId ICHIPWIFI::getId() {
	return (ICHIP2128);
}

void ICHIPWIFI::loadConfiguration() {
	WifiConfiguration *config = new WifiConfiguration();

	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
		Logger::debug(ICHIP2128, "Valid checksum so using stored wifi config values");
		//TODO: implement processing of config params for WIFI
//		prefsHandler->read(EESYS_WIFI0_SSID, &config->ssid);
	}
}

void ICHIPWIFI::saveConfiguration() {
	WifiConfiguration *config = (WifiConfiguration *) getConfiguration();

	//TODO: implement processing of config params for WIFI
//	prefsHandler->write(EESYS_WIFI0_SSID, config->ssid);
//	prefsHandler->saveChecksum();
}
