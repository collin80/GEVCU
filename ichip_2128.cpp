/*
 * ichip_2128.cpp
 *
 * Class to interface with the ichip 2128 based wifi adapter we're using on our board
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
 
#include "ichip_2128.h"

//initialization of hardware and parameters
void WIFI::init() {
}

//periodic processes
void WIFI::handleTick() {
}

//turn on the web server
void WIFI::enableServer() {
}

//turn off the web server
void WIFI::disableServer() {
}

//Determine if a parameter has changed, which one, and the new value
String WIFI::getNextParam() {
}

//try to retrieve the value of the given parameter
String WIFI::getParamById(String paramName) {
 
}

//set the given parameter with the given string
String WIFI::setParam(String paramName, String value) {
}

WIFI::WIFI() {
  serialInterface = &Serial3; //default is serial 3 because that should be what our shield really uses
}

WIFI::WIFI(USARTClass *which) {
  serialInterface = which;
}
