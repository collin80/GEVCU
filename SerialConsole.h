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
#include "SystemIO.h"
#include "PotThrottle.h"
#include "PotBrake.h"
#include "CanThrottle.h"
#include "CanBrake.h"
#include "DeviceManager.h"
#include "MotorController.h"
#include "BrusaDMC5.h"
#include "BrusaBSC6.h"
#include "ThrottleDetector.h"
#include "ichip_2128.h"
#include "CanOBD2.h"

class SerialConsole
{
public:
    SerialConsole();
    void loop();
    void printMenu();

protected:
    enum CONSOLE_STATE
    {
        STATE_ROOT_MENU
    };

private:
    bool handlingEvent;
    char cmdBuffer[80];
    int ptrBuffer;
    int state;

    void serialEvent();
    void sendWifiCommand(String command, String parameter);
    void handleConsoleCmd();
    void handleShortCmd();
    void handleConfigCmd();
    bool handleConfigCmdMotorController(String command, long value);
    bool handleConfigCmdThrottle(String command, long value);
    bool handleConfigCmdBrake(String command, long value);
    bool handleConfigCmdSystemIO(String command, long value);
    bool handleConfigCmdCharger(String command, long value);
    bool handleConfigCmdDcDcConverter(String command, long value);
    bool handleConfigCmdSystem(String command, long value, char *parameter);
    bool handleConfigCmdWifi(String command, String parameter);
    bool handleConfigCmdCanOBD2(String command, long value);
    void printMenuMotorController();
    void printMenuThrottle();
    void printMenuBrake();
    void printMenuSystemIO();
    void printMenuCharger();
    void printMenuDcDcConverter();
    void printMenuCanOBD2();
};

extern SerialConsole serialConsole;

#endif /* SERIALCONSOLE_H_ */
