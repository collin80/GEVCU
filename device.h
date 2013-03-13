/*
 * device.h
 *
 * Created: 1/20/2013 10:14:51 PM
 *  Author: Collin Kidder
 */ 


#ifndef DEVICE_H_
#define DEVICE_H_

#include "eeprom_layout.h"
#ifdef __SAM3X8E__
  #include <due_can.h>
#else
  #include "MCP2515.h"
  #include <EEPROM.h>
#endif

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
#ifdef __SAM3X8E__
    CANRaw* can;
#else
    MCP2515 * can;
#endif
    uint16_t pref_base_addr;
	
    public:
    DEVICE();
#ifdef __SAM3X8E__
  virtual void handleFrame(RX_CAN_FRAME& frame);
#else
  virtual void handleFrame(Frame& frame);
#endif
    virtual void handleTick();
    virtual void setupDevice();
    virtual DEVTYPE getDeviceType();
    virtual DEVID getDeviceID();
#ifdef __SAM3X8E__
    DEVICE(CANRaw *canlib);
#else
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
#endif
};

#endif /* DEVICE_H_ */
