/*
 * FaultHandler.cpp
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

#include "FaultHandler.h"

  FaultHandler::FaultHandler()
  {
  }

  uint16_t FaultHandler::raiseFault(uint16_t device, uint16_t code, char* msg) 
  {
	  //For now use the logger interface to send the fault to serial
	  //and, even then commented out because otherwise you get an avalance. 
	  //It needs to prevent repeated errors from constantly being sent.
	  //Logger::error("Device: %i raised error: %s", device, msg);
  }

  FAULT FaultHandler::getNextFault()
  {
  }

  FAULT FaultHandler::getFirstFault()
  {
  }

  FAULT FaultHandler::getFault(uint16_t fault)
  {
  }
  
  uint16_t FaultHandler::setFaultACK(uint16_t fault)
  {
  }

  uint16_t setFaultOngoing(uint16_t fault, bool ongoing)
  {
  }

  FaultHandler faultHandler;
