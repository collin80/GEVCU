/*
 * PrefHandler.h
 *
 * header for preference handler
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

#ifndef PREF_HANDLER_H_
#define PREF_HANDLER_H_

#include <Arduino.h>
#include "config.h"
#include "eeprom_layout.h"
#include "MemCache.h"
#include "DeviceTypes.h"
#include "Logger.h"

//normal or Last Known Good configuration
#define PREF_MODE_NORMAL  false
#define PREF_MODE_LKG     true

extern MemCache *memCache;

class PrefHandler {
public:

	PrefHandler();
	PrefHandler(DeviceId id);
        ~PrefHandler();
	void LKG_mode(bool mode);
	void write(uint16_t address, uint8_t val);
	void write(uint16_t address, uint16_t val);
	void write(uint16_t address, uint32_t val);
	void read(uint16_t address, uint8_t *val);
	void read(uint16_t address, uint16_t *val);
	void read(uint16_t address, uint32_t *val);
	uint8_t calcChecksum();
	void saveChecksum();
	bool checksumValid();
    void forceCacheWrite();
	bool isEnabled();
	void setEnabledStatus(bool en);
	static bool setDeviceStatus(uint16_t device, bool enabled);

private:
	uint32_t base_address; //base address for the parent device
	uint32_t lkg_address;
	bool use_lkg; //use last known good config?
	bool enabled;
	int position; //position within the device table
	void initDevTable();
};

#endif

