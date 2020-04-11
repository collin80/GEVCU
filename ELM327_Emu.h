/*
 *  ELM327_Emu.h
 *
 * Class emulates the serial comm of an ELM327 chip - Used to create an OBDII interface
 *
 * Created: 3/18/2014
 *  Author: Collin Kidder
 */

/*
 Copyright (c) 2013-2014 Collin Kidder, Michael Neuweiler, Charles Galpin

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

/*
List of AT commands to support:
AT E0 (turn echo off)
AT H (0/1) - Turn headers on or off - headers are used to determine how many ECU�s present (hint: only send one response to 0100 and emulate a single ECU system to save time coding)
AT L0 (Turn linefeeds off - just use CR)
AT Z (reset)
AT SH - Set header address - seems to set the ECU address to send to (though you may be able to ignore this if you wish)
AT @1 - Display device description - ELM327 returns: Designed by Andy Honecker 2011
AT I - Cause chip to output its ID: ELM327 says: ELM327 v1.3a
AT AT (0/1/2) - Set adaptive timing (though you can ignore this)
AT SP (set protocol) - you can ignore this
AT DP (get protocol by name) - (always return can11/500)
AT DPN (get protocol by number) - (always return 6)
AT RV (adapter voltage) - Send something like 14.4V
*/


#ifndef ELM327_H_
#define ELM327_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceManager.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "ELM327Processor.h"

/*
 * The extended configuration class with additional parameters for ichip WLAN
 */
class ELM327Configuration : public DeviceConfiguration
{
public:
};

class ELM327Emu : public Device
{
public:

    ELM327Emu();
    ELM327Emu(USARTClass *which);
    void setup(); //initialization on start up
    void handleTick(); //periodic processes
    void handleMessage(uint32_t messageType, void* message);
    DeviceType getType();
    DeviceId getId();
    void loop();
    void sendCmd(String cmd);

    void loadConfiguration();
    void saveConfiguration();

private:
    USARTClass *serialInterface; //Allows for retargetting which serial port we use
    ELM327Processor *elmProc;
    char incomingBuffer[128]; //storage for one incoming line
    int ibWritePtr;
    char buffer[30]; // a buffer for various string conversions

    void processCmd();
};

#endif
