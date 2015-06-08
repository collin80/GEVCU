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

PrefHandler::PrefHandler()
{
    deviceId = INVALID;
    lkg_address = EE_MAIN_OFFSET; //default to normal mode
    base_address = 0;
}

void PrefHandler::initDevTable()
{
    uint16_t id;

    memCache->Read(EE_DEVICE_TABLE, &id);

    if (id == 0xDEAD) {
        return;
    }

    Logger::debug(deviceId, "Initializing EEPROM device table");

    //initialize table with zeros
    id = 0;

    for (int x = 1; x < 64; x++) {
        memCache->Write(EE_DEVICE_TABLE + (2 * x), id);
    }

    //write out magic entry
    id = 0xDEAD;
    memCache->Write(EE_DEVICE_TABLE, id);
}

bool PrefHandler::isEnabled()
{
	return enabled;
}

bool PrefHandler::setEnabled(bool en)
{
	uint16_t id = deviceId;

	enabled = en;

	if (enabled) {
		id |= 0x8000; //set enabled bit
	}
	else {
		id &= 0x7FFF; //clear enabled bit
	}

	return memCache->Write(EE_DEVICE_TABLE + (2 * position), id);
}

/*
 * Search the device table for a device with a given ID
 */
int8_t PrefHandler::findDevice(DeviceId deviceId)
{
    uint16_t id;

    for (int pos = 1; pos < 64; pos++) {
        memCache->Read(EE_DEVICE_TABLE + (2 * pos), &id);

        if ((id & 0x7FFF) == deviceId) {
            return pos;
        }
    }
    return -1;
}

//Given a device ID we must search the 64 entry table found in EEPROM to see if the device
//has a spot in EEPROM. If it does not then
PrefHandler::PrefHandler(DeviceId id_in)
{
    uint16_t id;

    deviceId = id_in;
	enabled = false; 

    initDevTable();

    position = findDevice(deviceId);
    if (position > -1) {
        memCache->Read(EE_DEVICE_TABLE + (2 * position), &id);
        if (id & 0x8000) {
            enabled = true;
        }
        base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * position);
        lkg_address = EE_MAIN_OFFSET;
        Logger::debug(deviceId, "Device ID %X was found in device table at entry %i", (int) deviceId, position);
        return;
    }

    //if we got here then there was no entry for this device in the table yet.
    //try to find an empty spot and place it there.
    position = findDevice(NEW);
    if (position > -1) {
        base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * position);
        lkg_address = EE_MAIN_OFFSET;
        memCache->Write(EE_DEVICE_TABLE + (2 * position), (uint16_t)deviceId);
        Logger::debug(deviceId, "Device ID: %X was placed into device table at entry: %i", (int) deviceId, position);
        return;
    }

    //we found no matches and could not allocate a space. This is bad. Error out here
    base_address = 0xF0F0;
    lkg_address = EE_MAIN_OFFSET;
    Logger::error(deviceId, "PrefManager - Device Table Full!!!");
}

PrefHandler::~PrefHandler()
{
}

void PrefHandler::LKG_mode(bool mode)
{
    if (mode) {
        lkg_address = EE_LKG_OFFSET;
    } else {
        lkg_address = EE_MAIN_OFFSET;
    }
}

bool PrefHandler::write(uint16_t address, uint8_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Write((uint32_t) address + base_address + lkg_address, val);
}

bool PrefHandler::write(uint16_t address, uint16_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Write((uint32_t) address + base_address + lkg_address, val);
}

bool PrefHandler::write(uint16_t address, uint32_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Write((uint32_t) address + base_address + lkg_address, val);
}

bool PrefHandler::read(uint16_t address, uint8_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Read((uint32_t) address + base_address + lkg_address, val);
}

bool PrefHandler::read(uint16_t address, uint16_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Read((uint32_t) address + base_address + lkg_address, val);
}

bool PrefHandler::read(uint16_t address, uint32_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache->Read((uint32_t) address + base_address + lkg_address, val);
}

uint8_t PrefHandler::calcChecksum()
{
    uint16_t counter;
    uint8_t accum = 0;
    uint8_t temp;

    for (counter = 1; counter < EE_DEVICE_SIZE; counter++) {
        memCache->Read((uint32_t) counter + base_address + lkg_address, &temp);
        accum += temp;
    }

    return accum;
}

//calculate the current checksum and save it to the proper place
void PrefHandler::saveChecksum()
{
    uint8_t csum;
    csum = calcChecksum();
    memCache->Write(EE_CHECKSUM + base_address + lkg_address, csum);
}

bool PrefHandler::checksumValid()
{
    //get checksum from EEPROM and calculate the current checksum to see if they match
    uint8_t stored_chk, calc_chk;

    memCache->Read(EE_CHECKSUM + base_address + lkg_address, &stored_chk);
    calc_chk = calcChecksum();
    Logger::info(deviceId, "Stored Checksum: %X Calc: %X", stored_chk, calc_chk);

    return (stored_chk == calc_chk);
}

void PrefHandler::forceCacheWrite()
{
    memCache->FlushAllPages();
}

//initializes all the system EEPROM values. Chances are this should be broken out a bit but
//there is only one checksum check for all of them so it's simple to do it all here.
void PrefHandler::initSysEEPROM()
{
    Logger::info("Initializing EEPROM");

    //three temporary storage places to make saving to EEPROM easy
    uint8_t eight;
    uint16_t sixteen;
    uint32_t thirtytwo;

    Logger::info("Initializing EEPROM");

    eight = SYSTEM_DUED;
    write(EESYS_SYSTEM_TYPE, eight);

    sixteen = 1024; //no gain
    write(EESYS_ADC0_GAIN, sixteen);
    write(EESYS_ADC1_GAIN, sixteen);
    write(EESYS_ADC2_GAIN, sixteen);
    write(EESYS_ADC3_GAIN, sixteen);

    sixteen = 0; //no offset
    write(EESYS_ADC0_OFFSET, sixteen);
    write(EESYS_ADC1_OFFSET, sixteen);
    write(EESYS_ADC2_OFFSET, sixteen);
    write(EESYS_ADC3_OFFSET, sixteen);

    sixteen = 500; //multiplied by 1000 so 500k baud
    write(EESYS_CAN0_BAUD, sixteen);
    write(EESYS_CAN1_BAUD, sixteen);

    sixteen = 11520; //multiplied by 10
    write(EESYS_SERUSB_BAUD, sixteen);

    sixteen = 100; //multiplied by 1000
    write(EESYS_TWI_BAUD, sixteen);

    sixteen = 100; //number of ticks per second
    write(EESYS_TICK_RATE, sixteen);

    thirtytwo = 0;
    write(EESYS_RTC_TIME, thirtytwo);
    write(EESYS_RTC_DATE, thirtytwo);

    thirtytwo = 0; //ok, not technically 32 bytes but the four zeros still shows it is unused.
    write(EESYS_WIFI0_SSID, thirtytwo);
    write(EESYS_WIFI1_SSID, thirtytwo);
    write(EESYS_WIFI2_SSID, thirtytwo);
    write(EESYS_WIFIX_SSID, thirtytwo);

    eight = 0; //no channel, DHCP off, B mode
    write(EESYS_WIFI0_CHAN, eight);
    write(EESYS_WIFI0_DHCP, eight);
    write(EESYS_WIFI0_MODE, eight);

    write(EESYS_WIFI1_CHAN, eight);
    write(EESYS_WIFI1_DHCP, eight);
    write(EESYS_WIFI1_MODE, eight);

    write(EESYS_WIFI2_CHAN, eight);
    write(EESYS_WIFI2_DHCP, eight);
    write(EESYS_WIFI2_MODE, eight);

    write(EESYS_WIFIX_CHAN, eight);
    write(EESYS_WIFIX_DHCP, eight);
    write(EESYS_WIFIX_MODE, eight);

    thirtytwo = 0;
    write(EESYS_WIFI0_IPADDR, thirtytwo);
    write(EESYS_WIFI1_IPADDR, thirtytwo);
    write(EESYS_WIFI2_IPADDR, thirtytwo);
    write(EESYS_WIFIX_IPADDR, thirtytwo);

    write(EESYS_WIFI0_KEY, thirtytwo);
    write(EESYS_WIFI1_KEY, thirtytwo);
    write(EESYS_WIFI2_KEY, thirtytwo);
    write(EESYS_WIFIX_KEY, thirtytwo);

    eight = 1;
    write(EESYS_LOG_LEVEL, eight);

    saveChecksum();
}

