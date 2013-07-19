/*
 * fault_handler.h
 *
 * Creates a universal fault handling system that stores faults in EEPROM and can create, return, and clear
 * fault data. Could be the basis for an OBDII compliant fault database or could be used on its own
 *
 * Technically, one can't clear or erase faults. They get acknowledged and quit showing up
 *
 * Created: 3/19/2013 8:26:58 PM
 *  Author: Collin Kidder
 */ 


#ifndef FAULT_H_
#define FAULT_H_

#include <Arduino.h>
#include "config.h"

//structure to use for storing and retrieving faults.
//Stores the info a fault record will contain.
typedef struct {
  uint32_t timeStamp; 
  uint16_t device; //which device is generating this fault
  uint16_t faultCode; //set by the device itself. This only has meaning to a device
  uint8_t faultDescription[40]; //a short description of the problem in plain text
  uint8_t ack; //whether this fault has been acknowledged or not 1 = ack'd
  uint8_t ongoing; //whether fault still seems to be happening currently 1 = still going on
} FAULT; //50 bytes


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
};

extern FaultHandler faultHandler;

#endif /* FAULT_H_ */
