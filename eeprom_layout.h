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
#define EE_DEVICE_TABLE     0 //where is the table of devices found in EEPROM?

#define EE_DEVICE_SIZE      512 //# of bytes allocated to each device
#define EE_DEVICES_BASE     1024 //start of where devices in the table can use
#define EE_SYSTEM_START     128

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

//first, things in common to all devices - leave 20 bytes for this
#define EE_CHECKSUM         0 //1 byte - checksum for this section of EEPROM to makesure it is valid
#define EE_DEVICE_ID        1 //2 bytes - the value of the ENUM DEVID of this device.

//Motor controller data
#define EEMC_MAX_RPM            20 //2 bytes, unsigned int for maximum allowable RPM
#define EEMC_MAX_TORQUE         22 //2 bytes, unsigned int - maximum torque in tenths of a Nm
//#define EEMC_ACTIVE_HIGH      24  //1 byte - bitfield - each bit corresponds to whether a given signal is active high (1) or low (0)
// bit:     function:
// 0        Drive enable
// 1        Gear Select - Park/Neutral
// 2        Gear Select - Forward
// 3        Gear Select - Reverse
//#define EEMC_LIMP_SCALE           25 //1 byte - percentage of power to allow during limp mode
//#define EEMC_MAX_TEMP_INV     38 //2 bytes - signed int - Highest value on temp gauge (99% PWM output)
#define EEMC_KILOWATTHRS        40 //4 bytes - capacitance of controller capacitor bank in micro farads (uf) - set to zero to disable RC precharge
#define EEMC_NOMINAL_V          46 //2 bytes - nominal system voltage to expect (in tenths of a volt)
#define EEMC_REVERSE_LIMIT      48 //2 bytes - a percentage to knock the requested torque down by while in reverse.
#define EEMC_RPM_SLEW_RATE      50 //2 bytes - slew rate (rpm/sec) at which speed should change (only in speed mode)
#define EEMC_TORQUE_SLEW_RATE   52 //2 bytes - slew rate (0.1Nm/sec) at which the torque should change

//throttle data
#define EETH_MIN_ONE            20 //2 bytes - ADC value of minimum value for first channel
#define EETH_MAX_ONE            22 //2 bytes - ADC value of maximum value for first channel
#define EETH_MIN_TWO            24 //2 bytes - ADC value of minimum value for second channel
#define EETH_MAX_TWO            26 //2 bytes - ADC value of maximum value for second channel
#define EETH_REGEN_MIN          28 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where regen stops
#define EETH_FWD                30 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion starts 
#define EETH_MAP                32 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion is at 50% throttle
#define EETH_BRAKE_MIN          34 //2 bytes - ADC value of minimum value for brake input
#define EETH_BRAKE_MAX          36 //2 bytes - ADC value of max value for brake input
#define EETH_MAX_ACCEL_REGEN    38 //2 bytes - maximum percentage of throttle to command on accel pedal regen
#define EETH_MAX_BRAKE_REGEN    40 //2 bytes - maximum percentage of throttle to command for braking regen. Starts at min brake regen and works up to here.
#define EETH_NUM_THROTTLES      42 //1 byte - How many throttle inputs should we use? (1 or 2)
#define EETH_THROTTLE_TYPE      43 //1 byte - Allow for different throttle types. For now 1 = Linear pots, 2 = Inverse relationship between pots. See Throttle.h
#define EETH_MIN_BRAKE_REGEN    44 //2 bytes - the starting level for brake regen as a percentage of throttle
#define EETH_MIN_ACCEL_REGEN    46 //2 bytes - the starting level for accelerator regen as a percentage of throttle
#define EETH_REGEN_MAX          48 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where regen is at maximum
#define EETH_CREEP              50 //2 bytes - percentage of throttle used to simulate creep
#define EETH_CAR_TYPE           52 //1 byte - type of car for querying the throttle position via CAN bus
#define EETH_ADC_1              53 //1 byte - which ADC port to use for first throttle input
#define EETH_ADC_2              54 //1 byte - which ADC port to use for second throttle input

//System Data
#define EESYS_SYSTEM_TYPE        10  //1 byte - 1 = Old school protoboards 2 = GEVCU2/DUED 3 = GEVCU3 - Defaults to 2 if invalid or not set up
#define EESYS_RAWADC             20  //1 byte - if not zero then use raw ADC mode (no preconditioning or buffering or differential).
#define EESYS_ADC0_GAIN          30  //2 bytes - ADC gain centered at 1024 being 1 to 1 gain, thus 512 is 0.5 gain, 2048 is double, etc
#define EESYS_ADC0_OFFSET        32  //2 bytes - ADC offset from zero - ADC reads 12 bit so the offset will be [0,4095] - Offset is subtracted from read ADC value
#define EESYS_ADC1_GAIN          34  //2 bytes - ADC gain centered at 1024 being 1 to 1 gain, thus 512 is 0.5 gain, 2048 is double, etc
#define EESYS_ADC1_OFFSET        36  //2 bytes - ADC offset from zero - ADC reads 12 bit so the offset will be [0,4095] - Offset is subtracted from read ADC value
#define EESYS_ADC2_GAIN          38  //2 bytes - ADC gain centered at 1024 being 1 to 1 gain, thus 512 is 0.5 gain, 2048 is double, etc
#define EESYS_ADC2_OFFSET        40  //2 bytes - ADC offset from zero - ADC reads 12 bit so the offset will be [0,4095] - Offset is subtracted from read ADC value
#define EESYS_ADC3_GAIN          42  //2 bytes - ADC gain centered at 1024 being 1 to 1 gain, thus 512 is 0.5 gain, 2048 is double, etc
#define EESYS_ADC3_OFFSET        44  //2 bytes - ADC offset from zero - ADC reads 12 bit so the offset will be [0,4095] - Offset is subtracted from read ADC value

#define EESYS_ENABLE_INPUT       45 //1 byte - 255 = no enable input required, digital input to enable GEVCU
#define EESYS_PRECHARGE_MILLIS   46 //2 bytes - milliseconds for precharge cycle
#define EESYS_HV_NEGATIVE_RELAY  47 //1 byte - 255 = no HV negative relay, digital output for HV negative contactor
#define EESYS_PRECHARGE_RELAY    48 //1 byte - 255 = no precharge relay 0-3 = yes, there is one (and the output is the number stored)
#define EESYS_HV_POSITIVE_RELAY  49 //1 byte - 255 = no contactor relay 0-3 = yes there is
#define EESYS_ENABLE_RELAY       50 //1 byte - 255 = no enable relay output, digital output for enable signal/relay
#define EESYS_COOLING_RELAY      51 //1 byte output controlling external cooling relay
#define EESYS_COOLING_TEMP_ON    52 //1 bytes temperature at which external cooling is switched on
#define EESYS_COOLING_TEMP_OFF   53 //1 byte temperature at which external cooling is switched off
#define EESYS_BRAKE_LIGHT        54 //1 byte - 255 = no brake light output, digital output for brake light at regen
#define EESYS_REVERSE_LIGHT      55 //1 byte - 255 = no reverse light output, digital output for reverse light

#define EESYS_CAN0_BAUD          100 //2 bytes - Baud rate of CAN0 in 1000's of baud. So a value of 500 = 500k baud. Set to 0 to disable CAN0
#define EESYS_CAN1_BAUD          102 //2 bytes - Baud rate of CAN1 in 1000's of baud. So a value of 500 = 500k baud. Set to 0 to disable CAN1
#define EESYS_SERUSB_BAUD        104 //2 bytes - Baud rate of serial debugging port. Multiplied by 10 to get baud. So 115200 baud will be set as 11520
#define EESYS_TWI_BAUD           106 //2 bytes - Baud for TWI in 1000's just like CAN bauds. So 100k baud is set as 100
#define EESYS_TICK_RATE          108 //2 bytes - # of system ticks per second. Can range the full 16 bit value [1, 65536] which yields ms rate of [15us, 1000ms]

//We store the current system time from the RTC in EEPROM every so often.
//RTC is not battery backed up on the Due so a power failure will reset it.
//These storage spaces let the firmware reload the last knowm time to bootstrap itself
//as much as possible. The hope is that we'll be able to get access to internet eventually
//and use NTP to get the real time.
#define EESYS_RTC_TIME           150 //4 bytes - BCD packed version of the current time in the same format as it is stored in RTC chip
#define EESYS_RTC_DATE           154 //4 bytes - BCD version of date in format of RTC chip

//Technically there are two different canbus systems in use. The MCP2515 has 2 masks and 5 filters. The Arduino DUE
//Has 8 masks and 8 filters potentially (not really, you do need transmit boxes too). So, the most masks and filters
//we could ever set is 7 (using one mb as transmit) so support accordingly.

#define EESYS_CAN_RX_COUNT       199 //1 byte - how many mailboxes to use for RX on the Due. On the Macchina it is always 5.
#define EESYS_CAN_MASK0          200 //4 bytes - first canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER0        204 //4 bytes - first canbus filter - uses mask 0 on Due and Macchina
#define EESYS_CAN_MASK1          208 //4 bytes - second canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER1        212 //4 bytes - second canbus filter - uses mask 0 on Macchina, Mask 1 on Due
#define EESYS_CAN_MASK2          216 //4 bytes - third canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER2        220 //4 bytes - third canbus filter - uses mask 1 on Macchina, Mask 2 on Due
#define EESYS_CAN_MASK3          224 //4 bytes - fourth canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER3        228 //4 bytes - fourth canbus filter - uses mask 1 on Macchina, Mask 3 on Due
#define EESYS_CAN_MASK4          232 //4 bytes - fifth canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER4        236 //4 bytes - fifth canbus filter - uses mask 1 on Macchina, Mask 4 on Due
#define EESYS_CAN_MASK5          240 //4 bytes - sixth canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER5        244 //4 bytes - sixth canbus filter - not valid on Macchina, Mask 5 on Due
#define EESYS_CAN_MASK6          248 //4 bytes - seventh canbus mask - bit 31 sets whether it is extended or not (set = extended)
#define EESYS_CAN_FILTER6        252 //4 bytes - seventh canbus filter - not valid on Macchina, Mask 6 on Due

//Allow for a few defined WIFI SSIDs that the GEVCU will try to automatically connect to.
#define EESYS_WIFI0_SSID     300 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI0_CHAN         332 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI0_DHCP         333 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI0_MODE         334 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI0_IPADDR       335 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI0_KEY          339 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_WIFI1_SSID     400 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI1_CHAN         432 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI1_DHCP     433 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI1_MODE         434 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI1_IPADDR       435 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI1_KEY          439 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_WIFI2_SSID        500 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFI2_CHAN        532 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFI2_DHCP        533 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFI2_MODE        534 //1 byte - 0 = B, 1 = G
#define EESYS_WIFI2_IPADDR      535 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFI2_KEY         539 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

//If the above networks can't be joined then try to form our own adhoc network
//with the below parameters.
#define EESYS_WIFIX_SSID        579 //32 bytes - the SSID to create or use (prefixed with ! if create ad-hoc)
#define EESYS_WIFIX_CHAN        611 //1 byte - the wifi channel (1 - 11) to use
#define EESYS_WIFIX_DHCP        612 //1 byte - DHCP mode, 0 = off, 1 = server, 2 = client
#define EESYS_WIFIX_MODE        613 //1 byte - 0 = B, 1 = G
#define EESYS_WIFIX_IPADDR      614 //4 bytes - IP address to use if DHCP is off
#define EESYS_WIFIX_KEY         618 //40 bytes - the security key (13 bytes for WEP, 8 - 83 for WPA but only up to 40 here

#define EESYS_LOG_LEVEL         658 //1 byte - the log level
#define EESYS_AMPHOURS          659 //1 byte - ???
#define EESYS_BRAKELIGHT        660 //1 byte - 
#define EESYS_xxxx              661 //1 byte - 

#endif

