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

Layout :

Range EE_DEVICES_TABLE to EE_DEVICES_TABLE + (EE_NUM_DEVICES + 1) * 2 - 1
 0000-0001 : GEVCU marker (0xDEAD)
 0002-0003 : ID of device 1 (enabled if bit 0x8000 of ID is set)
 0004-0005 : ID of device 2 (enabled if bit 0x8000 of ID is set)
 0006-0007 : ID of device 3 (enabled if bit 0x8000 of ID is set)
 0008-0009 : ID of device 4 (enabled if bit 0x8000 of ID is set)
 ...
 0126-0127 : ID of device 63 (enabled if bit 0x8000 of ID is set)

Range EE_DEVICES_TABLE + (EE_NUM_DEVICES + 1) * 2 to EE_DEVICES_BASE - 1
 0128-1023 : unused

Range EE_DEVICES_BASE to EE_DEVICES_BASE + EE_NUM_DEVICES * EE_DEVICE_SIZE - 1
 1024-1535 : config device 1 (first byte = checksum)
 1536-2047 : config device 2 (first byte = checksum)
 2048-2559 : config device 3 (first byte = checksum)
 ...
 32768-33279 : config device 63 (first byte = checksum)

Range
 33280-34815 : unused

Range EE_LKG_OFFSET to LGK_OFFSET + EE_NUM_DEVICES * EE_DEVICE_SIZE - 1
 34816-35327 : lkg config device 1 (first byte = checksum)
 35328-35839 : lkg config device 2 (first byte = checksum)
 ...
 66560-67071 : lkg config device 63 (first byte = checksum)

Range EE_SYS_LOG to EE_FAULT_LOG - 1
 69632-102399 : system log

Range EE_FAULT_LOG to eeprom size - 1 ?
 102400-...

*/
#define EE_DEVICE_TABLE     0 //where is the table of devices found in EEPROM?
#define EE_NUM_DEVICES		63 // the number of supported device entries
#define EE_GEVCU_MARKER     0xDEAD // marker at position 0 to identify EEPROM was initialized

#define EE_DEVICE_SIZE      512 //# of bytes allocated to each device
#define EE_DEVICES_BASE     1024 //start of where devices in the table can use

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
#define EE_CHECKSUM                         0 //1 byte - checksum for this section of EEPROM to makesure it is valid

// Motor controller data
#define EEMC_MAX_RPM                        20 //2 bytes, unsigned int for maximum allowable RPM
#define EEMC_MAX_TORQUE                     22 //2 bytes, unsigned int - maximum torque in tenths of a Nm
#define EEMC_NOMINAL_V                      46 //2 bytes - nominal system voltage to expect (in tenths of a volt)
#define EEMC_REVERSE_LIMIT                  48 //2 bytes - a percentage to knock the requested torque down by while in reverse.
#define EEMC_UNUSED1                        50 //1 byte - unused
#define EEMC_UNUSED2                        51 // 1 byte - unused
#define EEMC_SLEW_RATE                      52 // 2 bytes - slew rate
#define EEMC_MAX_MECH_POWER_MOTOR           54 // 2 bytes - max mechanical power motoring in 4W steps
#define EEMC_MAX_MECH_POWER_REGEN           56 // 2 bytes - max mechanical power regen in 4W steps
#define EEMC_DC_VOLT_LIMIT_MOTOR            58 // 2 bytes - DC volt limit for motoring in 0.1V
#define EEMC_DC_VOLT_LIMIT_REGEN            60 // 2 bytes - DC volt limit for regen in 0.1V
#define EEMC_DC_CURRENT_LIMIT_MOTOR         62 // 2 bytes - DC current limit for motoring in 0.1A
#define EEMC_DC_CURRENT_LIMIT_REGEN         64 // 2 bytes - DC current limit for regen in 0.1A
#define EEMC_OSCILLATION_LIMITER            66 // 1 byte - flag to enable oscillation limiter (1=true/0=false)
#define EEMC_INVERT_DIRECTION               67 // 1 byte - flag to indicate if the motor's direction should be inverted
#define EEMC_POWER_MODE                     68 // 1 byte - speed or torque mode
#define EEMC_CREEP_LEVEL                    69 // 1 byte - percentage of throttle used to simulate creep
#define EEMC_CREEP_SPEED                    70 // 2 bytes - max speed for creep
#define EEMC_BRAKE_HOLD                     72 // 1 byte - percentage of max torque to achieve brake hold (0=off)
#define EEMC_GEAR_CHANGE_SUPPORT            73 // 1 byte - flag, true if gear chaning support is enabled
#define EEMC_BRAKE_HOLD_COEFF               74 // 1 byte - brake hold force coefficient
#define EEMC_CRUISE_KP                      75 // 2 byte - Kp value for cruise control PID
#define EEMC_CRUISE_KI                      77 // 2 byte - Ki value for cruise control PID
#define EEMC_CRUISE_KD                      79 // 2 byte - Kd value for cruise control PID
#define EEMC_CRUISE_LONG_PRESS_DELTA        81 // 2 byte - delta to target speed when pressing +/- button long (kph/rpm)
#define EEMC_CRUISE_STEP_DELTA              83 // 2 byte - delta to actual speen wehn pressing +/- button a short time
#define EEMC_CRUISE_USE_RPM                 85 // 1 byte - flag if true, cruise control uses rpm, if false kph to control vehicle speed

// Throttle data
#define EETH_LEVEL_MIN                      20 //2 bytes - ADC value of minimum value for first channel
#define EETH_LEVEL_MAX                      22 //2 bytes - ADC value of maximum value for first channel
#define EETH_LEVEL_MIN_TWO                  24 //2 bytes - ADC value of minimum value for second channel
#define EETH_LEVEL_MAX_TWO                  26 //2 bytes - ADC value of maximum value for second channel
#define EETH_REGEN_MIN                      28 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where regen stops
#define EETH_FWD                            30 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion starts
#define EETH_MAP                            32 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where forward motion is at 50% throttle
#define EETH_BRAKE_MIN                      34 //2 bytes - ADC value of minimum value for brake input
#define EETH_BRAKE_MAX                      36 //2 bytes - ADC value of max value for brake input
#define EETH_MAX_ACCEL_REGEN                38 //2 bytes - maximum percentage of throttle to command on accel pedal regen
#define EETH_MAX_BRAKE_REGEN                40 //2 bytes - maximum percentage of throttle to command for braking regen. Starts at min brake regen and works up to here.
#define EETH_NUM_THROTTLES                  42 //1 byte - How many throttle inputs should we use? (1 or 2)
#define EETH_THROTTLE_TYPE                  43 //1 byte - Allow for different throttle types. For now 1 = Linear pots, 2 = Inverse relationship between pots. See Throttle.h
#define EETH_MIN_BRAKE_REGEN                44 //2 bytes - the starting level for brake regen as a percentage of throttle
#define EETH_MIN_ACCEL_REGEN                46 //2 bytes - the starting level for accelerator regen as a percentage of throttle
#define EETH_REGEN_MAX                      48 //2 bytes - unsigned int - tenths of a percent (0-1000) of pedal position where regen is at maximum
#define EETH_UNUSED                         50 //2 bytes -
#define EETH_CAR_TYPE                       52 //1 byte - type of car for querying the throttle position via CAN bus
#define EETH_ADC_1                          53 //1 byte - which ADC port to use for first throttle input
#define EETH_ADC_2                          54 //1 byte - which ADC port to use for second throttle input

// DC-DC converter data
#define EEDC_BOOST_MODE                     20 // 1 byte, boost mode = 1, buck mode = 0
#define EEDC_DEBUG_MODE                     21 // 1 byte, debug mode enabled
#define EEDC_LOW_VOLTAGE                    22 // 2 byte, low voltage
#define EEDC_HIGH_VOLTAGE                   24 // 2 byte, hich voltage
#define EEDC_HV_UNDERVOLTAGE_LIMIT          26 // 2 byte, HV undervoltage limit
#define EEDC_LV_BUCK_CURRENT_LIMIT          28 // 2 byte, LV buck current limit
#define EEDC_HV_BUCK_CURRENT_LIMIT          30 // 2 byte, HV buck current limit
#define EEDC_LV_UNDERVOLTAGE_LIMIT          32 // 2 byte, LV undervoltage limit
#define EEDC_LV_BOOST_CURRENT_LIMIT         34 // 2 byte, LV boost current limit
#define EEDC_HV_BOOST_CURRENT_LIMIT         36 // 2 byte, HV boost current limit

// Charger data
#define EECH_MAX_INPUT_CURRENT              20 // 2 bytes, max mains current in 0.1A
#define EECH_CONSTANT_CURRENT               22 // 2 bytes, constant current in 0.1A
#define EECH_CONSTANT_VOLTAGE               24 // 2 bytes, constant voltage in 0.1V
#define EECH_TERMINATE_CURRENT              26 // 2 bytes, terminate current in 0.1A
#define EECH_MIN_BATTERY_VOLTAGE            28 // 2 bytes, minimum battery voltage to start charging in 0.1V
#define EECH_MAX_BATTERY_VOLTAGE            30 // 2 bytes, maximum battery voltage to charge in 0.1V
#define EECH_MIN_BATTERY_TEMPERATURE        32 // 2 bytes, minimum battery temp to charge in 0.1deg C
#define EECH_MAX_BATTERY_TEMPERATURE        34 // 2 bytes, maximum battery temp to charge in 0.1deg C
#define EECH_MAX_AMPERE_HOURS               36 // 2 bytes, maximum Ah to charge in 0.1Ah
#define EECH_MAX_CHARGE_TIME                38 // 2 bytes, maximum charge time in minutes
#define EECH_DERATING_TEMPERATURE           40 // 2 bytes, 0.1Ah per deg Celsius
#define EECH_DERATING_REFERENCE             42 // 2 bytes, 0.1 deg Celsius where derating will reach 0 Amp
#define EECH_HYSTERESE_STOP                 44 // 2 bytes, temperature where charging is interrupted in 0.1deg C
#define EECH_HYSTERESE_RESUME               46 // 2 bytes, temperature where chargin is resumed in 0.1deg C

// System I/O
#define EESIO_SYSTEM_TYPE                   10 //1 byte - 1 = Old school protoboards 2 = GEVCU2/DUED 3 = GEVCU3 - Defaults to 2 if invalid or not set up
#define EESIO_LOG_LEVEL                     11 //1 byte - the log level
#define EESIO_ENEGRY_CONSUMPTION            12 //4 bytes - accumulated power consumption
#define EESIO_ENABLE_INPUT                  45 // 1 byte - digital input to enable GEVCU (255 = no input required)
#define EESIO_PRECHARGE_MILLIS              46 // 2 bytes - milliseconds for precharge cycle
#define EESIO_SECONDARY_CONTACTOR_OUTPUT    48 // 1 byte - digital output for secondary contactor (255 = no output)
#define EESIO_PRECHARGE_RELAY_OUTPUT        49 // 1 byte - digital output for pre-charge relay (255 = no output)
#define EESIO_MAIN_CONTACTOR_OUTPUT         50 // 1 byte - digital output for main contactor (255 = no output)
#define EESIO_ENABLE_MOTOR_OUTPUT           51 // 1 byte - digital output to enable motor controller (255 = no output)
#define EESIO_COOLING_FAN_OUTPUT            52 // 1 byte - digital output to control external cooling relay (255 = no output)
#define EESIO_COOLING_TEMP_ON               53 // 1 byte - temperature at which external cooling is switched on
#define EESIO_COOLING_TEMP_OFF              54 // 1 byte - temperature at which external cooling is switched off
#define EESIO_BRAKE_LIGHT_OUTPUT            55 // 1 byte - digital output for brake light at regen (255 = no output)
#define EESIO_REVERSE_LIGHT_OUTPUT          56 // 1 byte - digital output for reverse light (255 = no output)
#define EESIO_INTERLOCK_INPUT               57 // 1 byte - digital input for interlock signal (255 = no input)
#define EESIO_CHARGE_POWER_AVAILABLE_INPUT  58 // 1 byte - digital input for charge power available signal (255 = no input)
#define EESIO_FAST_CHARGE_CONTACTOR_OUTPUT  59 // 1 byte - digital output for fast charge contactor (255 = no output)
#define EESIO_ENABLE_CHARGER_OUTPUT         60 // 1 byte - digital output for charger enable signal (255 = no output)
#define EESIO_ENABLE_DCDC_OUTPUT            61 // 1 byte - digital output for DCDC converter enable signal (255 = no output)
#define EESIO_ENABLE_HEATER_OUTPUT          62 // 1 byte - digital output for heater enable signal (255 = no output)
#define EESIO_HEATER_VALVE_OUTPUT           63 // 1 byte - digital output for heater valve signal (255 = no output)
#define EESIO_HEATER_PUMP_OUTPUT            64 // 1 byte - digital output for heater pump relay (255 = no output)
#define EESIO_COOLING_PUMP_OUTPUT           65 // 1 byte - digital output for cooling pump relay (255 = no output)
#define EESIO_WARNING_OUTPUT                66 // 1 byte - digital output for warning signal (255 = no output)
#define EESIO_POWER_LIMITATION_OUTPUT       67 // 1 byte - digital output for power limitation signal (255 = no output)
#define EESIO_REVERSE_INPUT                 68 // 1 byte - digital input for reverse signal (255 = no input)
#define EESIO_POWER_STEERING_OUTPUT         69 // 1 byte - digital output for power steering
#define EESIO_UNUSED_OUTPUT                 70 // 1 byte - digital output for ...
#define EESIO_STATE_OF_CHARGE_OUTPUT        71 // 1 byte - digital output for indication of SoC (255 = no output)
#define EESIO_GEAR_CHANGE_INPUT             72 // 1 byte - digital input for gear change signal (255 = no output)
#define EESIO_STATUS_LIGHT_OUTPUT           73 // 1 byte - digital oupt for PWM operated status light
#define EESIO_HEATER_TEMPERATURE_ON         74 // 1 byte - temp in deg C where heater is enabled
#define EESIO_ABS_INPUT                     75 // 1 byte - digital input for ABS signal (255 = no output)

// CanOBD2
#define EEOBD2_CAN_BUS_RESPOND              10 // 1 byte - which can bus should we respond to OBD2 requests (0=ev, 1=car, 255=ignore)
#define EEOBD2_CAN_ID_OFFSET_RESPOND        11 // 1 byte - offset for can id on wich we listen to incoming requests (0-7)
#define EEOBD2_CAN_BUS_POLL                 12 // 1 byte - which can bus should we poll OBD2 data from (0=ev, 1=car, 255=ignore)
#define EEOBD2_CAN_ID_OFFSET_POLL           13 // 1 byte - offset for can id on which we will request OBD2 data from (0-7, 255=broadcast)

// Fault Handler
#define EEFAULT_VALID                       0 //1 byte - Set to value of 0xB2 if fault data has been initialized
#define EEFAULT_READPTR                     1 //2 bytes - index where reading should start (first unacknowledged fault)
#define EEFAULT_WRITEPTR                    3 //2 bytes - index where writing should occur for new faults
#define EEFAULT_RUNTIME                     5 //4 bytes - stores the number of seconds (in tenths) that the system has been turned on for - total time ever
#define EEFAULT_FAULTS_START                10 //a bunch of faults stored one after the other start at this location

#endif
