/*
EEPROM Map. There is support for up to 6 devices: A motor controller, display, charger, BMS, Throttle,  and a misc device (EPAS, WOC, etc)
The EEPROM on an ATMEGA2560 is 4K. The firmware should have some space as well. This leads to 9 devices which might need EEPROM storage.
For safety, allocate 450 bytes for every device
*/

#define EE_MOTORCTL_START	0
#define EE_DISPLAY_START	450
#define EE_CHARGER_START	900
#define EE_BMS_START		1350
#define EE_THROTTLE_START	1800
#define EE_MISC_START		2250
#define EE_SYSTEM_START		3600
#define EE_DEVICE_SIZE          450 //# of bytes allocated to each device

/*Now, all devices also have a default list of things that WILL be stored in EEPROM. Each actual
implementation for a given device can store it's own custom info as well. This data must come after
the end of the stardard data. The below numbers are offsets from the device's eeprom section
*/

//first, things in common to all devices - leave 20 bytes for this
#define EE_CHECKSUM 		0 //1 byte - checksum for this section of EEPROM to makesure it is valid
#define EE_DEVICE_ID		1 //2 bytes - the value of the ENUM DEVID of this device.
#define EE_DEVICE_ENABLE	3 //1 byte - if this is zero then the device is disabled



//Motor controller data
#define EEMC_MAX_RPM			20 //2 bytes, unsigned int for maximum allowable RPM
#define EEMC_MAX_TORQUE			22 //2 bytes, unsigned int - maximum torque in tenths of a Nm
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

//throttle data
#define EETH_MIN_ONE			20 //2 bytes - ADC value of minimum value for first channel
#define EETH_MAX_ONE			22 //2 bytes - ADC value of maximum value for first channel
#define EETH_MIN_TWO			24 //2 bytes - ADC value of minimum value for second channel
#define EETH_MAX_TWO			26 //2 bytes - ADC value of maximum value for second channel
#define EETH_REGEN				28 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where regen stops
#define EETH_FWD				30 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion starts 
#define EETH_MAP				32 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion is at 50% throttle
#define EETH_BRAKE_MIN			34 //2 bytes - ADC value of minimum value for brake input
#define EETH_BRAKE_MAX			36 //2 bytes - ADC value of max value for brake input


