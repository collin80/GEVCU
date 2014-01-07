/*
 * Device.h
 *
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

#ifndef DEVICE_H_
#define DEVICE_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceTypes.h"
#include "eeprom_layout.h"
#include "PrefHandler.h"
#include "Sys_Messages.h"

/*
 * A abstract class to hold device configuration. It is to be accessed
 * by sub-classes via getConfiguration() and then cast into its
 * correct sub-class.
 */
class DeviceConfiguration {

};

/*
 * A abstract class for all Devices.
 */
class Device: public TickObserver {
public:
	Device();
	virtual void setup();
	virtual void handleMessage(uint32_t, void* );
	virtual DeviceType getType();
	virtual DeviceId getId();
	void handleTick();
	bool isEnabled();
	virtual uint32_t getTickInterval();

	virtual void loadConfiguration();
	virtual void saveConfiguration();
	DeviceConfiguration *getConfiguration();
	void setConfiguration(DeviceConfiguration *);

protected:
	PrefHandler *prefsHandler;

private:
	DeviceConfiguration *deviceConfiguration; // reference to the currently active configuration
};

#endif /* DEVICE_H_ */
