/*
 * Device.cpp
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

#include "Device.h"

Device::Device() {
	deviceConfiguration = NULL;
	prefsHandler = NULL;
}

//Empty functions to handle these callbacks if the derived classes don't

void Device::setup() {
}

void Device::handleTick() {
}

uint32_t Device::getTickInterval() {
	return 0;
}

//just bubbles up the value from the preference handler.
bool Device::isEnabled() {
	return prefsHandler->isEnabled();
}

void Device::handleMessage(uint32_t msgType, void* message) {
	switch (msgType) {
	case MSG_STARTUP:
		this->setup();
		break;
	}
}

DeviceType Device::getType() {
	return DEVICE_NONE;
}

DeviceId Device::getId() {
	return INVALID;
}

void Device::loadConfiguration() {
}

void Device::saveConfiguration() {
}

DeviceConfiguration *Device::getConfiguration() {
	return this->deviceConfiguration;
}

void Device::setConfiguration(DeviceConfiguration *configuration) {
	this->deviceConfiguration = configuration;
}

