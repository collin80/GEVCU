/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */

#include "device.h"

Device::Device() {
	this->canHandler = NULL;
}

Device::Device(CanHandler* canHandler) {
	this->canHandler = canHandler;
}

//Empty functions to handle these callbacks if the derived classes don't

void Device::setupDevice() {

}

void Device::handleTick() {

}

void Device::handleCanFrame(CANFrame& frame) {

}

void Device::handleMessage(uint32_t msgType, void* message) {

}

Device::DeviceType Device::getDeviceType() {
	return DEVICE_NONE;
}

Device::DeviceId Device::getDeviceID() {
	return INVALID;
}
