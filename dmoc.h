/*
 * dmoc.h
 *
 * Note that the dmoc needs to have some form of input for gear selector (drive/neutral/reverse)
 *
 *
 * Created: 1/13/2013 9:18:36 PM
 *  Author: Collin Kidder
 */


#ifndef DMOC_H_
#define DMOC_H_

#include "MCP2515.h"
#include "motorctrl.h"

class DMOC : 
public MOTORCTRL {
public:
  enum GEARS {
    NEUTRAL = 0,
    DRIVE = 1,
    REVERSE = 2,
    ERROR = 3
  };

  enum STEP {
    SPEED_TORQUE,
    CHAL_RESP
  };

  enum KEYSTATE {
    OFF = 0,
    ON = 1,
    RESERVED = 2,
    NOACTION = 3
  };

  enum OPSTATE {
    DISABLED = 0,
    STANDBY = 1,
    ENABLE = 2,
    POWERDOWN = 3
  };

private:
  uint16_t requestedTorque;
  uint16_t requestedRPM;
  uint16_t actualTorque;
  uint16_t actualRPM;
  uint16_t MaxTorque;	//maximum torque in 0.1 Nm
  uint16_t MaxRPM;
  GEARS selectedGear;
  OPSTATE opstate;
  int step;
  byte alive;

  void sendCmd1();
  void sendCmd2();
  void sendCmd3();
  void sendCmd4();
  void sendCmd5();
  byte calcChecksum(Frame thisFrame);

public:
  void handleFrame(Frame& frame);
  void handleTick();	
  void setOpState(OPSTATE op);
  void setGear(GEARS gear);
  DMOC(MCP2515 *canlib);
  DEVICE::DEVID getDeviceID();
};

#endif /* DMOC_H_ */

