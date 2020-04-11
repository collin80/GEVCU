/*
 * Logger.h
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

#ifndef LOGGER_H_
#define LOGGER_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceTypes.h"

class Device;

class Logger
{
public:
    enum LogLevel
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Off = 4
    };
    Logger();
    void debug(String, ...);
    void debug(Device *, String, ...);
    void info(String, ...);
    void info(Device *, String, ...);
    void warn(String, ...);
    void warn(Device *, String, ...);
    void error(String, ...);
    void error(Device *, String, ...);
    void console(String, ...);
    void setLoglevel(LogLevel);
    void setLoglevel(Device *, LogLevel);
    LogLevel getLogLevel();
    LogLevel getLogLevel(Device *);
    uint32_t getLastLogTime();
    boolean isDebug();
private:
    LogLevel logLevel;
    uint32_t lastLogTime;
    bool debugging;
    LogLevel *deviceLoglevel;
    char *msgBuffer;
    char *lastMsgBuffer;
    uint16_t lastMsgRepeated;
    uint32_t repeatStart;

    void log(String, LogLevel, String format, va_list);
};

extern Logger logger;

#endif /* LOGGER_H_ */
