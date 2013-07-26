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

#define CFG_TIMER_MAX_TICKABLES 9 // the maximum number of supported tickables per timer
#define CFG_DEV_MGR_MAX_DEVICES 9 // the maximum number of devices supported by the DeviceManager

// Define the devices which are to be used (and compiled) via the following lines. It is advisable
// to define only one of any device type (e.g. only DMOC or Brusa not both, only one throttle type for
// accelerator)

#define CFG_ENABLE_DEVICE_HEARTBEAT
#define CFG_ENABLE_DEVICE_POT_THROTTLE_ACCEL
//#define CFG_ENABLE_DEVICE_CAN_THROTTLE_ACCEL
#define CFG_ENABLE_DEVICE_POT_THROTTLE_BRAKE
#define CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
//#define CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5

// specify the intervals (microseconds) at which each device type should be "ticked"
// try to use the same numbers for several devices because then they will share
// the same timer (out of a limited number of 9 timers).

#define CFG_TICK_INTERVAL_HEARTBEAT 2000000
#define CFG_TICK_INTERVAL_POT_THROTTLE 10000
#define CFG_TICK_INTERVAL_CAN_THROTTLE 200000 // tick CanThrottle every 200ms
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER 10000
#define CFG_TICK_INTERVAL_MEM_CACHE 10000

#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus
#define CFG_CAN0_NUM_RX_MAILBOXES 5 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 6 // amount of CAN bus receive mailboxes for CAN1
#define CFG_CAN0_NUM_TX_MAILBOXES 3 // amount of CAN bus transmit mailboxes for CAN0
#define CFG_CAN1_NUM_TX_MAILBOXES 2 // amount of CAN bus transmit mailboxes for CAN1
#define CFG_CAN_MAX_DEVICES_PER_MAILBOX 8 // maximum number of devices per CAN mailbox

#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault
#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

//if this is defined then the ADC code will use raw readings from the actual ADC port of that number.
//In other words, no DMA, no differential input, just the ADC. If you ask for ADC0 you will get a raw
//reading from ADC0.
//#define RAWADC

#endif /* CONFIG_H_ */
