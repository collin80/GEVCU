/*
 * Logger.h
 *
 *  Created on: 10.05.2013
 *      Author: Michael Neuweiler
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#include <Arduino.h>
#include "config.h"

class Logger {
public:
	enum LogLevel {
		Debug, Info, Warn, Error
	};
	static void debug(char *, ...);
	static void info(char *, ...);
	static void warn(char *, ...);
	static void error(char *, ...);
	static void setLoglevel(LogLevel);
	static LogLevel getLogLevel();
private:
	static void log(LogLevel, char[], va_list);
};

#endif /* LOGGER_H_ */
