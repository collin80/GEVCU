/*
 * GEVCU.h
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

#ifndef GEVCU_H_
#define GEVCU_H_

#include <Arduino.h>
#include "config.h"
#include "Status.h"
#include "Device.h"
#include "Throttle.h"
#include "CanThrottle.h"
#include "CanBrake.h"
#include "PotThrottle.h"
#include "PotBrake.h"
#include "BatteryManager.h"
#include "ThinkBatteryManager.h"
#include "MotorController.h"
#include "DmocMotorController.h"
#include "BrusaDMC5.h"
#include "BrusaBSC6.h"
#include "BrusaNLG5.h"
#include "Heartbeat.h"
#include "SystemIO.h"
#include "CanHandler.h"
#include "MemCache.h"
#include "ThrottleDetector.h"
#include "DeviceManager.h"
#include "SerialConsole.h"
#include "ELM327_Emu.h"
#include "ichip_2128.h"
#include "Sys_Messages.h"
#include "PerfTimer.h"
#include "CodaMotorController.h"
#include "FaultHandler.h"
#include "CanIO.h"
#include "CanOBD2.h"

#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

#endif /* GEVCU_H_ */
