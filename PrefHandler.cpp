/*
 * PrefHandler.cpp
 *
 * Abstracts away the particulars of how preferences are stored.
 * Transparently supports main and "last known good" storage and retrieval
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

#include "PrefHandler.h"

PrefHandler::PrefHandler() {
  lkg_address = EE_MAIN_OFFSET; //default to normal mode
  base_address = 0;
}

bool PrefHandler::isEnabled() 
{
	return enabled;
}

void PrefHandler::setEnabledStatus(bool en) 
{
	uint16_t id;

	enabled = en;

	if (enabled) {
		id |= 0x8000; //set enabled bit
	}
	else {
		id &= 0x7FFF; //clear enabled bit
	}

	memCache->Write(EE_DEVICE_TABLE + (2 * position), id);
}


void PrefHandler::initDevTable() 
{
  uint16_t id;

  memCache->Read(EE_DEVICE_TABLE, &id);
  if (id == 0xDEAD) return;

  Logger::debug("Initializing EEPROM device table");

  //initialize table with zeros
  id = 0;
  for (int x = 1; x < 64; x++) {
	  memCache->Write(EE_DEVICE_TABLE + (2 * x), id);
  }

  //write out magic entry
  id = 0xDEAD;
  memCache->Write(EE_DEVICE_TABLE, id);
}

//Given a device ID we must search the 64 entry table found in EEPROM to see if the device
//has a spot in EEPROM. If it does not then 
PrefHandler::PrefHandler(DeviceId id_in) {
	uint16_t id;

	enabled = false; 

	initDevTable();

	for (int x = 1; x < 64; x++) {
		memCache->Read(EE_DEVICE_TABLE + (2 * x), &id);
		if ((id & 0x7FFF) == ((int)id_in)) {
			base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * x);
			lkg_address = EE_MAIN_OFFSET;
			if (id & 0x8000) enabled = true;
			position = x;
			Logger::info("Device ID: %X was found in device table at entry: %i", (int)id_in, x);
			return;
		}
	}

	//if we got here then there was no entry for this device in the table yet.
	//try to find an empty spot and place it there.
	for (int x = 1; x < 64; x++) {
		memCache->Read(EE_DEVICE_TABLE + (2 * x), &id);
		if (id == 0) {
			base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * x);
			lkg_address = EE_MAIN_OFFSET;
			enabled = false; //default to devices being off until the user says otherwise
			id = (int)id_in;
			memCache->Write(EE_DEVICE_TABLE + (2*x), id);
			position = x;
			Logger::info("Device ID: %X was placed into device table at entry: %i", (int)id, x);
			return;
		}
	}

	//we found no matches and could not allocate a space. This is bad. Error out here
	base_address = 0xF0F0;
	lkg_address = EE_MAIN_OFFSET;
	Logger::error("PrefManager - Device Table Full!!!");
}

//A special static function that can be called whenever, wherever to turn a specific device on/off. Does not
//attempt to do so at runtime so the user will still have to power cycle to change the device status.
//returns true if it could make the change, false if it could not.
bool PrefHandler::setDeviceStatus(uint16_t device, bool enabled) 
{
	uint16_t id;
	for (int x = 1; x < 64; x++) {
		memCache->Read(EE_DEVICE_TABLE + (2 * x), &id);
		if ((id & 0x7FFF) == (device & 0x7FFF)) {
			Logger::debug("Found a device record to edit");
			if (enabled) {
				id |= 0x8000;
			}
			else {
				id &= 0x7FFF;
			}
			Logger::debug("ID to write: %X", id);
			memCache->Write(EE_DEVICE_TABLE + (2 * x), id);
			return true;
		}
	}
	return false;
}

PrefHandler::~PrefHandler() {
}

void PrefHandler::LKG_mode(bool mode) {
  if (mode) lkg_address = EE_LKG_OFFSET;
  else lkg_address = EE_MAIN_OFFSET;
}

void PrefHandler::write(uint16_t address, uint8_t val) {
  memCache->Write((uint32_t)address + base_address + lkg_address, val);
}

void PrefHandler::write(uint16_t address, uint16_t val) {
  memCache->Write((uint32_t)address + base_address + lkg_address, val);
}

void PrefHandler::write(uint16_t address, uint32_t val) {
  memCache->Write((uint32_t)address + base_address + lkg_address, val);
}

void PrefHandler::read(uint16_t address, uint8_t *val) {
  memCache->Read((uint32_t)address + base_address + lkg_address, val);
}

void PrefHandler::read(uint16_t address, uint16_t *val) {
  memCache->Read((uint32_t)address + base_address + lkg_address, val);
}

void PrefHandler::read(uint16_t address, uint32_t *val) {
  memCache->Read((uint32_t)address + base_address + lkg_address, val);
}

uint8_t PrefHandler::calcChecksum() {
  uint16_t counter;
  uint8_t accum = 0;
  uint8_t temp;
  for (counter = 1; counter < EE_DEVICE_SIZE; counter++) {
    memCache->Read((uint32_t)counter + base_address + lkg_address, &temp);
    accum += temp;
  }
  return accum;
} 

//calculate the current checksum and save it to the proper place
void PrefHandler::saveChecksum() {
  uint8_t csum;
  csum = calcChecksum();
  memCache->Write(EE_CHECKSUM + base_address + lkg_address, csum);
}

bool PrefHandler::checksumValid() {
  //get checksum from EEPROM and calculate the current checksum to see if they match
  uint8_t stored_chk, calc_chk;
  
  memCache->Read(EE_CHECKSUM + base_address + lkg_address, &stored_chk);
  calc_chk = calcChecksum();
  Logger::info("Stored Checksum: %X Calc: %X", stored_chk, calc_chk);
  
  return (stored_chk == calc_chk);
}

void PrefHandler::forceCacheWrite()
{
  memCache->FlushAllPages();
}

