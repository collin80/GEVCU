/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.

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

#ifndef CONFIG_H_
#define CONFIG_H_

#define CFG_BUILD_NUM	1070        //increment this every time a git commit is done.
#define CFG_VERSION "GEVCU 2020-04-11"
#define CFG_DEFAULT_LOGLEVEL Logger::Info

//define this to add in latency and efficiency calculations. Comment it out for builds you're going to 
//use in an actual car. No need to waste cycles for 99% of everyone using the code.
//#define CFG_EFFICIENCY_CALCS


/*
 * SERIAL CONFIGURATION
 */
#define CFG_SERIAL_SPEED 115200
//#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

/*
 * TIMER INTERVALS
 *
 * specify the intervals (microseconds) at which each device type should be "ticked"
 * try to use the same numbers for several devices because then they will share
 * the same timer (out of a limited number of 9 timers).
 */
#define CFG_TICK_INTERVAL_HEARTBEAT                 2000000
#define CFG_TICK_INTERVAL_POT_THROTTLE              100000
#define CFG_TICK_INTERVAL_CAN_THROTTLE              100000
#define CFG_TICK_INTERVAL_CAN_OBD2                  200000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC     40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_CODAUQM  10000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA    30000
#define CFG_TICK_INTERVAL_MEM_CACHE                 40000
#define CFG_TICK_INTERVAL_STATUS                    40000
#define CFG_TICK_INTERVAL_BMS_THINK                 500000
#define CFG_TICK_INTERVAL_BMS_ORION                 500000
#define CFG_TICK_INTERVAL_DCDC_BSC6                 100000
#define CFG_TICK_INTERVAL_CHARGE_NLG5               100000
#define CFG_TICK_INTERVAL_WIFI                      100000
#define CFG_TICK_INTERVAL_SYSTEM_IO                 200000
#define CFG_TICK_INTERVAL_CAN_IO                    200000

/*
 * CAN BUS CONFIGURATION
 */
#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_TX_MAILBOXES 2 // how many of 8 mailboxes are used for TX for CAN0, rest is used for RX
#define CFG_CAN1_NUM_TX_MAILBOXES 3 // how many of 8 mailboxes are used for TX for CAN1, rest is used for RX
#define CFG_CANTHROTTLE_MAX_NUM_LOST_MSG 5 // maximum number of lost messages allowed (max 255)
#define CFG_MOTORCTRL_MAX_NUM_LOST_MSG 20 // maximum number of ticks the controller may not send messages (max 255)

/*
 * MISCELLANEOUS
 *
 */
#define CFG_THROTTLE_TOLERANCE  150 //the max that things can go over or under the min/max without fault - 1/10% each #
#define CFG_TORQUE_BRAKE_LIGHT_ON -500 // torque in 0.1Nm where brake light should be turned on - to prevent being kissed from behind
#define CFG_BRAKE_HOLD_MAX_TIME 5000 // max amount of ms to apply the brake hold functionality
#define CFG_PRE_CHARGE_RELAY_DELAY 200 // a delay to allow relays to (de-)activate before proceeding with next steps
#define CFG_PRE_CHARGE_START 1000 // delay for the pre-charge process to start - ensuring other deivces become available
#define CFG_THREE_CONTACTOR_PRECHARGE // do we use three contactors instead of two for pre-charge cycle ?
#define CFG_NO_TEMPERATURE_DATA 9999 // temperature used to indicate that no external temp sensor is connected
#define CFG_MIN_BATTERY_CHARGE_TEMPERATURE 5 // GEVCU won't start the battery charging process if the battery temp is below 5 deg C
#define CFG_WIFI_WPA2 // enable WPA2 encryption for ad-hoc wifi network at wifi reset (via command 'w'), comment line to disable
#define CFG_CAN_TEMPERATURE_OFFSET 50 // offset of temperatures reported via CAN bus - make sure GEVCU extension uses the same value!
#define CFG_ADC_GAIN 1024 // ADC gain centered at 1024 being 1 to 1 gain, thus 512 is 0.5 gain, 2048 is double, etc
#define CFG_ADC_OFFSET 0 // ADC offset from zero - ADC reads 12 bit so the offset will be [0,4095] - Offset is subtracted from read ADC value
#define CFG_THROTTLE_MAX_ERROR 150 //tenths of percentage allowable deviation between pedals
#define CFG_WEBSOCKET_MAX_TIME 25 // maximum processing time when assembling websocket message (in ms) - prevents interruptions when sending messages to controller
#define CFG_CHARGED_SHUTDOWN_TIME 600000 // ms after status changed to charged when shutting-down the system

/*
 * HARD CODED PARAMETERS
 *
 * If USE_HARD_CODED is defined or the checksum of the parameters stored in EEPROM,
 */
//#define USE_HARD_CODED

/*
 * ARRAY SIZE
 *
 * Define the maximum number of various object lists.
 * These values should normally not be changed.
 */
#define CFG_DEV_MGR_MAX_DEVICES 20 // the maximum number of devices supported by the DeviceManager
#define CFG_CAN_NUM_OBSERVERS 10 // maximum number of device subscriptions per CAN bus
#define CFG_TIMER_NUM_OBSERVERS 9 // the maximum number of supported observers per timer
#define CFG_TIMER_BUFFER_SIZE 100 // the size of the queuing buffer for TickHandler
#define CFG_SERIAL_SEND_BUFFER_SIZE 120
#define CFG_FAULT_HISTORY_SIZE	50 //number of faults to store in eeprom. A circular buffer so the last 50 faults are always stored.
#define CFG_WEBSOCKET_BUFFER_SIZE 50 // number of characters an incoming socket frame may contain
#define CFG_WIFI_NUM_SOCKETS 4 // max number of websocket connections
#define CFG_WIFI_BUFFER_SIZE 1025 // size of buffer for incoming data from wifi
#define LOG_BUFFER_SIZE 120 // size of log output messages
#define CFG_LOG_REPEAT_MSG_TIME 10000 // ms while a repeated message is suppressed to be sent to the wifi
#define CFG_CRUISE_SPEED_BUFFER_SIZE 10 // size of the buffer for actual speed when using cruise buffer
#define CFG_CRUISE_BUTTON_LONG_PRESS 1000 // ms after which a button press is considered a long press (for plus/minus)
#define CFG_CRUISE_SIZE_SPEED_SET 8 // max number of speed set entries (cruise speed buttons in dashboard)

/*
 * PIN ASSIGNMENT
 */
#define CFG_OUTPUT_NONE    				255
#define CFG_IO_BLINK_LED       			13 // 13 is L, 73 is TX, 72 is RX
#define CFG_EEPROM_WRITE_PROTECT		18 // pin used to control the write-enable signal for the eeprom, (19=GEVCU >=3, 18=GEVCU 2.x)
#define CFG_WIFI_RESET                  17 // pin to reset wifi chip (18=GEVCU >=3, 17=GEVCU 2)
#define CFG_WIFI_ENABLE                 42 // pin used to enable wifi chip


#define CFG_NUMBER_ANALOG_INPUTS  4
#define CFG_NUMBER_DIGITAL_INPUTS 4
#define CFG_NUMBER_DIGITAL_OUTPUTS  8
#define CFG_NUMBER_BATTERY_TEMPERATURE_SENSORS 6 // the maximum supported external temperature sensors for battery

#endif /* CONFIG_H_ */
