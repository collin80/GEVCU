/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */

#include "Device.h"

Device::Device() {
}

//Empty functions to handle these callbacks if the derived classes don't

void Device::setup() {

}

void Device::handleTick() {

}

void Device::handleCanFrame(RX_CAN_FRAME& frame) {

}

void Device::handleMessage(uint32_t msgType, void* message) {

}

Device::DeviceType Device::getType() {
	return DEVICE_NONE;
}

Device::DeviceId Device::getId() {
	return INVALID;
}
