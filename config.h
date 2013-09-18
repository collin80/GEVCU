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
 *      Author: Michael Neuweiler
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <due_can.h>

#define CFG_BUILD_NUM	1004        //increment this every time a git commit is done. 

#define CFG_VERSION "GEVCU alpha 12 Sept 2013"
#define CFG_SERIAL_SPEED 115200


#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb

#define CFG_DEV_MGR_MAX_DEVICES 9 // the maximum number of devices supported by the DeviceManager
#define CFG_CAN_NUM_OBSERVERS 10 // maximum number of device subscriptions per CAN bus
#define CFG_TIMER_NUM_OBSERVERS 9 // the maximum number of supported observers per timer
#define CFG_TIMER_USE_QUEUING // if defined, TickHandler uses a queuing buffer instead of direct calls from interrupts
#define CFG_TIMER_BUFFER_SIZE 100 // the size of the queuing buffer for TickHandler

// Define the devices which are to be used (and compiled) via the following lines. It is advisable
// to define only one of any device type (e.g. only DMOC or Brusa not both, only one throttle type for
// accelerator)

#define CFG_ENABLE_DEVICE_HEARTBEAT
#define CFG_ENABLE_DEVICE_POT_THROTTLE
//#define CFG_ENABLE_DEVICE_CAN_THROTTLE
#define CFG_ENABLE_DEVICE_POT_BRAKE
#define CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
//#define CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
//#define CFG_ENABLE_DEVICE_ICHIP2128_WIFI
//#define CFG_ENABLE_DEVICE_BMS_THINK

// specify the intervals (microseconds) at which each device type should be "ticked"
// try to use the same numbers for several devices because then they will share
// the same timer (out of a limited number of 9 timers).

//Switched pot throttle, motor controller, and memcache to 40ms ticks
//That's still quite fast (25 times per second). This times the cache out
//in around 4-5 seconds and still is plenty fast to keep the DMOC happy
#define CFG_TICK_INTERVAL_HEARTBEAT 2000000
#define CFG_TICK_INTERVAL_POT_THROTTLE 40000
#define CFG_TICK_INTERVAL_CAN_THROTTLE 50000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC 40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA 20000
#define CFG_TICK_INTERVAL_MEM_CACHE 40000
#define CFG_TICK_INTERVAL_BMS_THINK	500000
#define CFG_TICK_INTERVAL_WIFI		2000000

#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN1

#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault - 1/10% each #
#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX

// Throttle pins/ADC ports
#define CFG_THROTTLE_NONE	255
#define CFG_THROTTLE1_PIN	0
#define CFG_THROTTLE2_PIN	1
#define CFG_BRAKE_PIN		2

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	4

//if this is defined then the ADC code will use raw readings from the actual ADC port of that number.
//In other words, no DMA, no differential input, just the ADC. If you ask for ADC0 you will get a raw
//reading from ADC0.
//#define RAWADC

//define this if using the new DUED boards. These have ampseal connectors
#define DUED

#endif /* CONFIG_H_ */
