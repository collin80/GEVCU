/*
 * FaultHandler.h
 *
 * Creates a universal fault handling system that stores faults in EEPROM and can create, return, and clear
 * fault data. Could be the basis for an OBDII compliant fault database or could be used on its own
 *
 * Technically, one can't clear or erase faults. They get acknowledged and quit showing up
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


#ifndef FAULT_H_
#define FAULT_H_

#include <Arduino.h>
#include "config.h"
#include "Logger.h"
#include "FaultCodes.h"

//structure to use for storing and retrieving faults.
//Stores the info a fault record will contain.
typedef struct {
  uint32_t timeStamp; 
  uint16_t device; //which device is generating this fault
  uint16_t faultCode; //set by the device itself. There is a universal list of codes
  uint8_t ack : 1; ////whether this fault has been acknowledged or not 1 = ack'd 
  uint8_t ongoing : 1; //whether fault still seems to be happening currently 1 = still going on
} FAULT; //should be 9 bytes because the bottom two are bit fields in a single byte


class FaultHandler {
  public:
  FaultHandler(); //constructor
  uint16_t raiseFault(uint16_t device, uint16_t code, char* msg); //raise a new fault. Returns the fault # where this was stored
  FAULT getNextFault(); //get the next un-ack'd fault. Will also get first fault if the first call and you forgot to call getFirstFault
  FAULT getFirstFault(); //get the first un-acknowledged fault
  FAULT getFault(uint16_t fault);
  
  uint16_t setFaultACK(uint16_t fault); //acknowledge the fault # - returns fault # if successful (0xFFFF otherwise)
  uint16_t setFaultOngoing(uint16_t fault, bool ongoing); //set value of ongoing flag - returns fault # on success
  
  private:
  uint16_t  faultWritePointer; //fault # we're up to for writing. Location in EEPROM is start + (fault_ptr * sizeof(FAULT))
  uint16_t  faultReadPointer;  //fault # we're at when reading.
  FAULT faultList[CFG_FAULT_HISTORY_SIZE]; //store up to 50 faults for a long history. 50*9 = 450 bytes of EEPROM
};

extern FaultHandler faultHandler;

#endif /* FAULT_H_ */
