/*
 * FaultHandler.cpp
 *
 * Created: 3/19/2013 6:54:53 PM
 *  Author: Collin Kidder
 */ 

#include "FaultHandler.h"
#include "Logger.h"

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
