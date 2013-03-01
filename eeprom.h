/*
 * eeprom.h
 *
 * Creates an Arduino compatible interface to EEPROM by means of an I2C connected 24AA256 chip
 *
 * Created: 1/20/2013 10:14:51 PM
 *  Author: Collin Kidder
 */ 

#ifdef __SAM3X8E__

#ifndef EEPROM_H_
#define EEPROM_H_

#include "wire.h"

class EEPROMWRAPPER {
  public:
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);
  
  //Additional calls not implemented by AVR EEPROM lib
  uint16_t read16(uint16_t address);
  void write16(uint16_t address, uint16_t value);
  
  uint32_t read32(uint16_t address);
  void write32(uint16_t address, uint32_t value);
  
  EEPROMWRAPPER(); //constructor
  
};

extern EEPROMWRAPPER EEPROM;
#endif //EEPROM
#endif //SAM
