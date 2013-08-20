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

#include "config.h"
#include "sys_io.h"
#include "SerialConsole.h"
#include "Throttle.h"
#include "DeviceManager.h"
#include "MotorController.h"
#include "DmocMotorController.h" //TODO: direct reference to dmoc must be removed


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
}

void SerialConsole::loop() {
    
    if ( handlingEvent == false ) {
        if (SerialUSB.available()) {
          serialEvent();
        }
    }
}

void SerialConsole::printMenu() {
	SerialUSB.println("System Menu:");
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
	SerialUSB.println("B = dump dmoc eeprom values");
	SerialUSB.println("y = detect throttle min");
        SerialUSB.println("Y = detect throttle max");
        SerialUSB.println("z = detect throttle min/max  and other values");
	SerialUSB.println("Z = save detected throttle values");
	SerialUSB.println();
}

/* Single character interpreter of commands over
   serial connection. There is a help menu (press H or h or ?)
*/
void SerialConsole::serialEvent() 
{
        handlingEvent = true;
	int incoming;
	uint8_t val;
	static int state = 0;
	DmocMotorController* dmoc = (DmocMotorController*) DeviceManager::getInstance()->getMotorController(); //TODO: direct reference to dmoc must be removed
	
	incoming = SerialUSB.read();
	if (incoming == -1) {
            handlingEvent = false;
	    return;
	}
	
	if (state == 0) {
		switch (incoming) {
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
    			    } else {
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
		case 'B':
			Logger::info("Retrieving DMOC EEPROM values");
			for (int i = 0; i < 256; i++) {
				memCache->Read(EE_MOTORCTL_START + i, &val);
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
                case 'y': // detect throttle min
			DeviceManager::getInstance()->getAccelerator()->detectThrottleMin();
			break;
                case 'Y': // detect throttle max
			DeviceManager::getInstance()->getAccelerator()->detectThrottleMax();
			break;
		case 'z': // detect throttle min/max & other details
			DeviceManager::getInstance()->getAccelerator()->detectThrottle();
			break;
		case 'Z': // save throttle settings
                        DeviceManager::getInstance()->getAccelerator()->saveConfiguration();
			break;
		}
	}

        handlingEvent = false;
}
