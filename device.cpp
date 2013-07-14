/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */ 

#include "device.h"

//Empty functions to handle these two callbacks if the derived classes don't
void Device::handleFrame(CANFrame& frame) {
	
}

void Device::handleTick() {
	
}

void Device::setupDevice() {
}

Device::DeviceType Device::getDeviceType() {
  return DEVICE_NONE;
}

Device::DeviceId Device::getDeviceID() {
  return INVALID;
}

Device::Device() {
}


Device::Device(CanHandler* canHandler) {
  this->canHandler = canHandler;
}
