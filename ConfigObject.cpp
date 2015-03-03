/*
 * ConfigObject.cpp
 *
 * Abstracts away pretty much everything about how configuration is stored
 *
Copyright (c) 2015 Collin Kidder (Original idea: Jack Rickard)

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

#include "ConfigObject.h"
#include "globals.h"

int dummy;
extern MemCache *memCache;

//the big abstraction. Allows device to either be a device type such as DEVICE_MOTORCTRL or an actual device ID such as CODAUQM.
//Then, location is the data item you'd like to get or set such as EEMC_MAX_RPM. 
int& ConfigObject::operator()(int device, int location)
{
	int deviceId = device;
	int eepromOffset = 0;
	int val = 0;
	Device *devPtr;

	//is this a device type or an actual ID? Device types are under 0x1000 while id's are 0x1000 or higher
	if (device < 0x1000)
	{
		devPtr = deviceManager->getDeviceByType((DeviceType)device);
		if (devPtr == 0) return dummy;
		deviceId = devPtr->getId();
	}

	//Now, find the proper EEPROM location for this deviceID
	eepromOffset = getEELocation(deviceId);
	if (eepromOffset < 0) return dummy;

	if (location >= EE_DEVICE_SIZE) return dummy;

	memCache->Read((uint32_t)location + eepromOffset, (uint32_t *)&val);
	return val;
}

int ConfigObject::getEELocation(int deviceid)
{
	uint16_t id;

	for (int x = 1; x < 64; x++) {
		memCache->Read(EE_DEVICE_TABLE + (2 * x), &id);
		if ((id & 0x7FFF) == deviceid) {
			return EE_DEVICES_BASE + (EE_DEVICE_SIZE * x);			
		}
	}
	return -1;
}