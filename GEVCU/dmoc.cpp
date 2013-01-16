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
		break;
	case TORQUE:
		step = WATTS;
		break;
	case WATTS:
		step = RPM;
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
	output.data[0] = 0x4E; //msb  RPMS. RPM is offset by -20000 so zero rpm is value of 20000
	output.data[1] = 0x20; //lsb RPM
	output.data[2] = 0; //not used
	output.data[3] = 0; //not used
	output.data[4] = 0; //not used
	output.data[5] = ON;
	output.data[6] = (ENABLE<<4) + DRIVE + alive;
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
	can->LoadBuffer(TXB0, output);
	can->SendBuffer(TXB0);
}
	
//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DMOC::sendCmd3() {
	Frame output;
	static byte alive = 0;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x234;
	output.ide = 0; //standard frame
	output.data[0] = 0xC3; //msb of regen watt limit
	output.data[1] = 0x50; //lsb
	output.data[2] = 0xC3; //msb of acceleration limit
	output.data[3] = 0x50; //lsb
	output.data[4] = 0; //not used
	output.data[5] = 60; //20 degrees celsius 
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);
	can->LoadBuffer(TXB0, output);
	can->SendBuffer(TXB0);
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