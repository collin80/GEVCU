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

void MOTORCTRL::handleTick() {
  uint8_t val, val2;
  if (digitalRead(MOTORCTL_INPUT_DRIVE_EN) == LOW) {
    running = true;
  }
  else running = false;
  
  val = digitalRead(MOTORCTL_INPUT_FORWARD);
  val2 = digitalRead(MOTORCTL_INPUT_REVERSE);
  
  GearSwitch = GS_FAULT;
  if (val == LOW && val2 == HIGH) GearSwitch = GS_FORWARD;
  if (val == HIGH && val2 == LOW) GearSwitch = GS_REVERSE;
 
}

void MOTORCTRL::setupDevice() {
  //this is where common parameters for motor controllers should be loaded from EEPROM
  
  //first set up the appropriate digital pins. All are active low currently
  pinMode(MOTORCTL_INPUT_DRIVE_EN, INPUT_PULLUP); //Drive Enable
  pinMode(MOTORCTL_INPUT_FORWARD, INPUT_PULLUP); //Forward gear
  pinMode(MOTORCTL_INPUT_REVERSE, INPUT_PULLUP); //Reverse Gear
  pinMode(MOTORCTL_INPUT_LIMP, INPUT_PULLUP); //Limp mode
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

