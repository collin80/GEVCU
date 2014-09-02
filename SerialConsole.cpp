/*
 * SerialConsole.cpp
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

#include "SerialConsole.h"

extern PrefHandler *sysPrefs;

SerialConsole::SerialConsole(MemCache* memCache) :
		memCache(memCache), heartbeat(NULL) {
	init();
}

SerialConsole::SerialConsole(MemCache* memCache, Heartbeat* heartbeat) :
		memCache(memCache), heartbeat(heartbeat) {
	init();
}

void SerialConsole::init() {
	handlingEvent = false;

	//State variables for serial console
	ptrBuffer = 0;
	state = STATE_ROOT_MENU;
      
}

void SerialConsole::loop() {

	if (handlingEvent == false) {
		if (SerialUSB.available()) {
			serialEvent();
		}
	}
}

void SerialConsole::printMenu() {
	MotorController* motorController = (MotorController*) DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	ICHIPWIFI *wifi = (ICHIPWIFI*) DeviceManager::getInstance()->getDeviceByType(DEVICE_WIFI);

	//Show build # here as well in case people are using the native port and don't get to see the start up messages
	SerialUSB.print("Build number: ");
	SerialUSB.println(CFG_BUILD_NUM);
	if (motorController) {
		SerialUSB.println(
			"Motor Controller Status: isRunning: " + String(motorController->isRunning()) + " isFaulted: " + String(motorController->isFaulted()));
	}
	SerialUSB.println("System Menu:");
	SerialUSB.println();
	SerialUSB.println("Enable line endings of some sort (LF, CR, CRLF)");
	SerialUSB.println();
	SerialUSB.println("Short Commands:");
	SerialUSB.println("h = help (displays this message)");
	if (heartbeat != NULL) {
		SerialUSB.println("L = show raw analog/digital input/output values (toggle)");
	}
	SerialUSB.println("K = set all outputs high");
	SerialUSB.println("J = set all outputs low");
	//SerialUSB.println("U,I = test EEPROM routines");
	SerialUSB.println("E = dump system eeprom values");
	SerialUSB.println("z = detect throttle min/max, num throttles and subtype");
	SerialUSB.println("Z = save throttle values");
	SerialUSB.println("b = detect brake min/max");
	SerialUSB.println("B = save brake values");
	SerialUSB.println("p = enable wifi passthrough (reboot required to resume normal operation)");
	SerialUSB.println("S = show possible device IDs");
	SerialUSB.println("w = reset wifi to factory defaults, setup GEVCU ad-hoc network");
	SerialUSB.println("W = Set Wifi to WPS mode (try to automatically connect)");
	SerialUSB.println("s = Scan WiFi for nearby access points");
	SerialUSB.println();
	SerialUSB.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
    SerialUSB.println();
    Logger::console("LOGLEVEL=%i - set log level (0=debug, 1=info, 2=warn, 3=error, 4=off)", Logger::getLogLevel());
   
	uint8_t systype;
	sysPrefs->read(EESYS_SYSTEM_TYPE, &systype);
	Logger::console("SYSTYPE=%i - Set board revision (Dued=2, GEVCU3=3, GEVCU4=4)", systype);

	DeviceManager::getInstance()->printDeviceList();

	if (motorController && motorController->getConfiguration()) {
		MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();
                  
         SerialUSB.println();
         SerialUSB.println("MOTOR CONTROLS");
         SerialUSB.println();
		Logger::console("TORQ=%i - Set torque upper limit (tenths of a Nm)", config->torqueMax);
		Logger::console("RPM=%i - Set maximum RPM", config->speedMax);
		Logger::console("REVLIM=%i - How much torque to allow in reverse (Tenths of a percent)", config->reversePercent);
                            
	    Logger::console("COOLFAN=%i - Digital output to turn on cooling fan(0-7, 255 for none)", config->coolFan);
        Logger::console("COOLON=%i - Inverter temperature C to turn cooling on", config->coolOn);
        Logger::console("COOLOFF=%i - Inverter temperature C to turn cooling off", config->coolOff);  
        Logger::console("BRAKELT = %i - Digital output to turn on brakelight (0-7, 255 for none)", config->brakeLight);
        Logger::console("REVLT=%i - Digital output to turn on reverse light (0-7, 255 for none)", config->revLight);
        Logger::console("ENABLEIN=%i - Digital input to enable motor controller (0-3, 255 for none)", config->enableIn);
        Logger::console("REVIN=%i - Digital input to reverse motor rotation (0-3, 255 for none)", config->reverseIn);
       
	}


	if (accelerator && accelerator->getConfiguration()) {
		PotThrottleConfiguration *config = (PotThrottleConfiguration *) accelerator->getConfiguration();
         SerialUSB.println();
         SerialUSB.println("THROTTLE CONTROLS");
         SerialUSB.println();
	
		Logger::console("TPOT=%i - Number of pots to use (1 or 2)", config->numberPotMeters);
		Logger::console("TTYPE=%i - Set throttle subtype (1=std linear, 2=inverse)", config->throttleSubType);
		Logger::console("T1MN=%i - Set throttle 1 min value", config->minimumLevel1);
		Logger::console("T1MX=%i - Set throttle 1 max value", config->maximumLevel1);
		Logger::console("T2MN=%i - Set throttle 2 min value", config->minimumLevel2);
		Logger::console("T2MX=%i - Set throttle 2 max value", config->maximumLevel2);
		Logger::console("TRGNMAX=%i - Tenths of a percent of pedal where regen is at max", config->positionRegenMaximum);
		Logger::console("TRGNMIN=%i - Tenths of a percent of pedal where regen is at min", config->positionRegenMinimum);
		Logger::console("TFWD=%i - Tenths of a percent of pedal where forward motion starts", config->positionForwardMotionStart);
		Logger::console("TMAP=%i - Tenths of a percent of pedal where 50% throttle will be", config->positionHalfPower);
		Logger::console("TMINRN=%i - Percent of full torque to use for min throttle regen", config->minimumRegen);
		Logger::console("TMAXRN=%i - Percent of full torque to use for max throttle regen", config->maximumRegen);
		Logger::console("TCREEP=%i - Percent of full torque to use for creep (0=disable)", config->creep);
	}

	if (brake && brake->getConfiguration()) {
		PotThrottleConfiguration *config = (PotThrottleConfiguration *) brake->getConfiguration();
                SerialUSB.println();
                SerialUSB.println("BRAKE CONTROLS");
	        SerialUSB.println();
		Logger::console("B1MN=%i - Set brake min value", config->minimumLevel1);
		Logger::console("B1MX=%i - Set brake max value", config->maximumLevel1);
		Logger::console("BMINR=%i - Percent of full torque for start of brake regen", config->minimumRegen);
		Logger::console("BMAXR=%i - Percent of full torque for maximum brake regen", config->maximumRegen);
	}

	if (motorController && motorController->getConfiguration()) {
		MotorControllerConfiguration *config = (MotorControllerConfiguration *) motorController->getConfiguration();
                SerialUSB.println();
                SerialUSB.println("PRECHARGE CONTROLS");
	        SerialUSB.println();
		Logger::console("kWh=%d - kiloWatt Hours of energy used", config->kilowattHrs/3600000);
		Logger::console("PREDELAY=%i - Precharge delay time in milliseconds ", config->prechargeR);
	      //Logger::console("NOMV=%i - Nominal system voltage (1/10 of a volt)", config->nominalVolt);
		Logger::console("PRELAY=%i - Which output to use for precharge contactor (255 to disable)", config->prechargeRelay);
		Logger::console("MRELAY=%i - Which output to use for main contactor (255 to disable)", config->mainContactorRelay);
		Logger::console("NOMV=%i - Fully charged pack voltage", config->nominalVolt/10);
	}

	if (wifi) {
		//WifiConfiguration *config = (WifiConfiguration *) wifi->getConfiguration();
		//Logger::console("WSSIDn= - Set SSID to connect to (n=0-9)");
		//Logger::console("WPASSn= - Set WEP/WPA password (n=0-9)");
		//Logger::console("WTYPEn= - Set type of Wifi AP (n=0-9)");
		//Logger::console("Types: 0 = No sec, 1 = WEP64, 2 = WEP128, 3 = WPA-PSK, 4 = WPA2-PSK");
		//Logger::console("Types: 5 = WPA-TKIP, 6 = WPA2-TKIP, 7=EAP/WEP64, 8=EAP/WEP128");

	}

	Logger::console("WLAN - send a AT+i command to the wlan device");
}

/*	There is a help menu (press H or h or ?)

 This is no longer going to be a simple single character console.
 Now the system can handle up to 80 input characters. Commands are submitted
 by sending line ending (LF, CR, or both)
 */
void SerialConsole::serialEvent() {
	int incoming;
	incoming = SerialUSB.read();
	if (incoming == -1) { //false alarm....
		return;
	}

	if (incoming == 10 || incoming == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
	} else {
		cmdBuffer[ptrBuffer++] = (unsigned char) incoming;
		if (ptrBuffer > 79)
			ptrBuffer = 79;
	}
}

void SerialConsole::handleConsoleCmd() {
	handlingEvent = true;

	if (state == STATE_ROOT_MENU) {
		if (ptrBuffer == 1) { //command is a single ascii character
			handleShortCmd();
		} else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}
	}
	handlingEvent = false;
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
 comparison purposes.
 */
void SerialConsole::handleConfigCmd() {
	PotThrottleConfiguration *acceleratorConfig = NULL;
	PotThrottleConfiguration *brakeConfig = NULL;
	MotorControllerConfiguration *motorConfig = NULL;
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	MotorController *motorController = DeviceManager::getInstance()->getMotorController();
	int i;
	int newValue;
	bool updateWifi = true;

	//Logger::debug("Cmd size: %i", ptrBuffer);
	if (ptrBuffer < 6)
		return; //4 digit command, =, value is at least 6 characters
	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
	String cmdString = String();
	unsigned char whichEntry = '0';
	i = 0;

	while (cmdBuffer[i] != '=' && i < ptrBuffer) {
		/*if (cmdBuffer[i] >= '0' && cmdBuffer[i] <= '9') {
		    whichEntry = cmdBuffer[i++] - '0';
		}
		else */ cmdString.concat(String(cmdBuffer[i++]));
	}

	i++; //skip the =

	if (i >= ptrBuffer)
	{
		Logger::console("Command needs a value..ie TORQ=3000");
		Logger::console("");
		return; //or, we could use this to display the parameter instead of setting
	}

	if (accelerator)
		acceleratorConfig = (PotThrottleConfiguration *) accelerator->getConfiguration();
	if (brake)
		brakeConfig = (PotThrottleConfiguration *) brake->getConfiguration();
	if (motorController)
		motorConfig = (MotorControllerConfiguration *) motorController->getConfiguration();

	// strtol() is able to parse also hex values (e.g. a string "0xCAFE"), useful for enable/disable by device id
	newValue = strtol((char *) (cmdBuffer + i), NULL, 0);

	cmdString.toUpperCase();
	if (cmdString == String("TORQ") && motorConfig) {
		Logger::console("Setting Torque Limit to %i", newValue);
		motorConfig->torqueMax = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("RPM") && motorConfig) {
		Logger::console("Setting RPM Limit to %i", newValue);
		motorConfig->speedMax = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("REVLIM") && motorConfig) {
		Logger::console("Setting Reverse Limit to %i", newValue);
		motorConfig->reversePercent = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("TPOT") && acceleratorConfig) {
		Logger::console("Setting # of Throttle Pots to %i", newValue);
		acceleratorConfig->numberPotMeters = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TTYPE") && acceleratorConfig) {
		Logger::console("Setting Throttle Subtype to %i", newValue);
		acceleratorConfig->throttleSubType = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T1MN") && acceleratorConfig) {
		Logger::console("Setting Throttle1 Min to %i", newValue);
		acceleratorConfig->minimumLevel1 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T1MX") && acceleratorConfig) {
		Logger::console("Setting Throttle1 Max to %i", newValue);
		acceleratorConfig->maximumLevel1 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T2MN") && acceleratorConfig) {
		Logger::console("Setting Throttle2 Min to %i", newValue);
		acceleratorConfig->minimumLevel2 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("T2MX") && acceleratorConfig) {
		Logger::console("Setting Throttle2 Max to %i", newValue);
		acceleratorConfig->maximumLevel2 = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TRGNMAX") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen maximum to %i", newValue);
		acceleratorConfig->positionRegenMaximum = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TRGNMIN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen minimum to %i", newValue);
		acceleratorConfig->positionRegenMinimum = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TFWD") && acceleratorConfig) {
		Logger::console("Setting Throttle Forward Start to %i", newValue);
		acceleratorConfig->positionForwardMotionStart = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMAP") && acceleratorConfig) {
		Logger::console("Setting Throttle MAP Point to %i", newValue);
		acceleratorConfig->positionHalfPower = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMINRN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen Minimum Strength to %i", newValue);
		acceleratorConfig->minimumRegen = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TMAXRN") && acceleratorConfig) {
		Logger::console("Setting Throttle Regen Maximum Strength to %i", newValue);
		acceleratorConfig->maximumRegen = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("TCREEP") && acceleratorConfig) {
		Logger::console("Setting Throttle Creep Strength to %i", newValue);
		acceleratorConfig->creep = newValue;
		accelerator->saveConfiguration();
	} else if (cmdString == String("BMAXR") && brakeConfig) {
		Logger::console("Setting Max Brake Regen to %i", newValue);
		brakeConfig->maximumRegen = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("BMINR") && brakeConfig) {
		Logger::console("Setting Min Brake Regen to %i", newValue);
		brakeConfig->minimumRegen = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("B1MX") && brakeConfig) {
		Logger::console("Setting Brake Max to %i", newValue);
		brakeConfig->maximumLevel1 = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("B1MN") && brakeConfig) {
		Logger::console("Setting Brake Min to %i", newValue);
		brakeConfig->minimumLevel1 = newValue;
		brake->saveConfiguration();
	} else if (cmdString == String("PREC") && motorConfig) {
		Logger::console("Setting Precharge Capacitance to %i", newValue);
		motorConfig->kilowattHrs = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("PREDELAY") && motorConfig) {
		Logger::console("Setting Precharge Time Delay to %i milliseconds", newValue);
		motorConfig->prechargeR = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("NOMV") && motorConfig) {
		Logger::console("Setting fully charged voltage to %d vdc", newValue);
		motorConfig->nominalVolt = newValue * 10;
		motorController->saveConfiguration();
        } else if (cmdString == String("BRAKELT") && motorConfig) {
		motorConfig->brakeLight = newValue;
		motorController->saveConfiguration();
		Logger::console("Brake light output set to DOUT%i.",newValue);
 } else if (cmdString == String("REVLT") && motorConfig) {
		motorConfig->revLight = newValue;
		motorController->saveConfiguration();
		Logger::console("Reverse light output set to DOUT%i.",newValue);
 } else if (cmdString == String("ENABLEIN") && motorConfig) {
		motorConfig->enableIn = newValue;
		motorController->saveConfiguration();
		Logger::console("Motor Enable input set to DIN%i.",newValue);
 } else if (cmdString == String("REVIN") && motorConfig) {
		motorConfig->reverseIn = newValue;
		motorController->saveConfiguration();
		Logger::console("Motor Reverse input set to DIN%i.",newValue);
	} else if (cmdString == String("MRELAY") && motorConfig) {
		Logger::console("Setting Main Contactor relay output to DOUT%i", newValue);
		motorConfig->mainContactorRelay = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("PRELAY") && motorConfig) {
		Logger::console("Setting Precharge Relay output to DOUT%i", newValue);
		motorConfig->prechargeRelay = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("ENABLE")) {
		if (PrefHandler::setDeviceStatus(newValue, true)) {
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("Successfully enabled device.(%X, %d) Power cycle to activate.", newValue, newValue);
		}
		else {
			Logger::console("Invalid device ID (%X, %d)", newValue, newValue);
		}
	} else if (cmdString == String("DISABLE")) {
		if (PrefHandler::setDeviceStatus(newValue, false)) {
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("Successfully disabled device. Power cycle to deactivate.");
		}
		else {
			Logger::console("Invalid device ID (%X, %d)", newValue, newValue);
		}
	} else if (cmdString == String("SYSTYPE")) {
		if (newValue < 5 && newValue > 0) {
			sysPrefs->write(EESYS_SYSTEM_TYPE, (uint8_t)(newValue));
			sysPrefs->saveChecksum();
			sysPrefs->forceCacheWrite(); //just in case someone takes us literally and power cycles quickly
			Logger::console("System type updated. Power cycle to apply.");
		}
		else Logger::console("Invalid system type. Please enter a value 1 - 4");

       
	} else if (cmdString == String("LOGLEVEL")) {
		switch (newValue) {
		case 0:
			Logger::setLoglevel(Logger::Debug);
			Logger::console("setting loglevel to 'debug'");
			break;
		case 1:
			Logger::setLoglevel(Logger::Info);
			Logger::console("setting loglevel to 'info'");
			break;
		case 2:
			Logger::console("setting loglevel to 'warning'");
			Logger::setLoglevel(Logger::Warn);
			break;
		case 3:
			Logger::console("setting loglevel to 'error'");
			Logger::setLoglevel(Logger::Error);
			break;
		case 4:
			Logger::console("setting loglevel to 'off'");
			Logger::setLoglevel(Logger::Off);
			break;
		}
		sysPrefs->write(EESYS_LOG_LEVEL, (uint8_t)newValue);
		sysPrefs->saveChecksum();
	} else if (cmdString == String("WLAN")) {
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)(cmdBuffer + i));
		Logger::info("sent \"AT+i%s\" to wlan device", (cmdBuffer + i));
		updateWifi = false;
	} else if (cmdString == String("WSSID")) {
		String cmdString = String();
    	        cmdString.concat("WSI");
		cmdString.concat(whichEntry);
		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
		updateWifi = false;
	} else if (cmdString == String("WPASS")) {
		String cmdString = String();
		cmdString.concat("WPP");  //WKY for WEP so we should have a way to know the type first
		cmdString.concat(whichEntry);
		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
		updateWifi = false;
	} else if (cmdString == String("WTYPE")) {
		String cmdString = String();
		cmdString.concat("WST");
		cmdString.concat(whichEntry);
		cmdString.concat('=');
		cmdString.concat((char *)(cmdBuffer + i));
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)cmdString.c_str());
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN");
		updateWifi = false;
	} else if (cmdString == String("COOLFAN") && motorConfig) {		
		Logger::console("Cooling fan output updated to: %i", newValue);
        motorConfig->coolFan = newValue;
		motorController->saveConfiguration();
	} else if (cmdString == String("COOLON")&& motorConfig) {
		if (newValue <= 200 && newValue >= 0) {
			Logger::console("Cooling fan ON temperature updated to: %i degrees", newValue);
			motorConfig->coolOn = newValue;
			motorController->saveConfiguration();
		}
		else Logger::console("Invalid cooling ON temperature. Please enter a value 0 - 200F");
	} else if (cmdString == String("COOLOFF")&& motorConfig) {
		if (newValue <= 200 && newValue >= 0) {
			Logger::console("Cooling fan OFF temperature updated to: %i degrees", newValue);
			motorConfig->coolOff = newValue;
			motorController->saveConfiguration();
	    }
		else Logger::console("Invalid cooling OFF temperature. Please enter a value 0 - 200F");
	} else if (cmdString == String("OUTPUT") && newValue<8) {
                int outie = getOutput(newValue);
                Logger::console("DOUT%d,  STATE: %d",newValue, outie);
                if(outie)
                  {
                    setOutput(newValue,0);
                    motorController->statusBitfield1 &= ~(1 << newValue);//Clear
                  }
                   else
                     {
                       setOutput(newValue,1);
                        motorController->statusBitfield1 |=1 << newValue;//setbit to Turn on annunciator
		      }
                  
             
        Logger::console("DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d, DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3), getOutput(4), getOutput(5), getOutput(6), getOutput(7));
	} else {
		Logger::console("Unknown command");
		updateWifi = false;
	}
	// send updates to ichip wifi
	if (updateWifi)
		DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);
}

void SerialConsole::handleShortCmd() {
	uint8_t val;
	MotorController* motorController = (MotorController*) DeviceManager::getInstance()->getMotorController();
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	DeviceManager *deviceManager = DeviceManager::getInstance();

	switch (cmdBuffer[0]) {
	case 'h':
	case '?':
	case 'H':
		printMenu();
		break;
	case 'L':
		if (heartbeat != NULL) {
			heartbeat->setThrottleDebug(!heartbeat->getThrottleDebug());
			if (heartbeat->getThrottleDebug()) {
				Logger::console("Output raw throttle");
			} else {
				Logger::console("Cease raw throttle output");
			}
		}
		break;
	case 'U':
		Logger::console("Adding a sequence of values from 0 to 255 into eeprom");
		for (int i = 0; i < 256; i++) {
			memCache->Write(1000 + i, (uint8_t) i);
		}
		Logger::info("Flushing cache");
		memCache->FlushAllPages(); //write everything to eeprom
		memCache->InvalidateAll(); //remove all data from cache
		Logger::console("Operation complete.");
		break;
	case 'I':
		Logger::console("Retrieving data previously saved");
		for (int i = 0; i < 256; i++) {
			memCache->Read(1000 + i, &val);
			Logger::console("%d: %d", i, val);
		}
		break;
	case 'E':
		Logger::console("Reading System EEPROM values");
		for (int i = 0; i < 256; i++) {
			memCache->Read(EE_SYSTEM_START + i, &val);
			Logger::console("%d: %d", i, val);
		}
		break;
	case 'K': //set all outputs high
		for (int tout = 0; tout < NUM_OUTPUT; tout++) setOutput(tout, true);
		Logger::console("all outputs: ON");
		break;
	case 'J': //set the four outputs low
		for (int tout = 0; tout < NUM_OUTPUT; tout++) setOutput(tout, false);
		Logger::console("all outputs: OFF");
		break;
	case 'z': // detect throttle min/max & other details
		if (accelerator) {
			ThrottleDetector *detector = new ThrottleDetector(accelerator);
			detector->detect();
		}
		break;
	case 'Z': // save throttle settings
		if (accelerator) {
			accelerator->saveConfiguration();
		}
		break;
	case 'b':
		if (brake) {
			ThrottleDetector *detector = new ThrottleDetector(brake);
			detector->detect();
		}
		break;
	case 'B':
		if (brake != NULL) {
			brake->saveConfiguration();
		}
		break;
	case 'p':
		Logger::console("PASSTHROUGH MODE - All traffic Serial3 <-> SerialUSB");
		//this never stops so basically everything dies. you will have to reboot.
		int inSerialUSB, inSerial3;
		while (1 == 1) {
			inSerialUSB = SerialUSB.read();
			inSerial3 = Serial3.read();
			if (inSerialUSB > -1) {
				Serial3.write((char) inSerialUSB);
			}
			if (inSerial3 > -1) {
				SerialUSB.write((char) inSerial3);
			}
		}
		break;

	case 'S':
		//there is not really any good way (currently) to auto generate this list
		//the information just isn't stored anywhere in code. Perhaps we might
		//think to change that. Otherwise you must remember to update here or
		//nobody will know your device exists. Additionally, these values are
		//decoded into decimal from their hex specification in DeviceTypes.h
		Logger::console("DMOC645 = %X", DMOC645);
		Logger::console("Brusa DMC5 = %X", BRUSA_DMC5);
		Logger::console("Brusa Charger = %X", BRUSACHARGE);
		Logger::console("TCCH Charger = %X", TCCHCHARGE);
		Logger::console("Pot based accelerator = %X", POTACCELPEDAL);
		Logger::console("Pot based brake = %X", POTBRAKEPEDAL);
		Logger::console("CANBus accelerator = %X", CANACCELPEDAL);
		Logger::console("CANBus brake = %X", CANBRAKEPEDAL);
		Logger::console("WIFI (iChip2128) = %X", ICHIP2128);
		Logger::console("Th!nk City BMS = %X", THINKBMS);
		break;
	case 's':
		Logger::console("Finding and listing all nearby WiFi access points");
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"RP20");
		break;
	case 'W':
		Logger::console("Setting Wifi Adapter to WPS mode (make sure you press the WPS button on your router)");
		// restore factory defaults and give it some time
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"AWPS");
		break;
	case 'w':
		Logger::console("Resetting wifi to factory defaults and setting up GEVCU ad-hoc AP");
		// restore factory defaults and give it some time
        // pinMode(43,OUTPUT);
        //  digitalWrite(43, HIGH); 
        //  delay(3000);
        //  digitalWrite(43, LOW);
        delay(3000);
        deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"FD");
		delay(2000);
		// set-up specific ad-hoc network
                deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"HIF=1");  //Set for RS-232 serial.
		delay(1000);
		
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"BDRA");//Auto baud rate selection
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WLCH=6"); //use whichever channel an AP wants to use
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WLSI=!GEVCU"); //set for GEVCU aS AP.
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DIP=192.168.3.10"); //enable searching for a proper IP via DHCP
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DPSZ=8"); //set DHCP server for 10
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"RPG=secret"); // set the configuration password for /ichip
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WPWD=secret"); // set the password to update config params
		delay(1000);
		//	deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WSI1=AndroidAP"); // hotspot SSID
		//	deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WST1=4"); //wpa2
		//	deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"WPP1=verysecret"); //wpa2 password

		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"AWS=1"); //turn on web server for ten clients
		delay(1000);
		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_COMMAND, (void *)"DOWN"); //cause a reset to allow it to come up with the settings
		delay(5000); // a 5 second delay is required for the chip to come back up ! Otherwise commands will be lost

		deviceManager->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL); // reload configuration params as they were lost
		Logger::console("Wifi initialized");
		break;
	case 'X':
		setup(); //this is probably a bad idea. Do not do this while connected to anything you care about - only for debugging in safety!
		break;
	}
}
