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

/*
 * Each device must initialize its own PrefHandler with its ID.
 * The device is looked up in the device table in the EEPROM and added if necessary.
 */
PrefHandler::PrefHandler(DeviceId id_in)
{
    uint16_t id;

    deviceId = id_in;
    enabled = false;
    lkg_address = EE_MAIN_OFFSET;

    initDeviceTable();

    position = findDevice(deviceId);
    if (position > -1) {
        memCache.Read(EE_DEVICE_TABLE + (2 * position), &id);
        if (id & 0x8000) {
            enabled = true;
        }
        base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * position);
        logger.debug("Device ID %#x was found in device table at entry %i", (int) deviceId, position);
        return;
    }

    //if we got here then there was no entry for this device in the table yet.
    //try to find an empty spot and place it there.
    position = findDevice(NEW);
    if (position > -1) {
        base_address = EE_DEVICES_BASE + (EE_DEVICE_SIZE * position);
        lkg_address = EE_MAIN_OFFSET;
        memCache.Write(EE_DEVICE_TABLE + (2 * position), (uint16_t)deviceId);
        logger.debug("Device ID: %#x was placed into device table at entry: %i", (int) deviceId, position);
        return;
    }

    //we found no matches and could not allocate a space. This is bad. Error out here
    base_address = 0xF0F0;
    logger.error("PrefManager - Device Table Full (Device ID: %#x) !!!", deviceId);
}

PrefHandler::~PrefHandler()
{
}

/*
 * Initialize the device table area in the eeprom if GEVCU marker is missing
 */
void PrefHandler::initDeviceTable()
{
    uint16_t id;

    memCache.Read(EE_DEVICE_TABLE, &id);
    if (id == EE_GEVCU_MARKER) { // the device table was properly initialized
        return;
    }

    logger.debug("Initializing EEPROM device table");

    //initialize table with zeros
    id = 0;
    for (int x = 1; x <= EE_NUM_DEVICES; x++) {
        memCache.Write(EE_DEVICE_TABLE + (2 * x), id);
    }

    //write out magic entry
    id = EE_GEVCU_MARKER;
    memCache.Write(EE_DEVICE_TABLE, id);
}

/*
 * Is the device enabled in the configuration
 */
bool PrefHandler::isEnabled()
{
	return enabled;
}

/*
 * Enable / disable a device
 */
bool PrefHandler::setEnabled(bool en)
{
	uint16_t id = deviceId;

	enabled = en;

    if (enabled) {
        id |= 0x8000; //set enabled bit
    } else {
        id &= 0x7FFF; //clear enabled bit
    }

	return memCache.Write(EE_DEVICE_TABLE + (2 * position), id);
}

/*
 * Search the device table for a device with a given ID
 */
int8_t PrefHandler::findDevice(DeviceId deviceId)
{
    uint16_t id;

    for (int pos = 1; pos <= EE_NUM_DEVICES; pos++) {
        memCache.Read(EE_DEVICE_TABLE + (2 * pos), &id);

        if ((id & 0x7FFF) == deviceId) {
            return pos;
        }
    }
    return -1;
}

/*
 * Enable/Disable the LKG (last known good) configuration
 */
void PrefHandler::LKG_mode(bool mode)
{
    if (mode) {
        lkg_address = EE_LKG_OFFSET;
    } else {
        lkg_address = EE_MAIN_OFFSET;
    }
}

/*
 * Write one byte to an address relative to the device's base
 */
bool PrefHandler::write(uint16_t address, uint8_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Write((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Write two bytes to n address relative to the device's base
 */
bool PrefHandler::write(uint16_t address, uint16_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Write((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Write four bytes to an address relative to the device's base
 */
bool PrefHandler::write(uint16_t address, uint32_t val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Write((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Read one byte from an address relative to the device's base
 */
bool PrefHandler::read(uint16_t address, uint8_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Read((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Read two bytes from an address relative to the device's base
 */
bool PrefHandler::read(uint16_t address, uint16_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Read((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Read four bytes from an address relative to the device's base
 */
bool PrefHandler::read(uint16_t address, uint32_t *val)
{
    if (address >= EE_DEVICE_SIZE) {
        return false;
    }
    return memCache.Read((uint32_t) address + base_address + lkg_address, val);
}

/*
 * Calculate the checksum for a device configuration (block of EE_DEVICE_SIZE bytes)
 */
uint8_t PrefHandler::calcChecksum()
{
    uint16_t counter;
    uint8_t accum = 0;
    uint8_t temp;

    for (counter = 1; counter < EE_DEVICE_SIZE; counter++) {
        memCache.Read((uint32_t) counter + base_address + lkg_address, &temp);
        accum += temp;
    }

    return accum;
}

/*
 * Calculate the current checksum and save it to the proper place within the device config (EE_CHECKSUM)
 */
void PrefHandler::saveChecksum()
{
    uint8_t csum;
    csum = calcChecksum();
    memCache.Write(EE_CHECKSUM + base_address + lkg_address, csum);
}

/*
 * Get checksum from EEPROM and calculate the current checksum to see if they match
 */
bool PrefHandler::checksumValid()
{
    uint8_t stored_chk, calc_chk;

    memCache.Read(EE_CHECKSUM + base_address + lkg_address, &stored_chk);
    calc_chk = calcChecksum();
    if (stored_chk == calc_chk) {
        logger.debug("%#x valid checksum, using stored config values", deviceId);
        return true;
    } else {
        logger.warn("#x invalid checksum, using hard coded config values (stored: %#x, calc: %#x", deviceId, stored_chk, calc_chk);
        return false;
    }
}

/*
 * Write first dirty page of the cache to the eeprom
 */
void PrefHandler::suggestCacheWrite()
{
    // we don't call FlushAllPages because this would kill the timing (delay(10))
    memCache.FlushSinglePage();
}
