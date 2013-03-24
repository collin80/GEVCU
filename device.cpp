/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */ 

#include "device.h"
#include "mem_cache.h"

//Empty functions to handle these two callbacks if the derived classes don't
void DEVICE::handleFrame(RX_CAN_FRAME& frame) {
	
}

void DEVICE::handleTick() {
	
}

void DEVICE::setupDevice() {
}

DEVICE::DEVTYPE DEVICE::getDeviceType() {
  return DEVICE_NONE;
}

DEVICE::DEVID DEVICE::getDeviceID() {
  return INVALID;
}

DEVICE::DEVICE() {
  pref_base_addr = 0;
}


DEVICE::DEVICE(CANRaw* canlib) {
	can = canlib;
}

void DEVICE::prefWrite(uint16_t address, uint8_t val) {
  MemCache.Write((uint32_t)address + pref_base_addr, val);
}

void DEVICE::prefWrite(uint16_t address, uint16_t val) {
  MemCache.Write((uint32_t)address + pref_base_addr, val);
}

void DEVICE::prefWrite(uint16_t address, uint32_t val) {
  MemCache.Write((uint32_t)address + pref_base_addr, val);
}

void DEVICE::prefRead(uint16_t address, uint8_t *val) {
  MemCache.Read((uint32_t)address + pref_base_addr, val);
}

void DEVICE::prefRead(uint16_t address, uint16_t *val) {
  MemCache.Read((uint32_t)address + pref_base_addr, val);  
}

void DEVICE::prefRead(uint16_t address, uint32_t *val) {
  MemCache.Read((uint32_t)address + pref_base_addr, val);
}

uint8_t DEVICE::prefCalcChecksum() {
  uint16_t counter;
  uint8_t accum = 0;
  uint8_t temp;
  for (counter = 1; counter < EE_DEVICE_SIZE; counter++) {
    MemCache.Read((uint32_t)counter + pref_base_addr, &temp);
    accum += temp;
  }
  return accum;
} 

//calculate the current checksum and save it to the proper place
void DEVICE::prefSaveChecksum() {
  uint8_t csum;
  csum = prefCalcChecksum();
  MemCache.Write(EE_CHECKSUM + pref_base_addr, csum);
}

bool DEVICE::prefChecksumValid() {
  //get checksum from EEPROM and calculate the current checksum to see if they match
  uint8_t stored_chk, calc_chk;
  
  MemCache.Read(EE_CHECKSUM + pref_base_addr, &stored_chk);
  calc_chk = prefCalcChecksum();
  
  return (stored_chk == calc_chk);
}
