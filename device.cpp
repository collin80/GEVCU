/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin
 */ 

#include "device.h"


//Empty functions to handle these two callbacks if the derived classes don't

void DEVICE::handleFrame(Frame& frame) {
	
}

void DEVICE::handleTick() {
	
}

DEVICE::DEVICE(MCP2515 *canlib) {
	can = canlib;
}