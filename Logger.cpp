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

Logger::LogLevel logLevel = Logger::Debug;

void Logger::debug(char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log(Debug, message, args);
	va_end(args);
}

void Logger::info(char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log(Info, message, args);
	va_end(args);
}

void Logger::warn(char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log(Warn, message, args);
	va_end(args);
}

void Logger::error(char *message, ...) {
	va_list args;
	va_start(args, message);
	Logger::log(Error, message, args);
	va_end(args);
}

void Logger::setLoglevel(LogLevel level) {
	logLevel = level;
}

Logger::LogLevel Logger::getLogLevel() {
	return logLevel;
}

void Logger::log(LogLevel level, char *format, va_list args) {
	if (logLevel <= level) {
		SerialUSB.print(millis());
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
					SerialUSB.print(va_arg( args, float ), DEC);
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
}
