/*
EEPROM Map. There is support for up to 6 devices: A motor controller, display, charger, BMS, Throttle,  and a misc device (EPAS, WOC, etc)
The EEPROM on an ATMEGA2560 is 4K. The firmware should have some space as well. This leads to 9 devices which might need EEPROM storage.
For safety, allocate 450 bytes for every device
*/

#define EE_MOTORCTL_START	0
#define EE_DISPLAY_START		450
#define EE_CHARGER_START		900
#define EE_BMS_START		1350
#define EE_THROTTLE_START	1800
#define EE_MISC_START		2250
#define EE_SYSTEM_START		3600

/*Now, all devices also have a default list of things that WILL be stored in EEPROM. Each actual
implementation for a given device can store it's own custom info as well. This data must come after
the end of the stardard data. The below numbers are offsets from the device's eeprom section
*/

#define EE_CHECKSUM 		0 //1 byte - checksum for this section of EEPROM to make sure it is valid
#define EE_DEVICE_ID		1 //2 bytes - the value of the ENUM DEVID of this device.
#define EE_DEVICE_ENABLE	3 //1 byte - if this is zero then the device is disabled
