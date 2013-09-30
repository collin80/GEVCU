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

SerialConsole::SerialConsole(MemCache* memCache) 
    : memCache(memCache), heartbeat(NULL)
{
        init();
}

SerialConsole::SerialConsole(MemCache* memCache, Heartbeat* heartbeat) 
    : memCache(memCache), heartbeat(heartbeat)
{
        init();
}

void SerialConsole::init() {
    handlingEvent = false;
        
    // temp
    runRamp = false;
	runStatic = false;
	runThrottle = false;

	//State variables for serial console
	ptrBuffer = 0;
	state = STATE_ROOT_MENU;
}

void SerialConsole::loop() {
    
    if ( handlingEvent == false ) {
        if (SerialUSB.available()) {
          serialEvent();
        }
    }
}

void SerialConsole::printMenu() {
	//Show build # here as well in case people are using the native port and don't get to see the start up messages
	SerialUSB.print("Build number: ");
	SerialUSB.println(CFG_BUILD_NUM);
	SerialUSB.println("Status: isRunning: "
			+ String(DeviceManager::getInstance()->getMotorController()->isRunning())
			+ " isFaulted: "
			+ String(DeviceManager::getInstance()->getMotorController()->isFaulted()));
	SerialUSB.println("System Menu:");
	SerialUSB.println();
	SerialUSB.println("Enable line endings of some sort (LF, CR, CRLF)");
	SerialUSB.println();
	SerialUSB.println("Short Commands:");
	SerialUSB.println("h = help (displays this message)");
//These commented out lines really can't be used any more so there is no point in advertising them right now.
/*
	SerialUSB.println("D = disabled op state");
	SerialUSB.println("S = standby op state");
	SerialUSB.println("E = enabled op state");
	SerialUSB.println("n = neutral gear");
	SerialUSB.println("d = DRIVE gear");
	SerialUSB.println("r = reverse gear");
	SerialUSB.println("<space> = start/stop ramp test");
	SerialUSB.println("x = lock ramp at current value (toggle)");
	SerialUSB.println("t = Use accelerator pedal? (toggle)");
*/
	if ( heartbeat != NULL ) {
		SerialUSB.println("L = output raw input values (toggle)");
	}
	SerialUSB.println("K = set all outputs high");
	SerialUSB.println("J = set all outputs low");
	SerialUSB.println("U,I = test EEPROM routines");
	SerialUSB.println("A = dump system eeprom values");
	SerialUSB.println("z = detect throttle min/max, num throttles and subtype");
	SerialUSB.println("Z = save detected throttle values");
	SerialUSB.println("b = detect brake min/max");
	SerialUSB.println("B = Save detected brake values");
	SerialUSB.println("p = enable wifi passthrough (reboot required to resume normal operation)");
	SerialUSB.println();
	SerialUSB.println("Config Commands (enter command=newvalue). Current values shown in parenthesis:");
	SerialUSB.println("TORQ="
			+ String(DeviceManager::getInstance()->getMotorController()->getMaxTorque())
			+ " - Set torque upper limit (tenths of a Nm)");
	SerialUSB.println("RPMS="
			+ String(DeviceManager::getInstance()->getMotorController()->getMaxRpm())
			+ " - Set maximum RPMs");
	SerialUSB.println("REVLIM="
			+ String(DeviceManager::getInstance()->getMotorController()->getReversePercent())
			+ " - How much torque to allow in reverse (Tenths of a percent)");
	SerialUSB.println("TPOT="
			+ String(DeviceManager::getInstance()->getAccelerator()->getNumThrottlePots())
			+ " - Number of pots to use (1 or 2)");
	SerialUSB.println("TTYPE="
			+ String(DeviceManager::getInstance()->getAccelerator()->getSubtype())
			+ " - Set throttle subtype (1=std linear, 2=inverse)");
	SerialUSB.println("T1MN="
			+ String(DeviceManager::getInstance()->getAccelerator()->getT1Min())
			+ " - Set throttle 1 min value");
	SerialUSB.println("T1MX="
			+ String(DeviceManager::getInstance()->getAccelerator()->getT1Max())
			+ " - Set throttle 1 max value");
	SerialUSB.println("T2MN="
			+ String(DeviceManager::getInstance()->getAccelerator()->getT2Min())
			+ " - Set throttle 2 min value");
	SerialUSB.println("T2MX="
			+ String(DeviceManager::getInstance()->getAccelerator()->getT2Max())
			+ " - Set throttle 2 max value");
	SerialUSB.println("TRGN="
			+ String(DeviceManager::getInstance()->getAccelerator()->getRegenEnd())
			+ " - Tenths of a percent of pedal where regen ends");
	SerialUSB.println("TFWD="
			+ String(DeviceManager::getInstance()->getAccelerator()->getFWDStart())
			+ " - Tenths of a percent of pedal where forward motion starts");
	SerialUSB.println("TMAP="
			+ String(DeviceManager::getInstance()->getAccelerator()->getMAP())
			+ " - Tenths of a percent of pedal where 50% throttle will be");
	SerialUSB.println("TMRN="
			+ String(DeviceManager::getInstance()->getAccelerator()->getMaxRegen())
			+ " - Percent of full torque to use for throttle regen");
	SerialUSB.println("B1MN="
			+ String(DeviceManager::getInstance()->getBrake()->getT1Min())
			+ " - Set brake min value");
	SerialUSB.println("B1MX="
			+ String(DeviceManager::getInstance()->getBrake()->getT1Max())
			+ " - Set brake max value");
	SerialUSB.println("BMINR="
			+ String(DeviceManager::getInstance()->getBrake()->getMinRegen())
			+ " - Percent of full torque for start of brake regen");
	SerialUSB.println("BMAXR="
			+ String(DeviceManager::getInstance()->getBrake()->getMaxRegen())
			+ " - Percent of full torque for maximum brake regen");
	SerialUSB.println("PREC="
			+ String(DeviceManager::getInstance()->getMotorController()->getPrechargeC())
			+ " - Precharge capacitance (uf)");
	SerialUSB.println("PRER="
			+ String(DeviceManager::getInstance()->getMotorController()->getPrechargeR())
			+ " - Precharge resistance (1/10 of ohm)");
	SerialUSB.println("NOMV="
			+ String(DeviceManager::getInstance()->getMotorController()->getNominalV())
			+ " - Nominal system voltage (1/10 of a volt)");
	SerialUSB.println("PRELAY="
			+ String(DeviceManager::getInstance()->getMotorController()->getPrechargeRelay())
			+ " - Which output to use for precharge contactor (255 to disable)");
	SerialUSB.println("MRELAY="
			+ String(DeviceManager::getInstance()->getMotorController()->getMainRelay())
			+ " - Which output to use for main contactor (255 to disable)");
	SerialUSB.println("LOGLEVEL="
			+ String(Logger::getLogLevel())
			+ " - set log level (0=debug, 1=info, 2=warn, 3=error)");
}

/*	There is a help menu (press H or h or ?)

	This is no longer going to be a simple single character console.
	Now the system can handle up to 80 input characters. Commands are submitted
	by sending line ending (LF, CR, or both)
*/
void SerialConsole::serialEvent() 
{
	int incoming;
	incoming = SerialUSB.read();
	if (incoming == -1) { //false alarm....
	    return;
	}

	if (incoming == 10 || incoming == 13) { //command done. Parse it.
		handleConsoleCmd();
		ptrBuffer = 0; //reset line counter once the line has been processed
	}
	else {
		cmdBuffer[ptrBuffer++] = (unsigned char) incoming;
		if (ptrBuffer > 79) ptrBuffer = 79;
	}
}

void SerialConsole::handleConsoleCmd() 
{
	handlingEvent = true;
		
	if (state == STATE_ROOT_MENU) {
		if (ptrBuffer == 1) { //command is a single ascii character
			handleShortCmd();
		}
		else { //if cmd over 1 char then assume (for now) that it is a config line
			handleConfigCmd();
		}
	}
    handlingEvent = false;
}

/*For simplicity the configuration setting code uses four characters for each configuration choice. This makes things easier for
comparison purposes.
*/
void SerialConsole::handleConfigCmd()
{
	int i;
	int newValue;
	//Logger::debug("Cmd size: %i", ptrBuffer);
	if (ptrBuffer < 6) return; //4 digit command, =, value is at least 6 characters
	cmdBuffer[ptrBuffer] = 0; //make sure to null terminate
	String cmdString = String();
	i = 0;
	while (cmdBuffer[i] != '=' && i < ptrBuffer) cmdString.concat(String(cmdBuffer[i++]));

	i++; //skip the =

	if (i >= ptrBuffer) return; //or, we could use this to display the parameter instead of setting

	cmdString.toUpperCase();
	if (cmdString == String("TORQ")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Torque Limit to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setMaxTorque(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("RPMS")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting RPM Limit to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setMaxRpm(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}

	else if (cmdString == String("REVLIM")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Reverse Limit to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setReversePercent(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}

	else if (cmdString == String("TPOT")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting # of Throttle Pots to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setNumThrottlePots(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}

	else if (cmdString == String("TTYPE")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle Subtype to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setSubtype(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}

	else if (cmdString == String("T1MN")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle1 Min to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setT1Min(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("T1MX")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle1 Max to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setT1Max(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("T2MN")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle2 Min to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setT2Min(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("T2MX")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle2 Max to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setT2Max(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("TRGN")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle Regen End to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setRegenEnd(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("TFWD")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle Forward Start to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setFWDStart(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("TMAP")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle MAP Point to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setMAP(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}
	else if (cmdString == String("TMRN")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Throttle Regen Strength to %i", newValue);
		DeviceManager::getInstance()->getAccelerator()->setMaxRegen(newValue);
		DeviceManager::getInstance()->getAccelerator()->saveEEPROM();
	}

	else if (cmdString == String("BMAXR")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Max Brake Regen to %i", newValue);
		DeviceManager::getInstance()->getBrake()->setMaxRegen(newValue);
		DeviceManager::getInstance()->getBrake()->saveEEPROM();
	}

	else if (cmdString == String("BMINR")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Min Brake Regen to %i", newValue);
		DeviceManager::getInstance()->getBrake()->setMinRegen(newValue);
		DeviceManager::getInstance()->getBrake()->saveEEPROM();
	}

	else if (cmdString == String("B1MX")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Brake Max to %i", newValue);
		DeviceManager::getInstance()->getBrake()->setT1Max(newValue);
		DeviceManager::getInstance()->getBrake()->saveEEPROM();
	}
	else if (cmdString == String("B1MN")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Brake Min to %i", newValue);
		DeviceManager::getInstance()->getBrake()->setT1Min(newValue);
		DeviceManager::getInstance()->getBrake()->saveEEPROM();
	}

	else if (cmdString == String("PREC")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Precharge Capacitance to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setPrechargeC(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("PRER")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Precharge Resistance to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setPrechargeR(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("NOMV")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Nominal Voltage to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setNominalV(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("MRELAY")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Main Contactor relay to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setMainRelay(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("PRELAY")) {
		newValue = atoi((char *)(cmdBuffer + i));
		Logger::debug("Setting Precharge Relay to %i", newValue);
		DeviceManager::getInstance()->getMotorController()->setPrechargeRelay(newValue);
		DeviceManager::getInstance()->getMotorController()->saveEEPROM();
	}
	else if (cmdString == String("LOGLEVEL")) {
		newValue = atoi((char *)(cmdBuffer + i));
		switch (newValue) {
		case 0:
			Logger::setLoglevel(Logger::Debug);
			Logger::info("setting loglevel to 'debug'");
			break;
		case 1:
			Logger::setLoglevel(Logger::Info);
			Logger::info("setting loglevel to 'info'");
			break;
		case 2:
			Logger::setLoglevel(Logger::Info);
			Logger::info("setting loglevel to 'warning'");
			Logger::setLoglevel(Logger::Warn);
			break;
		case 3:
			Logger::setLoglevel(Logger::Info);
			Logger::info("setting loglevel to 'error'");
			Logger::setLoglevel(Logger::Error);
			break;
		}
	}
}

void SerialConsole::handleShortCmd() 
{
	uint8_t val;
	DmocMotorController* dmoc = (DmocMotorController*) DeviceManager::getInstance()->getMotorController(); //TODO: direct reference to dmoc must be removed

	switch (cmdBuffer[0]) {
		case 'h':
		case '?':
		case 'H':
			printMenu();
			break;
		case ' ':
			runRamp = !runRamp;
			if (runRamp) {
				Logger::info("Start Ramp Test");
				dmoc->setPowerMode(DmocMotorController::MODE_RPM); //TODO: direct reference to dmoc must be removed
			}
			else {
				Logger::info("End Ramp Test");
				dmoc->setPowerMode(DmocMotorController::MODE_TORQUE); //TODO: direct reference to dmoc must be removed
			}
			break;
		case 'd':
			dmoc->setGear(DmocMotorController::DRIVE); //TODO: direct reference to dmoc must be removed
			Logger::info("forward");
			break;
		case 'n':
			dmoc->setGear(DmocMotorController::NEUTRAL); //TODO: direct reference to dmoc must be removed
			Logger::info("neutral");
			break;
		case 'r':
			dmoc->setGear(DmocMotorController::REVERSE); //TODO: direct reference to dmoc must be removed
			Logger::info("reverse");
			break;
		case 'D':
			dmoc->setOpState(DmocMotorController::DISABLED); //TODO: direct reference to dmoc must be removed
			Logger::info("disabled");
			break;
		case 'S':
			dmoc->setOpState(DmocMotorController::STANDBY); //TODO: direct reference to dmoc must be removed
			Logger::info("standby");
			break;
		case 'E':
			dmoc->setOpState(DmocMotorController::ENABLE); //TODO: direct reference to dmoc must be removed
			Logger::info("enabled");
			break;
		case 'x':
			runStatic = !runStatic;
			if (runStatic) {
				Logger::info("Lock RPM rate");
			}
			else
				Logger::info("Unlock RPM rate");
			break;
		case 't':
			runThrottle = !runThrottle;
			if (runThrottle) {
				Logger::info("Use Throttle Pedal");
				dmoc->setPowerMode(DmocMotorController::MODE_TORQUE); //TODO: direct reference to dmoc must be removed
			}
			else {
				Logger::info("Ignore throttle pedal");
				dmoc->setPowerMode(DmocMotorController::MODE_RPM); //TODO: direct reference to dmoc must be removed
			}
			break;
		case 'L':
			if ( heartbeat != NULL ) {
				heartbeat->setThrottleDebug(!heartbeat->getThrottleDebug());
				if (heartbeat->getThrottleDebug()) {
    				Logger::info("Output raw throttle");
				} 
				else {	
					Logger::info("Cease raw throttle output");
    			}
			}
			break;
		case 'U':
			Logger::info("Adding a sequence of values from 0 to 255 into eeprom");
			for (int i = 0; i < 256; i++) {
				memCache->Write(1000 + i, (uint8_t) i);
			}
			Logger::info("Flushing cache");
			memCache->FlushAllPages(); //write everything to eeprom
			memCache->InvalidateAll(); //remove all data from cache
			Logger::info("Operation complete.");
			break;
		case 'I':
			Logger::info("Retrieving data previously saved");
			for (int i = 0; i < 256; i++) {
				memCache->Read(1000 + i, &val);
				Logger::info("%d: %d", i, val);
			}
			break;
		case 'A':
			Logger::info("Retrieving System EEPROM values");
			for (int i = 0; i < 256; i++) {
				memCache->Read(EE_SYSTEM_START + i, &val);
				Logger::info("%d: %d", i, val);
			}
			break;
		case 'K': //set all outputs high
			setOutput(0, true);
			setOutput(1, true);
			setOutput(2, true);
			setOutput(3, true);
			Logger::info("all outputs: ON");
			break;
		case 'J': //set the four outputs low
			setOutput(0, false);
			setOutput(1, false);
			setOutput(2, false);
			setOutput(3, false);
			Logger::info("all outputs: OFF");
			break;
		case 'z': // detect throttle min/max & other details
			DeviceManager::getInstance()->getAccelerator()->detectThrottle();
			break;
		case 'Z': // save throttle settings
			DeviceManager::getInstance()->getAccelerator()->saveConfiguration();
			break;
		case 'b':
			DeviceManager::getInstance()->getBrake()->detectThrottle();
			break;
		case 'B':
			DeviceManager::getInstance()->getBrake()->saveConfiguration();
			break;
		case 'p':
			Logger::info("PASSTHROUGH MODE - All traffic Serial3 <-> SerialUSB");
			//this never stops so basically everything dies. you will have to reboot.
			int inSerialUSB, inSerial3;
			while (1==1) {
				inSerialUSB = SerialUSB.read();
				inSerial3 = Serial3.read();
				if (inSerialUSB > -1) 
				{
					Serial3.write((char)inSerialUSB);
				}
				if (inSerial3 > -1) 
				{
					SerialUSB.write((char)inSerial3);
				}
			}
			break;
	}
}
