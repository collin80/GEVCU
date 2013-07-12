/*
 * pref_handler.cpp
 *
 * Abstracts away the particulars of how preferences are stored.
 * Transparently supports main and "last known good" storage and retrieval
 *
 * Created: 03/24/2013
 *  Author: Collin Kidder
 */ 

#include "pref_handler.h"

PREFHANDLER::PREFHANDLER() {
  lkg_address = EE_MAIN_OFFSET; //default to normal mode
  base_address = 0;
}

PREFHANDLER::PREFHANDLER(uint32_t base) {
  base_address = base;
  lkg_address = 0;
}

void PREFHANDLER::LKG_mode(bool mode) {
  if (mode) lkg_address = EE_LKG_OFFSET;
  else lkg_address = EE_MAIN_OFFSET;
}

void PREFHANDLER::write(uint16_t address, uint8_t val) {
  MemCache.Write((uint32_t)address + base_address + lkg_address, val);
}

void PREFHANDLER::write(uint16_t address, uint16_t val) {
  MemCache.Write((uint32_t)address + base_address + lkg_address, val);
}

void PREFHANDLER::write(uint16_t address, uint32_t val) {
  MemCache.Write((uint32_t)address + base_address + lkg_address, val);
}

void PREFHANDLER::read(uint16_t address, uint8_t *val) {
  MemCache.Read((uint32_t)address + base_address + lkg_address, val);
}

void PREFHANDLER::read(uint16_t address, uint16_t *val) {
  MemCache.Read((uint32_t)address + base_address + lkg_address, val);  
}

void PREFHANDLER::read(uint16_t address, uint32_t *val) {
  MemCache.Read((uint32_t)address + base_address + lkg_address, val);
}

uint8_t PREFHANDLER::calcChecksum() {
  uint16_t counter;
  uint8_t accum = 0;
  uint8_t temp;
  for (counter = 1; counter < EE_DEVICE_SIZE; counter++) {
    MemCache.Read((uint32_t)counter + base_address + lkg_address, &temp);
    accum += temp;
  }
  return accum;
} 

//calculate the current checksum and save it to the proper place
void PREFHANDLER::saveChecksum() {
  uint8_t csum;
  csum = calcChecksum();
  MemCache.Write(EE_CHECKSUM + base_address + lkg_address, csum);
}

bool PREFHANDLER::checksumValid() {
  //get checksum from EEPROM and calculate the current checksum to see if they match
  uint8_t stored_chk, calc_chk;
  
  MemCache.Read(EE_CHECKSUM + base_address + lkg_address, &stored_chk);
  calc_chk = calcChecksum();
  
  return (stored_chk == calc_chk);
}


