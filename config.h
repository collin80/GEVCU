/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.
 *
 *  Created on: 25.04.2013
 *      Author: Michael Neuweiler
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <due_can.h>

#define CFG_VERSION "GEVCU alpha 2013-07-14"
#define CFG_SERIAL_SPEED 115200

#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

#define CFG_MAX_DEVICES 9 // the maximum number of supported tickable devices (limited by available timers)

#define CFG_ENABLE_DEVICE_HEARTBEAT
#define CFG_ENABLE_DEVICE_POT_THROTTLE_ACCEL
//#define CFG_ENABLE_DEVICE_CAN_THROTTLE_ACCEL
#define CFG_ENABLE_DEVICE_POT_THROTTLE_BRAKE
#define CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
//#define CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5

#define CFG_TICK_INTERVAL_HEARTBEAT 2000000
#define CFG_TICK_INTERVAL_POT_THROTTLE 10000
#define CFG_TICK_INTERVAL_CAN_THROTTLE 200000 // tick CanThrottle every 200ms
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER 10000
#define CFG_TICK_INTERVAL_MEM_CACHE 10000

#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus

#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault
#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

#endif /* CONFIG_H_ */
