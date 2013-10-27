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

#include "config.h"
#include "ichip_2128.h"

/*
 * Initialization of hardware and parameters
 */
void ICHIPWIFI::setup() {

	prefsHandler = new PrefHandler(ICHIP2128);

	TickHandler::getInstance()->detach(this);

	tickCounter = 0;
	ibWritePtr = 0;
	loadParams = 5; // wait 5 seconds before loading parameters (required after resetting ichip)
	serialInterface->begin(115200);

	uint8_t sys_type;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &sys_type);
	if (sys_type == 3) {
		digitalWrite(18, HIGH);
	}

	sendCmd("FD");
	delay(500);

	//for now force a specific ad-hoc network to be set up
	sendCmd("WLCH=6"); //use WIFI channel 6
	sendCmd("WLSI=!GEVCU"); //name our ADHOC network GEVCU (the ! indicates a ad-hoc network)
	sendCmd("DIP=192.168.3.10"); //IP of GEVCU is this
	sendCmd("DPSZ=10"); //serve up 10 more addresses (11 - 20)
	sendCmd("RPG=secret"); // set the configuration password for /ichip
	sendCmd("WPWD=secret"); // set the password to update config params

//	sendCmd("WSI1=AndroidAP"); // hotspot SSID
//	sendCmd("WST1=4"); //wpa2
//	sendCmd("WPP1=verysecret"); //wpa2 password

	enableServer();

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);
}

/*
 * Send a command to ichip. The "AT+i" part will be added.
 */
void ICHIPWIFI::sendCmd(String cmd) {
	serialInterface->write("AT+i");
	serialInterface->print(cmd);
	serialInterface->write(13);
	loop();
}

/*
 * Periodic updates of parameters to ichip RAM.
 * Also query for changed parameters of the config page.
 */
void ICHIPWIFI::handleTick() {
	MotorController* motorController = DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();

	tickCounter++;

	// make small slices so the main loop is not blocked for too long
	if (tickCounter == 1) {
		if (motorController) {
			setParam("timeRunning", getTimeRunning());
			setParam("torqueRequested", motorController->getTorqueRequested() / 10.0f, 1);
			setParam("torqueActual", motorController->getTorqueActual() / 10.0f, 1);
		}
		if (accelerator)
			setParam("throttle", accelerator->getLevel() / 10.0f, 1);
		if (brake)
			setParam("brake", brake->getLevel() / 10.0f, 1);
		else
			setParam("brake", "n/a");
	} else if (tickCounter == 2) {
		if (motorController) {
			setParam("speedRequested", motorController->getSpeedRequested());
			setParam("speedActual", motorController->getSpeedActual());
			setParam("dcVoltage", motorController->getDcVoltage() / 10.0f, 1);
			setParam("dcCurrent", motorController->getDcCurrent() / 10.0f, 1);
		}
	} else if (tickCounter == 3) {
		if (motorController) {
			setParam("acCurrent", motorController->getAcCurrent() / 10.0f, 1);
			setParam("bitfield1", motorController->getStatusBitfield1());
			setParam("bitfield2", motorController->getStatusBitfield2());
			setParam("bitfield3", motorController->getStatusBitfield3());
			setParam("bitfield4", motorController->getStatusBitfield4());
		}
	} else if (tickCounter == 4) {
		if (motorController) {
			setParam("running", (motorController->isRunning() ? "true" : "false"));
			setParam("faulted", (motorController->isFaulted() ? "true" : "false"));
			setParam("warning", (motorController->isWarning() ? "true" : "false"));
			setParam("gear", (uint16_t) motorController->getGearSwitch());
		}
	} else if (tickCounter > 4) {
		if (motorController) {
			setParam("tempMotor", motorController->getTemperatureMotor() / 10.0f, 1);
			setParam("tempInverter", motorController->getTemperatureInverter() / 10.0f, 1);
			setParam("tempSystem", motorController->getTemperatureSystem() / 10.0f, 1);
			setParam("mechPower", motorController->getMechanicalPower() / 10.0f, 1);
			tickCounter = 0;
		}
		getNextParam();

		// wait "loadParams" cycles of tickCounter > 4 before sending config parameters
		// sending them too early after a soft-reset of ichip results in lost data.
		if (loadParams > 0) {
			if (loadParams == 1)
				loadParameters();
			loadParams--;
		}
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
	}
}

/*
 * Turn on the web server.
 * Requires a soft-reset to enable the server (DOWN).
 */
void ICHIPWIFI::enableServer() {
	sendCmd("AWS=1"); //turn on web server for three clients
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
}

/*
 * Turn off the web server
 */
void ICHIPWIFI::disableServer() {
	sendCmd("AWS=0"); //turn off web server
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
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
	serialInterface->write("AT+i");
	serialInterface->print(paramName);
	serialInterface->print("?");
	serialInterface->write(13);
}

/*
 * Set a parameter to the given string value
 */
void ICHIPWIFI::setParam(String paramName, String value) {
	serialInterface->write("AT+i");
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
	if (sys_type == 3)
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
				if (Logger::isDebug())
					Logger::debug(ICHIP2128, incomingBuffer);

				if (strchr(incomingBuffer, '=') && (strncmp(incomingBuffer, "AT+i", 4) != 0))
					processParameterChange(incomingBuffer);
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
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();

	Logger::info(ICHIP2128, "parameter change: %s", key);

	if (accelerator)
		acceleratorConfig = (PotThrottleConfiguration *)accelerator->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *)brake->getConfiguration();
	if(motorController)
		motorConfig = (MotorControllerConfiguration *)motorController->getConfiguration();

	char *value = strchr(key, '=');
	if (value) {
		value[0] = 0; // replace the '=' sign with a 0
		value++;
		if (value[0] == '"')
			value++; // if the value starts with a '"', advance one character
		if (value[strlen(value) - 1] == '"')
			value[strlen(value) - 1] = 0; // if the value ends with a '"' character, replace it with 0

		if (!strcmp(key, "numThrottlePots") && acceleratorConfig) {
			acceleratorConfig->numberPotMeters = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleSubType") && acceleratorConfig) {
			acceleratorConfig->throttleSubType = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMin1") && acceleratorConfig) {
			acceleratorConfig->minimumLevel1 = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMin2") && acceleratorConfig) {
			acceleratorConfig->minimumLevel2 = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMax1") && acceleratorConfig) {
			acceleratorConfig->maximumLevel1 = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMax2") && acceleratorConfig) {
			acceleratorConfig->maximumLevel2 = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleRegenMax") && acceleratorConfig) {
			acceleratorConfig->positionRegenMaximum = atol(value) * 10;
		} else if (!strcmp(key, "throttleRegenMin") && acceleratorConfig) {
			acceleratorConfig->positionRegenMinimum = atol(value) * 10;
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleFwd") && acceleratorConfig) {
			acceleratorConfig->positionForwardMotionStart = atol(value) * 10;
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMap") && acceleratorConfig) {
			acceleratorConfig->positionHalfPower = atol(value) * 10;
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMinRegen") && acceleratorConfig) {
			acceleratorConfig->minimumRegen = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleMaxRegen") && acceleratorConfig) {
			acceleratorConfig->maximumRegen = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "throttleCreep") && acceleratorConfig) {
			acceleratorConfig->creep = atol(value);
			accelerator->saveConfiguration();
		} else if (!strcmp(key, "brakeMin") && brakeConfig) {
			brakeConfig->minimumLevel1 = atol(value);
			brake->saveConfiguration();
		} else if (!strcmp(key, "brakeMax") && brakeConfig) {
			brakeConfig->maximumLevel1 = atol(value);
			brake->saveConfiguration();
		} else if (!strcmp(key, "brakeMinRegen") && brakeConfig) {
			brakeConfig->minimumRegen = atol(value);
			brake->saveConfiguration();
		} else if (!strcmp(key, "brakeMaxRegen") && brakeConfig) {
			brakeConfig->maximumRegen = atol(value);
			brake->saveConfiguration();
		} else if (!strcmp(key, "speedMax") && motorConfig) {
			motorConfig->speedMax = atol(value);
			motorController->saveConfiguration();
		} else if (!strcmp(key, "torqueMax") && motorConfig) {
			motorConfig->torqueMax = atol(value) * 10;
			motorController->saveConfiguration();
		} else if (!strcmp(key, "logLevel")) {
			extern PrefHandler *sysPrefs;
			uint8_t loglevel = atol(value);
			Logger::setLoglevel((Logger::LogLevel)loglevel);
			sysPrefs->write(EESYS_LOG_LEVEL, loglevel);
		}
	}
	getNextParam(); // try to get another one immediately
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
		setParam("numThrottlePots", acceleratorConfig->numberPotMeters);
		setParam("throttleSubType", acceleratorConfig->throttleSubType);
		setParam("throttleMin1", acceleratorConfig->minimumLevel1);
		setParam("throttleMin2", acceleratorConfig->minimumLevel2);
		setParam("throttleMax1", acceleratorConfig->maximumLevel1);
		setParam("throttleMax2", acceleratorConfig->maximumLevel2);
		setParam("throttleRegenMax", (uint16_t)(acceleratorConfig->positionRegenMaximum / 10));
		setParam("throttleRegenMin", (uint16_t)(acceleratorConfig->positionRegenMinimum / 10));
		setParam("throttleFwd", (uint16_t)(acceleratorConfig->positionForwardMotionStart / 10));
		setParam("throttleMap", (uint16_t)(acceleratorConfig->positionHalfPower / 10));
		setParam("throttleMinRegen", acceleratorConfig->minimumRegen);
		setParam("throttleMaxRegen", acceleratorConfig->maximumRegen);
		setParam("throttleCreep", acceleratorConfig->creep);
	}
	if (brakeConfig) {
		setParam("brakeMin", brakeConfig->minimumLevel1);
		setParam("brakeMax", brakeConfig->maximumLevel1);
		setParam("brakeMinRegen", brakeConfig->minimumRegen);
		setParam("brakeMaxRegen", brakeConfig->maximumRegen);
	}
	if (motorConfig) {
		setParam("speedMax", motorConfig->speedMax);
		setParam("torqueMax", (uint16_t)(motorConfig->torqueMax / 10)); // skip the tenth's
	}
	setParam("logLevel", (uint8_t)Logger::getLogLevel());
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
