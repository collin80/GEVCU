/*
 * modulemanager.h
 *
 * Keeps track of which modules are installed 
 * Allows for tracking a certain # of devices
 * which can be of a variety of different types
 * but all of which are derived from the DEVICE class
 *
 *
 * Created: 1/20/2013 10:22:22 PM
 *  Author: Collin
 */ 

//Need a way to pass messages from one device to others. The most likely thing would be to target a certain type of
//device. For instance, the throttle might need to send a message to whichever/any motor controller(s) are installed.
//This is also complicated by the fact that we don't know what sort of messages might need to be passed.
//we'll have to set up a list of types -> 8 bit ints, 16, 32bit. So messages have a target(8 bit), a message type (16 bit)
//and a message (8-32 bit integer)

//each device must have a reference back to the module manager.
//module manager passes messages between devices.

#ifndef MODULEMANAGER_H_
#define MODULEMANAGER_H_

#define MAX_DEVICES	8
#include "device.h"

class MODULEMANAGER {
	private:
		DEVICE* devices[MAX_DEVICES + 1]; //null terminated so need the extra 1
	
	public:
		void add(DEVICE * dev);
		void remove(DEVICE* dev);
		DEVICE* get(int index);
		MODULEMANAGER();
	
};

#endif /* MODULEMANAGER_H_ */