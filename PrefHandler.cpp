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

PrefHandler::PrefHandler(uint32_t base) {
  base_address = base;
  lkg_address = EE_MAIN_OFFSET;
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
  Logger::debug("Stored Checksum: %i Calc: %i", stored_chk, calc_chk);
  
  return (stored_chk == calc_chk);
}
