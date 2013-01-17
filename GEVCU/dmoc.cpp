/*
 * dmoc.c
 *
 * Interface to the DMOC - Handles sending of commands and reception of status frames
 *
 * Created: 1/13/2013 6:10:00 PM
 *  Author: Collin
 */ 

#include "dmoc.h"

DMOC::DMOC(MCP2515 *canlib) {
	step = RPM;
	selectedGear = NEUTRAL;
	MaxTorque = 500; //in tenths so 50Nm max torque. This is plenty for testing
	MaxRPM = 2000; //also plenty for a bench test
	can = canlib;
}	
	
void DMOC::handleFrame(Frame& frame) {
		
}
	
/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
  rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
*/
void DMOC::handleTick() {
	switch (step) {
	case RPM:
		step = TORQUE;
		sendCmd1();
		break;
	case TORQUE:
		step = WATTS;
		sendCmd2();
		break;
	case WATTS:
		step = RPM;
		sendCmd3();
		break;	
	}
		
}
	
void DMOC::setThrottle(int throt) {
	requestedThrottle = throt;	
}

//Commanded RPM plus state of key and gear selector	
void DMOC::sendCmd1() {
	Frame output;
	static byte alive = 0;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x232;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.srr = 0;
	
	//Requested throttle goes negative to ask for regen but we don't actually want to command
	//the motor to spin the other direction. Only ever spin forwards for now. Eventually the gear
	//selection might require that negative RPMs be allowed but we might not use RPM control. TCE doesnt.
	//The TCE seems to always command just torque.
	if (requestedThrottle > 0) 
		requestedRPM = 20000 + (((long)requestedThrottle * (long)MaxRPM) / 1000);
	else
		requestedRPM = 20000;
	output.data[0] = (requestedRPM & 0xFF00) >> 8;
	output.data[1] = (requestedTorque & 0x00FF);
	output.data[2] = 0; //not used
	output.data[3] = 0; //not used
	output.data[4] = 0; //not used
	output.data[5] = ON;
	
	//Enable drive only if there is commanded throttle. Otherwise go to standby.
	if (requestedThrottle < -10 || requestedThrottle > 10) 
		output.data[6] = (ENABLE<<6) + (DRIVE<<4) + alive;
	else
		output.data[6] = (ENABLE<<6) + (STANDBY<<4) + alive;
		
	output.data[7] = calcChecksum(output);
	can->LoadBuffer(TXB0, output);
	can->SendBuffer(TXB0);	
}

//Torque limits
void DMOC::sendCmd2() {
	Frame output;
	static byte alive = 0;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x233;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.srr = 0;
	//30000 is the base point where torque = 0
	//MaxTorque is in tenths like it should be.
	//Requested throttle is [-1000, 1000] 
	//data 0-1 is upper limit, 2-3 is lower limit. They are set to same value to lock torque to this value
	requestedTorque = 30000 + (((long)requestedThrottle * (long)MaxTorque) / 1000);
	output.data[0] = (requestedTorque & 0xFF00) >> 8;
	output.data[1] = (requestedTorque & 0x00FF);
	output.data[2] = output.data[0];
	output.data[3] = output.data[1];
	output.data[4] = 0x75; //msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 20Nm
	output.data[5] = 0xF8; //lsb
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);
	can->LoadBuffer(TXB1, output);
	can->SendBuffer(TXB1);
}
	
//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DMOC::sendCmd3() {
	Frame output;
	static byte alive = 0;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x234;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.srr = 0;
	output.data[0] = 0xC3; //msb of regen watt limit
	output.data[1] = 0x50; //lsb
	output.data[2] = 0xC3; //msb of acceleration limit
	output.data[3] = 0x50; //lsb
	output.data[4] = 0; //not used
	output.data[5] = 60; //20 degrees celsius 
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);
	can->LoadBuffer(TXB2, output);
	can->SendBuffer(TXB2);
}		
			
byte DMOC::calcChecksum(Frame thisFrame) {
	byte cs;
	byte i;
	cs = thisFrame.id;
	for (i = 0; i < 7; i++) cs += thisFrame.data[i];
	i = cs + 3;
	cs = ((int)256 - i);
	return cs;
}