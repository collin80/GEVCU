/*
 * motorctrl.cpp
 *
 * Parent class for all motor controllers.
 *
 * Created: 02/04/2013
 *  Author: Collin Kidder
 */ 
 
 #include "device.h"
 #include "motorctrl.h"
 
MOTORCTRL::MOTORCTRL(MCP2515 *canlib) : DEVICE(canlib) {

}

DEVICE::DEVTYPE MOTORCTRL::getDeviceType() {
	return (DEVICE::DEVICE_MOTORCTRL);
}

int MOTORCTRL::getThrottle() {
	return (requestedThrottle);
}

void MOTORCTRL::setThrottle(int newthrottle) {
	requestedThrottle = newthrottle;
}

bool MOTORCTRL::isRunning() {
	return (running);
}

bool MOTORCTRL::isFaulted() {
	return (faulted);
}

DEVICE::DEVID MOTORCTRL::getDeviceID() {
  return DEVICE::INVALID;
}

