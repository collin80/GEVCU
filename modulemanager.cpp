/*
 * modulemanager.cpp
 *
 * Created: 1/20/2013 10:22:07 PM
 *  Author: Collin
 */ 

#include "device.h"
#include "modulemanager.h"

MODULEMANAGER::MODULEMANAGER() {
	int i;
	for (i = 0; i < MAX_DEVICES; i++) devices[i] = NULL;
}

void MODULEMANAGER::add(DEVICE * dev) {
	int i;
	for (i = 0; i < MAX_DEVICES; i++) {
		if (devices[i] == NULL) {
			devices[i] = dev;
		}			
	}
}

void MODULEMANAGER::remove(DEVICE* dev) {
	
}

DEVICE* MODULEMANAGER::get(int index) {
	if (index >= 0 && index < MAX_DEVICES)
		return devices[index];
	
	return NULL;
}