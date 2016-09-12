/*
 * SocketProcessor.h
 *
 * Abstract Class / interface to handle socket data
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

#ifndef SOCKETPROCESSOR_H_
#define SOCKETPROCESSOR_H_

#include <WString.h>
/*
 * Interface
 */
class SocketProcessor
{
public:
    virtual ~SocketProcessor() {}
    virtual String generateUpdate() = 0; // generate socket specific update message which gets sent on a regular basis
    virtual String generateLogEntry(char *logLevel, char *deviceName, char *message) = 0; // convert a log message to a socket specific message
    virtual String processInput(char *input) = 0; // process input from a socket and return data (NULL means disconnect socket)
private:

};

#endif /* SOCKETPROCESSOR_H_ */
