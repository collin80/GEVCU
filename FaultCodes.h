/*
 * config.h
 *
 * Defines the allowable fault codes

 Copyright (c) 2014 Collin Kidder, Michael Neuweiler, Charles Galpin, Jack Rickard

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
 *      
 */

#ifndef FAULTCODES_H_
#define FAULTCODES_H_

enum FAULTCODE {
	FAULT_NONE,
	FAULT_THROTTLE_LOW_A, //The A throttle seems to be stuck low
	FAULT_THROTTLE_HIGH_A, //The A throttle seems to be stuck high
	FAULT_THROTTLE_LOW_B, //The B throttle seems to be stuck low
	FAULT_THROTTLE_HIGH_B, //The B throttle seems to be stuck high
	FAULT_THROTTLE_LOW_C, //The C throttle seems to be stuck low
	FAULT_THROTTLE_HIGH_C, //The C throttle seems to be stuck high
	FAULT_THROTTLE_MISMATCH_AB, //Throttles A and B do not match
	FAULT_THROTTLE_MISMATCH_AC, //Throttles A and C do not match
	FAULT_THROTTLE_MISMATCH_BC, //Throttles B and C do not match
	FAULT_THROTTLE_MISC, //There was a general fault in the throttle
	FAULT_THROTTLE_COMM, //There was a problem communicating with an external throttle module
	FAULT_BRAKE_LOW, //The brake input seems to be stuck low
	FAULT_BRAKE_HIGH, //The brake input seems to be stuck high
	FAULT_BRAKE_COMM, //There was a problem communicating with an external brake module
	FAULT_12V_BATT_HIGH, //The +12V battery or DC/DC system seems to have excessively high voltage
	FAULT_12V_BATT_LOW, //The +12V battery or DC/DC system seems to have excessively low voltage
	FAULT_HV_BATT_HIGH, //The HV battery has too high of a voltage
	FAULT_HV_BATT_LOW, //The HV battery has too low of a voltage
	FAULT_HV_BATT_OVERCURR, //A request has been made for more current than allowable from the HV battery
	FAULT_HV_BATT_UNDERTEMP, //The HV battery is too cold
	FAULT_HV_BATT_OVERTEMP, //The HV battery is too hot
	FAULT_HV_BATT_ISOLATION, //There is a problem with isolation of HV from the chassis
	FAULT_HV_CELL_HIGH, //A cell in the HV pack is too high of a voltage
	FAULT_HV_CELL_LOW, //A cell in the HV pack is too low of a voltage
	FAULT_HV_CELL_UNDERTEMP, //A cell in the HV pack is too cold
	FAULT_HV_CELL_OVERTEMP, //A cell in the HV pack is too hot
	FAULT_BMS_COMM, //There was a problem communicating with the BMS
	FAULT_BMS_MISC, //There was a general fault at the BMS
	FAULT_MOTOR_OVERTEMP, //The motor is too hot
	FAULT_MOTORCTRL_OVERTEMP, //The motor controller is too hot
	FAULT_MOTORCTRL_COMM, //There was a problem communicating with the motor controller
	FAULT_MOTORCTRL_MISC, //There was a general fault in the motor controller
	FAULT_MOTORCTRL_INSUFF_TORQUE, //The motor and controller cannot satisfy the torque request
	FAULT_MOTORCTRL_EXCESSIVE_SPEED, //The motor is spinning faster that it should be
	FAULT_MOTORCTRL_EXCESSIVE_TORQUE, //The motor is providing more torque than it should be (didn't request this much)
	FAULT_MOTORCTRL_CANT_ENABLE, //It seems that we cannot enable the motor controller though we've tried.
	FAULT_MOTORCTRL_CANT_DRIVE, //It seems we cannot go into drive even though we asked to go into drive
	FAULT_MOTORCTRL_CANT_REVERSE, //It seems we cannot go into reverse even though we asked to go into reverse
	FAULT_MOTORCTRL_POWERREQ_INV  //there was a request for power from the motor but the controller is not in a state to provide power
};


#endif