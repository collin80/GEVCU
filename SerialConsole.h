/*
 * SerialConsole.h
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

#ifndef SERIALCONSOLE_H_
#define SERIALCONSOLE_H_

#include "config.h"
#include "Heartbeat.h"
#include "MemCache.h"
#include "config.h"
#include "sys_io.h"
#include "PotThrottle.h"
#include "DeviceManager.h"
#include "MotorController.h"
#include "DmocMotorController.h" //TODO: direct reference to dmoc must be removed
#include "ThrottleDetector.h"
#include "ichip_2128.h"

class SerialConsole {
public:
    SerialConsole(MemCache* memCache);
	SerialConsole(MemCache* memCache, Heartbeat* heartbeat);
	void loop();
	void printMenu();

protected:
	enum CONSOLE_STATE
	{
		STATE_ROOT_MENU
	};

private:
    Heartbeat* heartbeat;
    MemCache* memCache;
    bool handlingEvent;
	char cmdBuffer[80];
	int ptrBuffer;
	int state;
    
    void init();
    void serialEvent();
	void handleConsoleCmd();
	void handleShortCmd();
    void handleConfigCmd();
};

#endif /* SERIALCONSOLE_H_ */
