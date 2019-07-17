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
#include "Device.h"
#include "DeviceManager.h"

Logger::LogLevel Logger::logLevel = CFG_DEFAULT_LOGLEVEL;
uint32_t Logger::lastLogTime = 0;
bool Logger::debugging = false;
Logger::LogLevel *Logger::deviceLoglevel = new Logger::LogLevel[deviceIdsSize];
char *Logger::msgBuffer = new char[LOG_BUFFER_SIZE];
char *Logger::lastMsgBuffer = new char[LOG_BUFFER_SIZE];
uint16_t Logger::lastMsgRepeated = 0;
uint32_t Logger::repeatStart = 0;

/*
 * Output a debug message with a variable amount of parameters.
 * printf() style, see Logger::log()
 *
 */
void Logger::debug(char *message, ...)
{
    if (logLevel > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(NULL, Debug, message, args);
    va_end(args);
}

/*
 * Output a debug message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::debug(Device *device, char *message, ...)
{
    if (getLogLevel(device) > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(device->getCommonName(), Debug, message, args);
    va_end(args);
}

/*
 * Output a info message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::info(char *message, ...)
{
    if (logLevel > Info) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(NULL, Info, message, args);
    va_end(args);
}

/*
 * Output a info message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::info(Device *device, char *message, ...)
{
    if (getLogLevel(device) > Info) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(device->getCommonName(), Info, message, args);
    va_end(args);
}

/*
 * Output a warning message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::warn(char *message, ...)
{
    if (logLevel > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(NULL, Warn, message, args);
    va_end(args);
}

/*
 * Output a warning message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::warn(Device *device, char *message, ...)
{
    if (getLogLevel(device) > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(device->getCommonName(), Warn, message, args);
    va_end(args);
}

/*
 * Output a error message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::error(char *message, ...)
{
    if (logLevel > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(NULL, Error, message, args);
    va_end(args);
}

/*
 * Output a error message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::error(Device *device, char *message, ...)
{
    if (getLogLevel(device) > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    Logger::log(device->getCommonName(), Error, message, args);
    va_end(args);
}

/*
 * Output a comnsole message with a variable amount of parameters
 * printf() style, see Logger::logMessage()
 */
void Logger::console(char *message, ...)
{
    va_list args;
    va_start(args, message);
    vsnprintf(msgBuffer, LOG_BUFFER_SIZE, message, args);
    SerialUSB.println(msgBuffer);
    va_end(args);
}

/*
 * Set the log level. Any output below the specified log level will be omitted.
 * Also set the debugging flag for faster evaluation in isDebug().
 */
void Logger::setLoglevel(LogLevel level)
{
    logLevel = level;
    for (int deviceEntry = 0; deviceEntry < deviceIdsSize; deviceEntry ++) {
		deviceLoglevel[deviceEntry] = level;
    }
    debugging = (level == Debug);
}

/*
 * Set the log level for a specific device. If one device has Debugging loglevel set, also set the
 * debugging flag (for faster evaluation in isDebug()).
 */
void Logger::setLoglevel(Device *device, LogLevel level)
{
	debugging = false;
    for (int deviceEntry = 0; deviceEntry < deviceIdsSize; deviceEntry ++) {
    	if (deviceIds[deviceEntry] == device->getId()) {
    		deviceLoglevel[deviceEntry] = level;
    	}
    	if (deviceLoglevel[deviceEntry] == Debug) {
    		debugging = true;
    	}
    }
}
/*
 * Retrieve the current log level.
 */
Logger::LogLevel Logger::getLogLevel()
{
    return logLevel;
}

/*
 * Retrieve the specific log level of a device
 */
Logger::LogLevel Logger::getLogLevel(Device *device)
{
    for (int deviceEntry = 0; deviceEntry < deviceIdsSize; deviceEntry ++) {
    	if (deviceIds[deviceEntry] == device->getId()) {
    		return deviceLoglevel[deviceEntry];
    	}
    }
    return logLevel;
}

/*
 * Return a timestamp when the last log entry was made.
 */
uint32_t Logger::getLastLogTime()
{
    return lastLogTime;
}

/*
 * Returns if debug log level is enabled. This can be used in time critical
 * situations to prevent unnecessary string concatenation (if the message won't
 * be logged in the end).
 *
 * Example:
 * if (Logger::isDebug()) {
 *    Logger::debug("current time: %d", millis());
 * }
 */
boolean Logger::isDebug()
{
    return debugging;
}

/*
 * Output a log message (called by debug(), info(), warn(), error(), console())
 *
 * Supports printf() syntax
 */
void Logger::log(char *deviceName, LogLevel level, char *format, va_list args)
{
    char *logLevel = "DEBUG";
    lastLogTime = millis();

    switch (level) {
        case Info:
            logLevel = "INFO";
            break;
        case Warn:
            logLevel = "WARNING";
            break;
        case Error:
            logLevel = "ERROR";
            break;
    }
    vsnprintf(msgBuffer, LOG_BUFFER_SIZE, format, args);

    // print to serial USB
    SerialUSB.print(lastLogTime);
    SerialUSB.print(" - ");
    SerialUSB.print(logLevel);
    SerialUSB.print(": ");
    if (deviceName) {
        SerialUSB.print(deviceName);
        SerialUSB.print(" - ");
    }
    SerialUSB.println(msgBuffer);

    // send to wifi
    if (level != Debug) {
    	if (strcmp(msgBuffer, lastMsgBuffer) == 0 && (repeatStart == 0 || (repeatStart + CFG_LOG_REPEAT_MSG_TIME) > millis())) {
    		if (lastMsgRepeated == 0) {
    			repeatStart = millis();
    		}
    		lastMsgRepeated++;
    	} else {
    		if (lastMsgRepeated > 1) {
    			sprintf(lastMsgBuffer, "Last message repeated %d times", lastMsgRepeated);
        		char *params[] = { "INFO", NULL, lastMsgBuffer };
        		deviceManager.sendMessage(DEVICE_WIFI, INVALID, MSG_LOG, params);
        		lastMsgRepeated = 0;
        		repeatStart = 0;
        		lastMsgBuffer[0] = 0;
    		}

    		char *params[] = { logLevel, deviceName, msgBuffer };
    		deviceManager.sendMessage(DEVICE_WIFI, INVALID, MSG_LOG, params);
    		strcpy(lastMsgBuffer, msgBuffer);
    	}
    }
}
