/*
 * eeprom_layout.h
 *
*EEPROM Map. There is support for up to 6 devices: A motor controller, display, charger, BMS, Throttle,  and a misc device (EPAS, WOC, etc)
* 
*There is a 256KB eeprom chip which stores these settings. The 4K is allocated to primary storage and 4K is allocated to a "known good"
* storage location. This leaves most of EEPROM free for something else, probably logging. 

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

#ifndef EEPROM_H_
#define EEPROM_H_

#include "config.h"

/*
The device table is just a list of IDs. The devices register for a spot in the table.
Since each device has a 16 bit ID and the reserved space is 128 bytes we can support
64 different devices in the table and EEPROM
Devices are considered enabled if their highest ID bit is set (0x8000) otherwise
they're disabled.
This means that valid IDs must be under 0x8000 but that still leaves a couple of open IDs ;)
First device entry is 0xDEAD if valid - otherwise table is initialized
*/
#define EE_DEVICE_TABLE		0 //where is the table of devices found in EEPROM?

#define EE_DEVICE_SIZE      512 //# of bytes allocated to each device
#define EE_DEVICES_BASE		1024 //start of where devices in the table can use
#define EE_SYSTEM_START		128

#define EE_MAIN_OFFSET          0 //offset from start of EEPROM where main config is
#define EE_LKG_OFFSET           34816  //start EEPROM addr where last known good config is

//start EEPROM addr where the system log starts. <SYS LOG YET TO BE DEFINED>
#define EE_SYS_LOG              69632

//start EEPROM addr for fault log (Used by fault_handler)
#define EE_FAULT_LOG            102400

/*Now, all devices also have a default list of things that WILL be stored in EEPROM. Each actual
implementation for a given device can store it's own custom info as well. This data must come after
the end of the stardard data. The below numbers are offsets from the device's eeprom section
*/

struct EEPROM_LAYOUT
{
	uint16_t location; //offset in device config space
	uint8_t size; //size of this entry 1,2,4, or arbitrary size if a string (anything over 4 is assumed to be a string)
	char name[40]; //plain text way we'd like to refer to this (short description)
	int upperBound; //largest value we'll accept. Negative numbers are OK.
	int lowerBound; //smallest value we'll accept. Negative numbers are OK.
};

EEPROM_LAYOUT DeviceVars[] = {
	//Common to all devices
	{0, 1, "Checksum", 0,255},
	{1, 2, "Device ID", 0x1000,0x7FFF},
	{3, 1, "Local debugging (1 = Local Debugging)", 0,1},
	//motor controller specific
	{20, 2, "Max allowable RPM", 0,20000},
	{22, 2, "Max allowable torque (Nm)", 0,20000},
	{24, 1, "Precharge relay (255 = none)", 0,255},
	{25, 1, "Contactor relay (255 = none)", 0,255},
	{26, 1, "Cooling fan relay (255 = none)", 0,255},
	{27, 1, "Deg C to turn on fan", 0,255},
	{28, 1, "Deg C to turn fan back off", 0,255},
	{29, 4, "Kilowatt hours", 0,0x7FFFFFFF},
	{33, 2, "Precharge resistor resistance (.1 ohms)", 0,32000},
	{35, 2, "Nominal sys voltage (.1 V)", 0,8000},
	{37, 2, "Reverse torque limit (%)", 0,1000},
	{39, 2, "RPM slew rate (RPM/Sec)", 0,20000},
	{41, 2, "Torque slew rate (.1Nm/S)", 0,10000},
	{43, 1, "Brake light output (255=None)", 0,255},
	{44, 1, "Reverse light output (255=None)", 0,255},
	{45, 1, "Drive enable input (255=None)", 0,255},
	{46, 1, "Reverse mode input (255=None)", 0,255},
	{47, 1, "Motoring mode (0=Torque, 1=Speed)", 0,1},
	//Throttle Specific
	{20, 2, "Min ADC value for throttle 1", 0,64000},
	{22, 2, "Max ADC value for throttle 1", 0,64000},
	{24, 2, "Min ADC value for throttle 2", 0,64000},
	{26, 2, "Max ADC value for throttle 2", 0,64000},
	{28, 2, "End of regen throttle pos (.1 %)", 0,1000},
	{30, 2, "Start of motion throttle pos (.1 %)", 0,1000},
	{32, 2, "50% throttle position (.1 %)", 0,1000},
	{34, 2, "Min ADC value for brake", 0,64000},
	{36, 2, "Max ADC value for brake", 0,64000},
	{38, 2, "% of max torque for throttle regen(.1%)", 0,1000},
	{40, 2, "% of max torque for brake regen (.1%)", 0,1000},
	{42, 1, "How many throttle inputs", 1,2},
	{43, 1, "Throttle type (1=Linear, 2=Inverse)", 1,2},
	{44, 2, "Brake regen start percentage (.1%)", 0, 1000},
	{46, 2, "Accel regen start percentage (.1%)", 0, 1000},
	{48, 2, "Max regen point (.1%)", 0, 1000},
	{50, 2, "Creep throttle (.1%)", 0, 1000},
	{52, 1, "Car type for CANBus throttle", 0, 10}, //FIXME: I have no idea what range to use or what the numbers mean
	{53, 1, "Which ADC port to use for Throttle 1", 0, 7},
	{54, 1, "Which ADC port to use for Throttle 2", 0, 7},
};

EEPROM_LAYOUT SysVars[] = {
	{10, 1, "Board type (Generation of board)", 1, 5},
	{20, 1, "Use RAW ADC mode? (1 = yes)", 0, 1},
	{30, 2, "ADC0 gain (1024 is unity)", 1, 10000},
	{32, 2, "ADC0 Offset to subtract", 0, 4095},
	{34, 2, "ADC1 gain (1024 is unity)", 1, 10000},
	{36, 2, "ADC1 Offset to subtract", 0, 4095},
	{38, 2, "ADC2 gain (1024 is unity)", 1, 10000},
	{40, 2, "ADC2 Offset to subtract", 0, 4095},
	{42, 2, "ADC3 gain (1024 is unity)", 1, 10000},
	{44, 2, "ADC3 Offset to subtract", 0, 4095},
	{100, 2, "CAN0 baud rate in thousands", 0, 1000},
	{102, 2, "CAN1 baud rate in thousands", 0, 1000},
	{104, 2, "Serial debugging baud rate in tens", 0, 1000},
	{106, 2, "I2C baud rate in thousands", 0, 400},
	{108, 2, "System tick rate", 0, 65535},
	{199, 1, "RX mail boxes", 1, 7},
	{200, 4, "CAN0 Mask 0", 0, 0x1FFFFFFF},
	{204, 4, "CAN0 Filter 0", 0, 0x1FFFFFFF},
	{208, 4, "CAN0 Mask 1", 0, 0x1FFFFFFF},
	{212, 4, "CAN0 Filter 1", 0, 0x1FFFFFFF},
	{216, 4, "CAN0 Mask 2", 0, 0x1FFFFFFF},
	{220, 4, "CAN0 Filter 2", 0, 0x1FFFFFFF},
	{224, 4, "CAN0 Mask 3", 0, 0x1FFFFFFF},
	{228, 4, "CAN0 Filter 3", 0, 0x1FFFFFFF},
	{232, 4, "CAN0 Mask 4", 0, 0x1FFFFFFF},
	{236, 4, "CAN0 Filter 4", 0, 0x1FFFFFFF},
	{240, 4, "CAN0 Mask 5", 0, 0x1FFFFFFF},
	{244, 4, "CAN0 Filter 5", 0, 0x1FFFFFFF},
	{248, 4, "CAN0 Mask 6", 0, 0x1FFFFFFF},
	{252, 4, "CAN0 Filter 6", 0, 0x1FFFFFFF},
	{256, 4, "CAN1 Mask 0", 0, 0x1FFFFFFF},
	{260, 4, "CAN1 Filter 0", 0, 0x1FFFFFFF},
	{264, 4, "CAN1 Mask 1", 0, 0x1FFFFFFF},
	{268, 4, "CAN1 Filter 1", 0, 0x1FFFFFFF},
	{272, 4, "CAN1 Mask 2", 0, 0x1FFFFFFF},
	{276, 4, "CAN1 Filter 2", 0, 0x1FFFFFFF},
	{280, 4, "CAN1 Mask 3", 0, 0x1FFFFFFF},
	{284, 4, "CAN1 Filter 3", 0, 0x1FFFFFFF},
	{288, 4, "CAN1 Mask 4", 0, 0x1FFFFFFF},
	{292, 4, "CAN1 Filter 4", 0, 0x1FFFFFFF},
	{296, 4, "CAN1 Mask 5", 0, 0x1FFFFFFF},
	{300, 4, "CAN1 Filter 5", 0, 0x1FFFFFFF},
	{304, 4, "CAN1 Mask 6", 0, 0x1FFFFFFF},
	{308, 4, "CAN1 Filter 6", 0, 0x1FFFFFFF},
	{320, 32, "WIFI default SSID", 0, 31},
	{352, 1, "WIFI default channel", 1, 11},
	{353, 1, "DHCP mode (0=Off 1=Server 2=Client", 0, 2},

};

//Allow for a few defined WIFI SSIDs that the GEVCU will try to automatically connect to. 
#define EESYS_WIFI0_SSID	 300 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI0_CHAN         332 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI0_DHCP         333 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI0_MODE         334 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI0_IPADDR       335 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI0_KEY          339 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_WIFI1_SSID	 400 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI1_CHAN         432 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI1_DHCP	 433 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI1_MODE         434 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI1_IPADDR       435 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI1_KEY          439 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_WIFI2_SSID		500 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI2_CHAN		532 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI2_DHCP		533 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI2_MODE		534 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI2_IPADDR		535 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI2_KEY			539 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

//If the above networks can't be joined then try to form our own adhoc network
//with the below parameters.
#define EESYS_WIFIX_SSID		579 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFIX_CHAN		611 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFIX_DHCP		612 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFIX_MODE        613 //1 byte - 0 = B, 1 = G
#define EESYS_WIFIX_IPADDR      614 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFIX_KEY         618 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_LOG_LEVEL         658 //1 byte - the log level
#define EESYS_AMPHOURS			659 //1 byte - ???
#define EESYS_BRAKELIGHT		660 //1 byte - 
#define EESYS_xxxx				661 //1 byte -


enum SysVarDefs
{
	SYS_TYPE = 0,
	RAWADC,
	ADC0_GAIN,
	ADC0_OFFSET,
	ADC1_GAIN,
	ADC1_OFFSET,
	ADC2_GAIN,
	ADC2_OFFSET,
	ADC3_GAIN,
	ADC3_OFFSET,
	CAN0_BAUD,
	CAN1_BAUD,
	SER_BAUD,
	TWI_BAUD,
	TICK_RATE,
	CAN_RX_RCVBOXES,
	CAN0_MASK0,
	CAN0_FILTER0,
	CAN0_MASK1,
	CAN0_FILTER1,
	CAN0_MASK2,
	CAN0_FILTER2,
	CAN0_MASK3,
	CAN0_FILTER3,
	CAN0_MASK4,
	CAN0_FILTER4,
	CAN0_MASK5,
	CAN0_FILTER5,
	CAN0_MASK6,
	CAN0_FILTER6,
	CAN1_MASK0,
	CAN1_FILTER0,
	CAN1_MASK1,
	CAN1_FILTER1,
	CAN1_MASK2,
	CAN1_FILTER2,
	CAN1_MASK3,
	CAN1_FILTER3,
	CAN1_MASK4,
	CAN1_FILTER4,
	CAN1_MASK5,
	CAN1_FILTER5,
	CAN1_MASK6,
	CAN1_FILTER6,

};


//gives a prettier name to the above entries in the array. Must be in the same order as the above array
enum DeviceVarDefs
{
	CHECKSUM = 0,
	DEVICEID,
	LOCAL_DEBUG,
	//Motor Controller Specific
	MAX_RPM,
	MAX_TORQUE,
	PRECHARGE_RELAY,
	CONTACTOR_RELAY,
	FAN_RELAY,
	FAN_ON_TEMP,
	FAN_OFF_TEMP,
	KILOWATTHRS,
	PRECHARGE_OHMS,
	NOMINAL_VOLTS,
	REVERSE_TORQUE,
	RPM_SLEW,
	TORQUE_SLEW,
	BRAKE_OUTPUT,
	REVERSE_OUTPUT,
	ENABLE_IN,
	REVERSE_IN,
	MOTOR_MODE,
	//Throttle specific
	THROTTLE1_MIN,
	THROTTLE1_MAX,
	THROTTLE2_MIN,
	THROTTLE2_MAX,
	REGEN_END,
	MOTION_START,
	HALF_THROTTLE,
	BRAKE_MIN,
	BRAKE_MAX,
	ACCEL_REGEN,
	BRAKE_REGEN,
	NUM_THROTTLES,
	THROTTLE_TYPE,
	MIN_BRAKE_REGEN,
	MIN_ACCEL_REGEN,
	REGEN_MAX,
	CREEP_THROTTLE,
	CAR_TYPE,
	THROTTLE1_ADC,
	THROTTLE2_ADC
};

#define EEFAULT_VALID			0 //1 byte - Set to value of 0xB2 if fault data has been initialized
#define EEFAULT_READPTR			1 //2 bytes - index where reading should start (first unacknowledged fault)
#define EEFAULT_WRITEPTR		3 //2 bytes - index where writing should occur for new faults
#define EEFAULT_RUNTIME			5 //4 bytes - stores the number of seconds (in tenths) that the system has been turned on for - total time ever
#define EEFAULT_FAULTS_START	10 //a bunch of faults stored one after the other start at this location


#endif

