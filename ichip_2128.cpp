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
#ifdef CFG_ENABLE_DEVICE_ICHIP2128_WIFI
#include "ichip_2128.h"

//initialization of hardware and parameters
void ICHIPWIFI::setup() {
	TickHandler::getInstance()->detach(this);

	tickCounter = 0;
	ibWritePtr = 0;
	loadParams = 5; // wait 5 seconds before loading parameters (required after resetting ichip)
	serialInterface->begin(115200);

#ifdef GEVCU3
	digitalWrite(18, HIGH);
#endif

	//for now force a specific ad-hoc network to be set up
	sendCmd("WLCH=6"); //use WIFI channel 6
	sendCmd("WLSI=!GEVCU"); //name our ADHOC network GEVCU (the ! indicates a ad-hoc network)
	sendCmd("DIP=192.168.3.10"); //IP of GEVCU is this
	sendCmd("DPSZ=10"); //serve up 10 more addresses (11 - 20)
	sendCmd("RPG=secret"); // set the configuration password for /ichip
	sendCmd("WPWD=secret"); // set the password to update config params
	enableServer();

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_WIFI);
}

void ICHIPWIFI::sendCmd(String cmd) {
	serialInterface->write("AT+i");
	serialInterface->print(cmd);
	serialInterface->write(13);
	loop();
}

//periodic processes
void ICHIPWIFI::handleTick() {
	tickCounter++;
	MotorController* motorController = DeviceManager::getInstance()->getMotorController();
	if (motorController) {
		// make small slices so the main loop is not blocked for too long
		if (tickCounter == 1) {
			setParam("timeRunning", getTimeRunning());
			setParam("throttle", motorController->getThrottle() / 10.0f, 1);
			setParam("torqueRequested", motorController->getTorqueRequested() / 10.0f, 1);
			setParam("torqueActual", motorController->getTorqueActual() / 10.0f, 1);
		}
		else if (tickCounter == 2) {
			setParam("speedRequested", motorController->getSpeedRequested());
			setParam("speedActual", motorController->getSpeedActual());
			setParam("dcVoltage", motorController->getDcVoltage() / 10.0f, 1);
			setParam("dcCurrent", motorController->getDcCurrent() / 10.0f, 1);
		}
		else if (tickCounter == 3) {
			setParam("acCurrent", motorController->getAcCurrent() / 10.0f, 1);
			setParam("bitfield1", motorController->getStatusBitfield1());
			setParam("bitfield2", motorController->getStatusBitfield2());
			setParam("bitfield3", motorController->getStatusBitfield3());
			setParam("bitfield4", motorController->getStatusBitfield4());
		}
		else if (tickCounter == 4) {
			setParam("running", (motorController->isRunning() ? "true" : "false"));
			setParam("faulted", (motorController->isFaulted() ? "true" : "false"));
			setParam("warning", (motorController->isWarning() ? "true" : "false"));
			setParam("gear", motorController->getGearSwitch());
		}
		else if (tickCounter > 4) {
			setParam("tempMotor", motorController->getTemperatureMotor() / 10.0f, 1);
			setParam("tempInverter", motorController->getTemperatureInverter() / 10.0f, 1);
			setParam("tempSystem", motorController->getTemperatureSystem() / 10.0f, 1);
			setParam("mechPower", motorController->getMechanicalPower() / 10.0f, 1);
			tickCounter = 0;

			getNextParam();

			if (loadParams > 0) {
				if (loadParams == 1)
					loadParameters();
				loadParams--;
			}
		}
	}
}

char *ICHIPWIFI::getTimeRunning() {
	uint32_t ms = millis();
	int seconds = (int) (ms / 1000) % 60;
	int minutes = (int) ((ms / (1000 * 60)) % 60);
	int hours = (int) ((ms / (1000 * 3600)) % 24);
	sprintf(runtime, "%02d:%02d:%02d", hours, minutes, seconds);
	return runtime;
}

void ICHIPWIFI::handleMessage(uint32_t messageType, void* message) {
	Device::handleMessage(messageType, message);

	switch (messageType) {
	case MSG_SET_PARAM:
		char **params = (char **)message;
		setParam((char *)params[0], (char *)params[1]);
		break;
	}
}

//turn on the web server
void ICHIPWIFI::enableServer() {
	sendCmd("AWS=1"); //turn on web server for three clients
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
}

//turn off the web server
void ICHIPWIFI::disableServer() {
	sendCmd("AWS=0"); //turn off web server
	sendCmd("DOWN"); //cause a reset to allow it to come up with the settings
}

//Determine if a parameter has changed, which one, and the new value
void ICHIPWIFI::getNextParam() {
	sendCmd("WNXT"); //send command to get next changed parameter
}

//try to retrieve the value of the given parameter
void ICHIPWIFI::getParamById(String paramName) {
	serialInterface->write("AT+i");
	serialInterface->print(paramName);
	serialInterface->print("?");
	serialInterface->write(13);
}

//set the given parameter with the given string
void ICHIPWIFI::setParam(String paramName, String value) {
	serialInterface->write("AT+i");
	serialInterface->print(paramName);
	serialInterface->write("=\"");
	serialInterface->print(value);
	serialInterface->write("\"\r");
}

//set the given parameter with the given string
void ICHIPWIFI::setParam(String paramName, int32_t value) {
	char buffer[33];
	sprintf(buffer, "%lu", value);
	setParam(paramName, buffer);
}

//set the given parameter with the given string
void ICHIPWIFI::setParam(String paramName, float value, int precision) {
	char format[10];
	char buffer[33];
	sprintf(format, "%%.%df", precision);
	sprintf(buffer, format, value);
	setParam(paramName, buffer);
}

ICHIPWIFI::ICHIPWIFI() {
#ifdef GEVCU3
	serialInterface = &Serial2;
#else
	serialInterface = &Serial3; //default is serial 3 because that should be what our shield really uses
#endif
}

ICHIPWIFI::ICHIPWIFI(USARTClass *which) {
	serialInterface = which;
}

//called in the main loop (hopefully) in order to process serial input waiting for us
//from the wifi module. It should always terminate its answers with 13 so buffer
//until we get 13 (CR) and then process it.
//But, for now just echo stuff to our serial port for debugging
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
	char *value = strchr(key, '=');
	if (value) {
		value[0] = 0; // replace the '=' sign with a 0
		value++;
		if (value[0] == '"')
			value++; // if the value starts with a '"', advance one character
		if (value[strlen(value) - 1] == '"')
			value[strlen(value) - 1] = 0; // if the value ends with a '"' character, replace it with 0

		if (!strcmp(key, "numThrottlePots")) {
			DeviceManager::getInstance()->getAccelerator()->setNumberPotMeters(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleSubType")) {
			DeviceManager::getInstance()->getAccelerator()->setSubtype(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMin1")) {
			DeviceManager::getInstance()->getAccelerator()->setMinumumLevel1(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMin2")) {
			DeviceManager::getInstance()->getAccelerator()->setMinimumLevel2(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMax1")) {
			DeviceManager::getInstance()->getAccelerator()->setMaximumLevel1(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMax2")) {
			DeviceManager::getInstance()->getAccelerator()->setMaximumLevel2(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleRegen")) {
			DeviceManager::getInstance()->getAccelerator()->setPositionRegenStart(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleFwd")) {
			DeviceManager::getInstance()->getAccelerator()->setPositionForwardMotionStart(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMap")) {
			DeviceManager::getInstance()->getAccelerator()->setPositionHalfPower(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "throttleMaxRegen")) {
			DeviceManager::getInstance()->getAccelerator()->setMaximumRegen(atol(value));
			DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
		} else if (!strcmp(key, "brakeMin")) {
			DeviceManager::getInstance()->getBrake()->setMinumumLevel1(atol(value));
			DeviceManager::getInstance()->getBrake()->saveEEPROM();
		} else if (!strcmp(key, "brakeMax")) {
			DeviceManager::getInstance()->getBrake()->setMaximumLevel1(atol(value));
			DeviceManager::getInstance()->getBrake()->saveEEPROM();
		} else if (!strcmp(key, "brakeMinRegen")) {
			DeviceManager::getInstance()->getBrake()->setMinimumRegen(atol(value));
			DeviceManager::getInstance()->getBrake()->saveEEPROM();
		} else if (!strcmp(key, "brakeMaxRegen")) {
			DeviceManager::getInstance()->getBrake()->setMaximumRegen(atol(value));
			DeviceManager::getInstance()->getBrake()->saveEEPROM();
		} else if (!strcmp(key, "speedMax")) {
			DeviceManager::getInstance()->getMotorController()->setSpeedMax(atol(value));
			DeviceManager::getInstance()->getMotorController()->saveEEPROM();
		} else if (!strcmp(key, "torqueMax")) {
			DeviceManager::getInstance()->getMotorController()->setTorqueMax(atol(value) * 10);
			DeviceManager::getInstance()->getMotorController()->saveEEPROM();
		}
	}
	getNextParam(); // try to get another one immediately
}

/*
 * Load parameters from eeprom and forward them to ichip.
 * This is required to initially set-up the
 */
void ICHIPWIFI::loadParameters() {
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();

	setParam("numThrottlePots", accelerator->getNumberPotMeters());
	setParam("throttleSubType", accelerator->getSubtype());
	setParam("throttleMin1", accelerator->getMinimumLevel1());
	setParam("throttleMin2", accelerator->getMinimumLevel2());
	setParam("throttleMax1", accelerator->getMaximumLevel1());
	setParam("throttleMax2", accelerator->getMaximumLevel2());
	setParam("throttleRegen", accelerator->getPositionRegenStart());
	setParam("throttleFwd", accelerator->getPositionForwardMotionStart());
	setParam("throttleMap", accelerator->getPositionHalfPower());
	setParam("throttleMaxRegen", accelerator->getMaximumRegen());
	setParam("brakeMin", brake->getMinimumLevel1());
	setParam("brakeMax", brake->getMaximumLevel1());
	setParam("brakeMinRegen", brake->getMinimumRegen());
	setParam("brakeMaxRegen", brake->getMaximumRegen());
	setParam("speedMax", motorController->getSpeedMax());
	setParam("torqueMax", motorController->getTorqueMax() / 10); // skip the tenth's
}

DeviceType ICHIPWIFI::getType() {
	return DEVICE_WIFI;
}

DeviceId ICHIPWIFI::getId() {
	return (ICHIP2128);
}

#endif
