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
  pref_base_addr = EE_MOTORCTL_START;
}

DEVICE::DEVTYPE MOTORCTRL::getDeviceType() {
	return (DEVICE::DEVICE_MOTORCTRL);
}

void MOTORCTRL::setupDevice() {
  //this is where common parameters for motor controllers should be loaded from EEPROM
  if (prefChecksumValid()) { //checksum is good, read in the values stored in EEPROM
    prefRead(EEMC_MAX_RPM, MaxRPM);
    prefRead(EEMC_MAX_TORQUE, MaxTorque);
  }
  else { //checksum invalid. Reinitialize values and store to EEPROM
    MaxRPM = 5000;
    MaxTorque = 500; //50Nm
    prefWrite(EEMC_MAX_RPM, MaxRPM);
    prefWrite(EEMC_MAX_TORQUE, MaxTorque);
  }
}
/*
#define EEMC_ACTIVE_HIGH		24  //1 byte - bitfield - each bit corresponds to whether a given signal is active high (1) or low (0)
									// bit:		function:
									// 0		Drive enable
									// 1		Gear Select - Park/Neutral
									// 2		Gear Select - Forward
									// 3		Gear Select - Reverse
#define EEMC_LIMP_SCALE			25 //1 byte - percentage of power to allow during limp mode
#define EEMC_MAX_REGEN			26 //1 byte - percentage of max torque to apply to regen
#define EEMC_REGEN_SCALE		28 //1 byte - percentage - reduces all regen related values (throttle, brake, maximum above)
#define EEMC_PRECHARGE_RELAY	29 //1 byte - 0 = no precharge relay 1 = yes, there is one
#define EEMC_CONTACTOR_RELAY	30 //1 byte - 0 = no contactor relay 1 = yes there is
#define EEMC_COOLING			31 //1 byte - set point in C for starting up cooling relay
#define EEMC_MIN_TEMP_MOTOR		32 //2 bytes - signed int - Smallest value on temp gauge (1% PWM output)
#define EEMC_MAX_TEMP_MOTOR		34 //2 bytes - signed int - Highest value on temp gauge (99% PWM output)
#define EEMC_MIN_TEMP_INV		36 //2 bytes - signed int - Smallest value on temp gauge (1% PWM output)
#define EEMC_MAX_TEMP_INV		38 //2 bytes - signed int - Highest value on temp gauge (99% PWM output)
*/


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

