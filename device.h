/*
 * device.h
 *
 * Created: 1/20/2013 10:14:51 PM
 *  Author: Collin Kidder
 */ 


#ifndef DEVICE_H_
#define DEVICE_H_

#include "MCP2515.h"
#include "eeprom_layout.h"
#include <EEPROM.h>

class DEVICE {
  public:
    enum DEVTYPE{
      DEVICE_ANY,
      DEVICE_MOTORCTRL,
      DEVICE_BMS,
      DEVICE_CHARGER,
      DEVICE_DISPLAY,
      DEVICE_THROTTLE,
      DEVICE_MISC,
      DEVICE_NONE
    };
    enum DEVID{ //unique device ID for every piece of hardware possible			
      DMOC645 = 0x1000,
      BRUSACHARGE = 0x1010,
      TCCHCHARGE = 0x1020,
      POTACCELPEDAL = 0x1030,
      INVALID = 0xFFFF
    };
	
    protected:
    MCP2515 * can;
    uint16_t pref_base_addr;
	
    public:
    DEVICE();        
    virtual void handleFrame(Frame& frame);
    virtual void handleTick();
    virtual void setupDevice();
    virtual DEVTYPE getDeviceType();
    virtual DEVID getDeviceID();
    DEVICE(MCP2515 *canlib);
    void prefWrite(uint16_t address, uint8_t val);
    void prefWrite(uint16_t address, uint16_t val);
    void prefWrite(uint16_t address, uint32_t val);
    void prefRead(uint16_t address, uint8_t &val);
    void prefRead(uint16_t address, uint16_t &val);
    void prefRead(uint16_t address, uint32_t &val);
    uint8_t prefCalcChecksum();
    void prefSaveChecksum();
    bool prefChecksumValid();
};

#endif /* DEVICE_H_ */
