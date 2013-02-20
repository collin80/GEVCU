/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */ 

#include "device.h"

//Empty functions to handle these two callbacks if the derived classes don't

void DEVICE::handleFrame(Frame& frame) {
	
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

DEVICE::DEVICE(MCP2515 *canlib) {
	can = canlib;
}

void DEVICE::prefWrite(uint16_t address, uint8_t val) {
  EEPROM.write(address + pref_base_addr, val);
}

void DEVICE::prefWrite(uint16_t address, uint16_t val) {
  EEPROM.write(address + pref_base_addr, val >> 8);
  EEPROM.write(address + 1 + pref_base_addr, val & 0xFF);
}

void DEVICE::prefWrite(uint16_t address, uint32_t val) {
  EEPROM.write(address + pref_base_addr, val >> 24);
  EEPROM.write(address + 1 + pref_base_addr, (val >> 16) & 0xFF);
  EEPROM.write(address + 2 + pref_base_addr, (val >> 8) & 0xFF);
  EEPROM.write(address + 3 + pref_base_addr, val & 0xFF);
}

void DEVICE::prefRead(uint16_t address, uint8_t &val) {
  val = EEPROM.read(address + pref_base_addr);
}

void DEVICE::prefRead(uint16_t address, uint16_t &val) {
  val = EEPROM.read(address + pref_base_addr) << 8;
  val += EEPROM.read(address + 1 + pref_base_addr);
}

void DEVICE::prefRead(uint16_t address, uint32_t &val) {
  val = (uint32_t)EEPROM.read(address + pref_base_addr) << 24;
  val += (uint32_t)EEPROM.read(address + 1 + pref_base_addr) << 16;
  val += (uint32_t)EEPROM.read(address + 2 + pref_base_addr) << 8;
  val += (uint32_t)EEPROM.read(address + 3 + pref_base_addr);  
}

uint8_t DEVICE::prefCalcChecksum() {
  uint16_t counter;
  uint8_t accum = 0;
  for (counter = 0; counter < EE_DEVICE_SIZE; counter++) {
    accum += EEPROM.read(counter + pref_base_addr);
  }
  return accum;
} 

//calculate the current checksum and save it to the proper place
void DEVICE::prefSaveChecksum() {
  uint8_t csum;
  csum = prefCalcChecksum();
  EEPROM.write(EE_CHECKSUM + pref_base_addr, csum);
}

bool DEVICE::prefChecksumValid() {
  //get checksum from EEPROM and calculate the current checksum to see if they match
  uint8_t stored_chk, calc_chk;
  
  stored_chk = EEPROM.read(EE_CHECKSUM + pref_base_addr);
  calc_chk = prefCalcChecksum();
  
  return (stored_chk == calc_chk);
}
