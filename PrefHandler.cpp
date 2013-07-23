/*
 * pref_handler.cpp
 *
 * Abstracts away the particulars of how preferences are stored.
 * Transparently supports main and "last known good" storage and retrieval
 *
 * Created: 03/24/2013
 *  Author: Collin Kidder
 */ 

#include "PrefHandler.h"

PrefHandler::PrefHandler() {
  lkg_address = EE_MAIN_OFFSET; //default to normal mode
  base_address = 0;
}

PrefHandler::PrefHandler(uint32_t base) {
  base_address = base;
  lkg_address = EE_MAIN_OFFSET;
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
  Logger::debug("Stored Checksum: %i Calc: %i", stored_chk, calc_chk);
  
  return (stored_chk == calc_chk);
}
