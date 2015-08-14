/*
 * OBD2Handler.h - Handles OBD2 PID requests
 *
Copyright (c) 2013-14 Collin Kidder, Michael Neuweiler, Charles Galpin

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

#ifndef OBD2_H_
#define OBD2_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "MotorController.h"
#include "BatteryManager.h"
#include "DeviceManager.h"
#include "TickHandler.h"
#include "CanHandler.h"
#include "constants.h"

class OBD2Handler {
public:
	bool processRequest(uint8_t mode, uint8_t pid, char *inData, char *outData);
	static OBD2Handler *getInstance();

protected:

private:
	OBD2Handler(); //it's not right to try to directly instantiate this class
	bool processShowData(uint8_t pid, char *inData, char *outData);
	bool processShowCustomData(uint16_t pid, char *inData, char *outData);

	static OBD2Handler *instance;
	MotorController* motorController;
	Throttle* accelPedal;
	Throttle* brakePedal;
	BatteryManager *BMS;
};

#endif


