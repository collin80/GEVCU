/*
 * constants.h
 *
 * Defines the global / application wide constants
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
 *      Author: Michael Neuweiler
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

namespace Constants {
	// misc
	static const char* trueStr = "true";
	static const char* falseStr = "false";
	static const char* notAvailable = "n/a";
	static const char* ichipCommandPrefix = "AT+i";

	// configuration
	static const char* numThrottlePots = "numThrottlePots";
	static const char* throttleSubType = "throttleSubType";
	static const char* throttleMin1 = "throttleMin1";
	static const char* throttleMin2 = "throttleMin2";
	static const char* throttleMax1 = "throttleMax1";
	static const char* throttleMax2 = "throttleMax2";
	static const char* throttleRegenMax = "throttleRegenMax";
	static const char* throttleRegenMin = "throttleRegenMin";
	static const char* throttleFwd = "throttleFwd";
	static const char* throttleMap = "throttleMap";
	static const char* throttleMinRegen = "throttleMinRegen";
	static const char* throttleMaxRegen = "throttleMaxRegen";
	static const char* throttleCreep = "throttleCreep";
	static const char* brakeMin = "brakeMin";
	static const char* brakeMax = "brakeMax";
	static const char* brakeMinRegen = "brakeMinRegen";
	static const char* brakeMaxRegen = "brakeMaxRegen";
	static const char* speedMax = "speedMax";
	static const char* torqueMax = "torqueMax";
	static const char* logLevel = "logLevel";

	// status
	static const char* timeRunning = "timeRunning";
	static const char* torqueRequested = "torqueRequested";
	static const char* torqueActual = "torqueActual";
	static const char* throttle = "throttle";
	static const char* brake = "brake";
	static const char* speedRequested = "speedRequested";
	static const char* speedActual = "speedActual";
	static const char* dcVoltage = "dcVoltage";
	static const char* dcCurrent = "dcCurrent";
	static const char* acCurrent = "acCurrent";
	static const char* bitfield1 = "bitfield1";
	static const char* bitfield2 = "bitfield2";
	static const char* bitfield3 = "bitfield3";
	static const char* bitfield4 = "bitfield4";
	static const char* running = "running";
	static const char* faulted = "faulted";
	static const char* warning = "warning";
	static const char* gear = "gear";
	static const char* tempMotor = "tempMotor";
	static const char* tempInverter = "tempInverter";
	static const char* tempSystem = "tempSystem";
	static const char* mechPower = "mechPower";

	// messages
	static const char* validChecksum = "Valid checksum, using stored config values";
	static const char* invalidChecksum = "Invalid checksum, using hard coded config values";
	static const char* valueOutOfRange = "value out of range: %l";
	static const char* normalOperation = "normal operation restored";
}

#endif /* CONSTANTS_H_ */
