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

#define CFG_BUILD_NUM	1003        //increment this every time a git commit is done. 
#define CFG_VERSION "GEVCU alpha 2013-09-15"


/*
 * SERAIL CONFIGURATION
 */
#define CFG_SERIAL_SPEED 115200
#define SerialUSB Serial // re-route serial-usb output to programming port ;) comment if output should go to std usb


/*
 * DEVICE DEFINITION
 *
 * Define the devices which are to be used (and compiled) via the following lines
 * Uncomment only one device per device type.
 */
#define CFG_ENABLE_DEVICE_HEARTBEAT
#define CFG_ENABLE_DEVICE_POT_THROTTLE
//#define CFG_ENABLE_DEVICE_CAN_THROTTLE
//#define CFG_ENABLE_DEVICE_POT_BRAKE
#define CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
//#define CFG_ENABLE_DEVICE_MOTORCTRL_BRUSA_DMC5
//#define CFG_ENABLE_DEVICE_ICHIP2128_WIFI
//#define CFG_ENABLE_DEVICE_BMS_THINK


/*
 * TIMER INTERVALS
 *
 * specify the intervals (microseconds) at which each device type should be "ticked"
 * try to use the same numbers for several devices because then they will share
 * the same timer (out of a limited number of 9 timers).
 */
#define CFG_TICK_INTERVAL_HEARTBEAT					2000000
#define CFG_TICK_INTERVAL_POT_THROTTLE				40000
#define CFG_TICK_INTERVAL_CAN_THROTTLE				40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC		40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA	20000
#define CFG_TICK_INTERVAL_MEM_CACHE					40000
#define CFG_TICK_INTERVAL_BMS_THINK					500000
#define CFG_TICK_INTERVAL_WIFI						2000000


/*
 * CAN BUS CONFIGURATION
 */
#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN1


/*
 * MISCELLANEOUS
 *
 */
#define CFG_THROTTLE_TOLERANCE  30 //the max that things can go over or under the min/max without fault - 1/10% each #


/*
 * HARD CODED PARAMETERS
 *
 * If USE_HARD_CODED is defined, the parameter values defined here are used
 * instead of those stored in the EEPROM.
 */
//#define USE_HARD_CODED
#define ThrottleNumPots			1		//# of pots to use by default
#define ThrottleSubtype			1		//subtype 1 is a standard linear pot throttle
#define ThrottleRegenValue		0		//where does Regen stop (1/10 of percent)
#define ThrottleFwdValue		175		//where does forward motion start
#define ThrottleMapValue		665		//Where is the 1/2 way point for throttle
#define ThrottleMaxRegenValue	00		//how many percent of full regen to do with accel pedal
#define ThrottleMaxErrValue		75		//tenths of percentage allowable deviation between pedals
#define Throttle1MinValue		180		//Value ADC reads when pedal is up
#define Throttle1MaxValue		930		//Value ADC reads when pedal fully depressed
#define Throttle2MinValue		360		//Value ADC reads when pedal is up
#define Throttle2MaxValue		1900	//Value ADC reads when pedal fully depressed
#define BrakeMinValue			100		//Value ADC reads when brake is not pressed
#define BrakeMaxValue			500		//Value ADC reads when brake is pushed all of the way down
#define BrakeMaxRegenValue		40		//percent of full power to use for brake regen (max)

#define MaxTorqueValue		2000 //in tenths of a Nm
#define	 MaxRPMValue		6000 //DMOC will ignore this but we can use it ourselves for limiting
#define PrechargeC			11000 //approximate C of DMOC input
#define PrechargeR			500 //a stupidly high resistance just to make sure we precharge long enough
#define NominalVolt			3300 //a reasonable figure for a lithium cell pack driving the DMOC (in tenths of a volt)
#define PrechargeRelay		3 //third output
#define MainContactorRelay	4 //fourth output

#define MaxRegenWatts	20000 //in actual watts, there is no scale here
#define MaxAccelWatts	150000


/*
 * ARRAY SIZE
 *
 * Define the maximum number of various object lists.
 * These values should normally not be changed.
 */
#define CFG_DEV_MGR_MAX_DEVICES 9 // the maximum number of devices supported by the DeviceManager
#define CFG_CAN_NUM_OBSERVERS 10 // maximum number of device subscriptions per CAN bus
#define CFG_TIMER_NUM_OBSERVERS 9 // the maximum number of supported observers per timer
#define CFG_TIMER_USE_QUEUING // if defined, TickHandler uses a queuing buffer instead of direct calls from interrupts
#define CFG_TIMER_BUFFER_SIZE 100 // the size of the queuing buffer for TickHandler


/*
 * PIN ASSIGNMENT
 */
#define CFG_THROTTLE_NONE	255
#define CFG_THROTTLE1_PIN	0
#define CFG_THROTTLE2_PIN	1
#define CFG_BRAKE_PIN		2
#define BLINK_LED          73 //13 is L, 73 is TX, 72 is RX
#define DUED //define this if using the new DUED boards. These have ampseal connectors

#define NUM_ANALOG	4
#define NUM_DIGITAL	4
#define NUM_OUTPUT	4


/*
 * DEBUGING
 */
//if this is defined then the ADC code will use raw readings from the actual ADC port of that number.
//In other words, no DMA, no differential input, just the ADC. If you ask for ADC0 you will get a raw
//reading from ADC0.
//#define RAWADC

#endif /* CONFIG_H_ */
