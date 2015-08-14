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

/*
Format these fault codes in DTC format.
DTC uses 16 bits to store a code
Byte A is the first byte, B is the second byte
A7-6 = Which fault system (00=P=PowerTrain, 01=C=Chassis, 10=B=Body, 11=U=Network)
A5-4 = First Digit (0-3)
A3-0 = Second Digit (0-F)
B7-4 = Third Digit (0-F)
B3-0 = Last / Fourth Digit (0-F)

Many codes are already used in ICE vehicles but rarely with hex digits so exclusively use
codes with hex digits to avoid overlap
*/

enum FAULTCODE {
	FAULT_NONE = 0,

	//The A throttle seems to be stuck low
	FAULT_THROTTLE_LOW_A = 0x0F01, //P0F01 

	//The A throttle seems to be stuck high
	FAULT_THROTTLE_HIGH_A = 0x0F02, //P0F02

	//The B throttle seems to be stuck low
	FAULT_THROTTLE_LOW_B = 0x0F06, //P0F06

	//The B throttle seems to be stuck high
	FAULT_THROTTLE_HIGH_B = 0x0F07, //P0F07

	//The C throttle seems to be stuck low
	FAULT_THROTTLE_LOW_C = 0x0F0A, //P0F0A

	//The C throttle seems to be stuck high
	FAULT_THROTTLE_HIGH_C = 0x0F0B, //P0F0B

	//Throttles A and B do not match
	FAULT_THROTTLE_MISMATCH_AB = 0x0F11, //P0F11

	//Throttles A and C do not match
	FAULT_THROTTLE_MISMATCH_AC = 0x0F12, //P0F12

	//Throttles B and C do not match
	FAULT_THROTTLE_MISMATCH_BC = 0x0F13, //P0F13

	//There was a general fault in the throttle
	FAULT_THROTTLE_MISC = 0x0F40, //P0F40

	//There was a problem communicating with an external throttle module
	FAULT_THROTTLE_COMM = 0x0F60, //P0F60

	//The brake input seems to be stuck low
	FAULT_BRAKE_LOW = 0x0B01, //P0B01

	//The brake input seems to be stuck high
	FAULT_BRAKE_HIGH = 0x0B02, //P0B02

	//There was a problem communicating with an external brake module
	FAULT_BRAKE_COMM = 0x0B60, //P0B60
	
	//The +12V battery or DC/DC system seems to have excessively high voltage
	FAULT_12V_BATT_HIGH = 0x0A01, //P0A01
	
	//The +12V battery or DC/DC system seems to have excessively low voltage
	FAULT_12V_BATT_LOW = 0x0A02, //P0A02
	
	//The HV battery has too high of a voltage
	FAULT_HV_BATT_HIGH = 0x0A11, //P0A11
	
	//The HV battery has too low of a voltage
	FAULT_HV_BATT_LOW = 0x0A12, //P0A12
	
	//A request has been made for more current than allowable from the HV battery
	FAULT_HV_BATT_OVERCURR = 0x0A30, //P0A30
	
	//The HV battery is too cold
	FAULT_HV_BATT_UNDERTEMP = 0x0A50, //P0A50

	//The HV battery is too hot
	FAULT_HV_BATT_OVERTEMP = 0x0A51, //P0A51
	
	//There is a problem with isolation of HV from the chassis
	FAULT_HV_BATT_ISOLATION = 0x0A70, //P0A70
	
	//A cell in the HV pack is too high of a voltage
	FAULT_HV_CELL_HIGH = 0x0AA1, //P0AA1
	
	//A cell in the HV pack is too low of a voltage
	FAULT_HV_CELL_LOW = 0x0AA2, //P0AA2
	
	//A cell in the HV pack is too cold
	FAULT_HV_CELL_UNDERTEMP = 0x0AB0, //P0AB0
	
	//A cell in the HV pack is too hot
	FAULT_HV_CELL_OVERTEMP = 0x0AB1, //P0AB1

	//BMS failed to initialize properly
	FAULT_BMS_INIT = 0xCC10, //U0C10 

	//There was a problem communicating with the BMS
	FAULT_BMS_COMM = 0xCC60, //U0C60 
	
	//There was a general fault at the BMS
	FAULT_BMS_MISC = 0xCC40, //U0C40
	
	//The motor is too hot
	FAULT_MOTOR_OVERTEMP = 0x0D50, //P0D50 
	
	//The motor controller is too hot
	FAULT_MOTORCTRL_OVERTEMP = 0x0D80, //P0D80
	
	//There was a problem communicating with the motor controller
	FAULT_MOTORCTRL_COMM = 0x0D60, //P0D60
	
	//There was a general fault in the motor controller
	FAULT_MOTORCTRL_MISC = 0x0D40, //P0D40
	
	//The motor and controller cannot satisfy the torque request
	FAULT_MOTORCTRL_INSUFF_TORQUE = 0x0D10, //P0D10 
	
	//The motor is spinning faster that it should be
	FAULT_MOTORCTRL_EXCESSIVE_SPEED = 0x0D20, //P0D20
	
	//The motor is providing more torque than it should be (didn't request this much)
	FAULT_MOTORCTRL_EXCESSIVE_TORQUE = 0x0D25, //P0D25
	
	//It seems that we cannot enable the motor controller though we've tried.
	FAULT_MOTORCTRL_CANT_ENABLE = 0x0DA0, //P0DA0
	
	//It seems we cannot go into drive even though we asked to go into drive
	FAULT_MOTORCTRL_CANT_DRIVE = 0x0DA4, //P0DA4
	
	//It seems we cannot go into reverse even though we asked to go into reverse
	FAULT_MOTORCTRL_CANT_REVERSE = 0x0DA8, //P0DA8
	
	//there was a request for power from the motor but the controller is not in a state to provide power
	FAULT_MOTORCTRL_POWERREQ_INV = 0x0DB0  //P0DB0
};


#endif


