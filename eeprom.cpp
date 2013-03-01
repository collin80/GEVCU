/*
 * device.cpp
 *
 * Created: 1/20/2013 10:14:36 PM
 *  Author: Collin Kidder
 */

#ifdef __SAM3X8E__

#include "eeprom.h"

uint8_t EEPROMWRAPPER::read(uint16_t address) {
  uint8_t buffer[3];
  uint8_t c;
  buffer[0] = ((address & 0xFF00) >> 8);
  buffer[1] = (address & 0x00FF);
  Wire.beginTransmission(0b01010000);  
  Wire.write(buffer, 2);
  Wire.endTransmission();
  delayMicroseconds(100);

  Wire.requestFrom(0b01010000, 2);
  if(Wire.available())    
  { 
    c = Wire.read(); // receive a byte as character

  }
  return c;
}

void EEPROMWRAPPER::write(uint16_t address, uint8_t value) {
  uint8_t buffer[3];
  buffer[0] = ((address & 0xFF00) >> 8);
  buffer[1] = (address & 0x00FF);
  buffer[2] = value;
  Wire.beginTransmission(0b01010000);
  Wire.write(buffer, 3);    
  Wire.endTransmission();
}
/* 
 uint16_t EEPROM::read16(uint16_t address);
 void EEPROM::write16(uint16_t address, uint16_t value);
 
 uint32_t EEPROM::read32(uint16_t address);
 void EEPROM::write32(uint16_t address, uint32_t value);
 */

EEPROMWRAPPER::EEPROMWRAPPER() {
  //Wire.begin();
}

EEPROMWRAPPER EEPROM = EEPROMWRAPPER();

#endif


