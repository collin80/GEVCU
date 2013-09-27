/*
 * Logger.cpp
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

#include "Logger.h"

Logger::LogLevel Logger::logLevel = Logger::Debug;
uint32_t Logger::lastLogTime = 0;

void Logger::debug(char *message, ...) {
	if (logLevel > Debug)
		return;
	va_list args;
	va_start(args, message);
	Logger::log((DeviceId)NULL, Debug, message, args);
	va_end(args);
}

void Logger::debug(DeviceId deviceId, char *message, ...) {
	if (logLevel > Debug)
		return;
	va_list args;
	va_start(args, message);
	Logger::log(deviceId, Debug, message, args);
	va_end(args);
}

void Logger::info(char *message, ...) {
	if (logLevel > Info)
		return;
	va_list args;
	va_start(args, message);
	Logger::log((DeviceId)NULL, Info, message, args);
	va_end(args);
}

void Logger::info(DeviceId deviceId, char *message, ...) {
	if (logLevel > Info)
		return;
	va_list args;
	va_start(args, message);
	Logger::log(deviceId, Info, message, args);
	va_end(args);
}

void Logger::warn(char *message, ...) {
	if (logLevel > Warn)
		return;
	va_list args;
	va_start(args, message);
	Logger::log((DeviceId)NULL, Warn, message, args);
	va_end(args);
}

void Logger::warn(DeviceId deviceId, char *message, ...) {
	if (logLevel > Warn)
		return;
	va_list args;
	va_start(args, message);
	Logger::log(deviceId, Warn, message, args);
	va_end(args);
}

void Logger::error(char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log((DeviceId)NULL, Error, message, args);
	va_end(args);
}

void Logger::error(DeviceId deviceId, char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log(deviceId, Error, message, args);
	va_end(args);
}

void Logger::setLoglevel(LogLevel level) {
	logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() {
	return logLevel;
}

uint32_t Logger::getLastLogTime() {
	return lastLogTime;
}

void Logger::log(DeviceId deviceId, LogLevel level, char *format, va_list args) {
		lastLogTime = millis();
		SerialUSB.print(lastLogTime);
		SerialUSB.print(" - ");

		switch (level) {
		case Debug:
			SerialUSB.print("DEBUG");
			break;
		case Info:
			SerialUSB.print("INFO");
			break;
		case Warn:
			SerialUSB.print("WARNING");
			break;
		case Error:
			SerialUSB.print("ERROR");
			break;
		}
		SerialUSB.print(": ");

		if (deviceId)
			printDeviceName(deviceId);

		for (; *format != 0; ++format) {
			if (*format == '%') {
				++format;
				if (*format == '\0')
					break;
				if (*format == '%') {
					SerialUSB.print(*format);
					continue;
				}
				if (*format == 's') {
					register char *s = (char *) va_arg( args, int );
					SerialUSB.print(s);
					continue;
				}
				if (*format == 'd' || *format == 'i') {
					SerialUSB.print(va_arg( args, int ), DEC);
					continue;
				}
				if (*format == 'f') {
					SerialUSB.print(va_arg( args, double ), 2);
					continue;
				}
				if (*format == 'x') {
					SerialUSB.print(va_arg( args, int ), HEX);
					continue;
				}
				if (*format == 'X') {
					SerialUSB.print("0x");
					SerialUSB.print(va_arg( args, int ), HEX);
					continue;
				}
				if (*format == 'b') {
					SerialUSB.print(va_arg( args, int ), BIN);
					continue;
				}
				if (*format == 'B') {
					SerialUSB.print("0b");
					SerialUSB.print(va_arg( args, int ), BIN);
					continue;
				}
				if (*format == 'l') {
					SerialUSB.print(va_arg( args, long ), DEC);
					continue;
				}

				if (*format == 'c') {
					SerialUSB.print(va_arg( args, int ));
					continue;
				}
				if (*format == 't') {
					if (va_arg( args, int ) == 1) {
						SerialUSB.print("T");
					} else {
						SerialUSB.print("F");
					}
					continue;
				}
				if (*format == 'T') {
					if (va_arg( args, int ) == 1) {
						SerialUSB.print("true");
					} else {
						SerialUSB.print("false");
					}
					continue;
				}

			}
			SerialUSB.print(*format);
		}
		SerialUSB.println();
}

void Logger::printDeviceName(DeviceId deviceId) {
	switch (deviceId) {
	case DMOC645:
		SerialUSB.print("DMOC645");
		break;
	case BRUSA_DMC5:
		SerialUSB.print("DMC5");
		break;
	case BRUSACHARGE:
		SerialUSB.print("NLG5");
		break;
	case TCCHCHARGE:
		SerialUSB.print("TCCH");
		break;
	case POTACCELPEDAL:
		SerialUSB.print("POTACCEL");
		break;
	case POTBRAKEPEDAL:
		SerialUSB.print("POTBRAKE");
		break;
	case CANACCELPEDAL:
		SerialUSB.print("CANACCEL");
		break;
	case ICHIP2128:
		SerialUSB.print("ICHIP");
		break;
	case THINKBMS:
		SerialUSB.print("THINKBMS");
		break;
	}
	SerialUSB.print(" - ");
}
