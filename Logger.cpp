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

Logger logger;

Logger::Logger() {
    logLevel = CFG_DEFAULT_LOGLEVEL;
    debugging = false;
    deviceLoglevel = new Logger::LogLevel[deviceIdsSize];
    lastMsgRepeated = 0;
    repeatStart = 0;
    historyPtr = 0;
}

/*
 * Output a debug message with a variable amount of parameters.
 * printf() style, see Logger::log()
 *
 */
void Logger::debug(String message, ...)
{
    if (logLevel > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    log("", Debug, message, args);
    va_end(args);
}

/*
 * Output a debug message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::debug(Device *device, String message, ...)
{
    if (getLogLevel(device) > Debug) {
        return;
    }

    va_list args;
    va_start(args, message);
    log(device->getCommonName(), Debug, message, args);
    va_end(args);
}

/*
 * Output a info message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::info(String message, ...)
{
    if (logLevel > Info) {
        return;
    }

    va_list args;
    va_start(args, message);
    log("", Info, message, args);
    va_end(args);
}

/*
 * Output a info message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::info(Device *device, String message, ...)
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
void Logger::warn(String message, ...)
{
    if (logLevel > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    log("", Warn, message, args);
    va_end(args);
}

/*
 * Output a warning message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::warn(Device *device, String message, ...)
{
    if (getLogLevel(device) > Warn) {
        return;
    }

    va_list args;
    va_start(args, message);
    log(device->getCommonName(), Warn, message, args);
    va_end(args);
}

/*
 * Output a error message with a variable amount of parameters
 * printf() style, see Logger::log()
 */
void Logger::error(String message, ...)
{
    if (logLevel > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    log("", Error, message, args);
    va_end(args);
}

/*
 * Output a error message with the name of a device appended before the message
 * printf() style, see Logger::log()
 */
void Logger::error(Device *device, String message, ...)
{
    if (getLogLevel(device) > Error) {
        return;
    }

    va_list args;
    va_start(args, message);
    log(device->getCommonName(), Error, message, args);
    va_end(args);
}

/*
 * Output a comnsole message with a variable amount of parameters
 * printf() style, see Logger::logMessage()
 */
void Logger::console(String message, ...)
{
    va_list args;
    va_start(args, message);
    vsnprintf(msgBuffer, LOG_BUFFER_SIZE, message.c_str(), args);
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
 */
void Logger::log(String deviceName, LogLevel level, String format, va_list args)
{
    vsnprintf(msgBuffer, LOG_BUFFER_SIZE, format.c_str(), args);
    LogEntry logEntry = createLogEntry(level, deviceName, String(msgBuffer));

    logToPrinter(SerialUSB, logEntry);
    logToWifi(logEntry);
}

Logger::LogEntry &Logger::createLogEntry(Logger::LogLevel level, String deviceName, String message)
{
    LogEntry &entry = history[historyPtr++];

    if (historyPtr > HISTORY_SIZE - 1) {
        historyPtr = 0;
    }

    entry.time = millis();
    entry.level = level;
    entry.deviceName = deviceName;
    entry.message = message;

    return entry;
}

String Logger::logLevelToString(Logger::LogLevel level)
{
    switch (level) {
        case Info:
            return "INFO";
        case Warn:
            return "WARNING";
        case Error:
            return "ERROR";
        case Debug:
            return "DEBUG";
    }
    return "";
}

void Logger::logToPrinter(Print &printer, LogEntry &logEntry)
{
    printer.print(logEntry.time);
    printer.print(" - ");
    printer.print(logLevelToString(logEntry.level));
    printer.print(": ");
    if (logEntry.deviceName.length() > 0) {
        printer.print(logEntry.deviceName);
        printer.print(" - ");
    }
    printer.println(logEntry.message);
}

void Logger::logToWifi(LogEntry &logEntry)
{
    if (logEntry.level > Debug) {
        LogEntry lastEntry = history[(historyPtr == 0 ? HISTORY_SIZE - 1 : historyPtr - 1)];
        if (logEntry.message.equals(lastEntry.message) && (repeatStart == 0 || (repeatStart + CFG_LOG_REPEAT_MSG_TIME) > millis())) {
            if (lastMsgRepeated == 0) {
                repeatStart = millis();
            }
            lastMsgRepeated++;
        } else {
            if (lastMsgRepeated > 1) {
                String info = String("Last message repeated ");
                info.concat(lastMsgRepeated);
                info.concat(" times");
                String params[] = { "INFO", "", info };
                deviceManager.sendMessage(DEVICE_WIFI, INVALID, MSG_LOG, params);
                lastMsgRepeated = 0;
                repeatStart = 0;
            }

            String params[] = { logLevelToString(logEntry.level), logEntry.deviceName, logEntry.message };
            deviceManager.sendMessage(DEVICE_WIFI, INVALID, MSG_LOG, params);
        }
    }
}

void Logger::printHistory(Print &printer) {
    printer.println("LOG START");
    for (uint16_t i = historyPtr; i < HISTORY_SIZE - 1 && history[i].time > 0; i++) {
        logToPrinter(printer, history[i]);
        handOff();
    }
    for (uint16_t i = 0; i < historyPtr; i++) {
        logToPrinter(printer, history[i]);
        handOff();
    }
    printer.println("LOG END");
}

// dirty but we need avoid a timeout in controllers due to long serial processing
void Logger::handOff() {
    tickHandler.process();
    canHandlerEv.process();
    canHandlerCar.process();
}
