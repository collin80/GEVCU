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

#include <due_can.h>

#define CFG_BUILD_NUM	1052        //increment this every time a git commit is done.
#define CFG_VERSION "GEVCU 2015-06-08"

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
#define CFG_TICK_INTERVAL_POT_THROTTLE              40000
#define CFG_TICK_INTERVAL_CAN_THROTTLE              40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER          40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC     40000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_CODAUQM  10000
#define CFG_TICK_INTERVAL_MOTOR_CONTROLLER_BRUSA    20000
#define CFG_TICK_INTERVAL_MEM_CACHE                 40000
#define CFG_TICK_INTERVAL_BMS_THINK                 500000
#define CFG_TICK_INTERVAL_DCDC_BSC6                 100000
#define CFG_TICK_INTERVAL_CHARGE_NLG5               100000
#define CFG_TICK_INTERVAL_WIFI                      200000
#define CFG_TICK_INTERVAL_SYSTEM_IO                 200000
#define CFG_TICK_INTERVAL_CAN_IO                    200000

/*
 * CAN BUS CONFIGURATION
 */
#define CFG_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus (EV)
#define CFG_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus (Car)
#define CFG_CAN0_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN0
#define CFG_CAN1_NUM_RX_MAILBOXES 7 // amount of CAN bus receive mailboxes for CAN1
#define CFG_CANTHROTTLE_MAX_NUM_LOST_MSG 3 // maximum number of lost messages allowed (max 255)
#define CFG_MOTORCTRL_MAX_NUM_LOST_MSG 20 // maximum number of ticks the controller may not send messages (max 255)

/*
 * MISCELLANEOUS
 *
 */
#define CFG_THROTTLE_TOLERANCE  150 //the max that things can go over or under the min/max without fault - 1/10% each #
#define CFG_PRE_CHARGE_RELAY_DELAY 200 // a delay to allow relays to (de-)activate before proceeding with next steps
#define CFG_PRE_CHARGE_START 1000 // delay for the pre-charge process to start - ensuring other deivces become available
#define CFG_THREE_CONTACTOR_PRECHARGE // do we use three contactors instead of two for pre-charge cycle ?
#define CFG_NO_TEMPERATURE_DATA 9999 // temperature used to indicate that no external temp sensor is connected
#define CFG_MIN_BATTERY_CHARGE_TEMPERATURE 5 // GEVCU won't start the battery charging process if the battery temp is below 5 deg C
#define CFG_TORQUE_BRAKE_LIGHT_ON -100 // torque in 0.1Nm where brake light should be turned on - to prevent being kissed from behind
#define CFG_WIFI_WPA2 // enable WPA2 encryption for ad-hoc wifi network at wifi reset (via command 'w'), comment line to disable

/*
 * HARD CODED PARAMETERS
 *
 * If USE_HARD_CODED is defined or the checksum of the parameters stored in EEPROM,
 * the parameter values defined here are used instead of those stored in the EEPROM.
 */
//#define USE_HARD_CODED
#define ThrottleNumPots         1       //# of pots to use by default
#define ThrottleADC1            0       //Which pin to use
#define ThrottleADC2            1       //Which pin to use
#define ThrottleSubtype         1       //subtype 1 is a standard linear pot throttle
#define ThrottleRegenMinValue	270		//where does Regen stop (1/10 of percent)
#define ThrottleRegenMaxValue	30		//where Regen is at maximum (1/10 of percent)
#define ThrottleFwdValue		300		//where does forward motion start
#define ThrottleMapValue		750		//Where is the 1/2 way point for throttle
#define ThrottleMinRegenValue	0		//how many percent of full power to use at minimal regen
#define ThrottleMaxRegenValue	50		//how many percent of full power to use at maximum regen
#define ThrottleCreepValue		0		//how many percent of full power to use at creep
#define ThrottleMaxErrValue		150		//tenths of percentage allowable deviation between pedals
#define Throttle1MinValue		95		//Value ADC reads when pedal is up
#define Throttle1MaxValue		3150		//Value ADC reads when pedal fully depressed
#define Throttle2MinValue		0		//Value ADC reads when pedal is up
#define Throttle2MaxValue		0	//Value ADC reads when pedal fully depressed
#define BrakeMinValue			100		//Value ADC reads when brake is not pressed
#define BrakeMaxValue			3200		//Value ADC reads when brake is pushed all of the way down
#define BrakeMinRegenValue		0		//percent of full power to use for brake regen (min)
#define BrakeMaxRegenValue		50		//percent of full power to use for brake regen (max)
#define BrakeADC                2       //which ADC pin to use


#define MaxTorqueValue		3000 //in tenths of a Nm
#define MaxRPMValue         6000 //DMOC will ignore this but we can use it ourselves for limiting
#define RPMSlewRateValue    0 // rpm/sec the requested speed should change (speed mode)
#define TorqueSlewRateValue 0 // 0.1Nm/sec the requested torque output should change (torque mode)
#define KilowattHrs         11000 //not currently used
#define EnableInput         255 // digital input port for enable signal
#define PrechargeMillis     3000 // milliseconds for pre-charge cycle
#define NominalVolt         3300 //a reasonable figure for a lithium cell pack driving the DMOC (in tenths of a volt)
#define PrechargeRelayOutput 4
#define MainContactorRelayOutput 5
#define SecondaryContactorRelayOutput 2
#define EnableRelayOutput   3
#define BrakeLightOutput    255
#define ReverseLightOutput  255
#define ReversePercent      50
#define CoolingFanRelayOutput  255  //output to use for cooling fan
#define CoolingTemperatureOn 40 //temperature (in C) to turn on cooling fan
#define CoolingTemperatureOff 35 //temperature to turn it off
#define InterlockInput 255


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
#define CFG_SERIAL_SEND_BUFFER_SIZE 100
#define CFG_FAULT_HISTORY_SIZE	50 //number of faults to store in eeprom. A circular buffer so the last 50 faults are always stored.

/*
 * PIN ASSIGNMENT
 */
#define CFG_OUTPUT_NONE    255
#define CFG_BLINK_LED       73 //13 is L, 73 is TX, 72 is RX

#define CFG_NUMBER_ANALOG_INPUTS  4
#define CFG_NUMBER_DIGITAL_INPUTS 4
#define CFG_NUMBER_DIGITAL_OUTPUTS  8
#define CFG_NUMBER_TEMPERATURE_SENSORS 8 // the maximum supported external temperature sensors

#endif /* CONFIG_H_ */
