/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 * To differentiate between different platforms, the pre-compiler definitions __arm__
 * (Arduino Due) and __AVR__ (Arduino Uno based Machina) are used to compile the
 * machine dependent code.
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
#define CFG_DUE_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus
#define CFG_DUE_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus

#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault

/*
 * specify if the webserver should be included in the build
 */
#define CFG_WEBSERVER_ENABLED // comment out if no webserver is desired

#endif /* CONFIG_H_ */
