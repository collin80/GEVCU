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

#define CFG_VERSION "GEVCU alpha 2013-07-11"
#define CFG_SERIAL_SPEED 115200

#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

#define CFG_MAX_TICK_DEVICES 16 // the maximum number of supported tickable devices

#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus

#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault

#endif /* CONFIG_H_ */
