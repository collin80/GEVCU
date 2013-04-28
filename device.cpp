/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */ 

#include "device.h"

//Empty functions to handle these two callbacks if the derived classes don't
void DEVICE::handleFrame(CANFrame& frame) {
	
}

void DEVICE::handleTick() {
	
}

void DEVICE::setupDevice() {
}

DEVICE::DEVTYPE DEVICE::getDeviceType() {
  return DEVICE_NONE;
}

DEVICE::DEVID DEVICE::getDeviceID() {
  return INVALID;
}

DEVICE::DEVICE() {
}


DEVICE::DEVICE(CANHandler* canhandler) {
  canbus = canhandler;
}
