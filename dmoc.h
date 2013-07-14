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

#include <Arduino.h>
#include "config.h"
#include "motorctrl.h"
#include "sys_io.h"
#include "TickHandler.h"

class DMOC : public MotorController {
public:
  enum Gears {
    NEUTRAL = 0,
    DRIVE = 1,
    REVERSE = 2,
    ERROR = 3
  };

  enum Step {
    SPEED_TORQUE,
    CHAL_RESP
  };
  
  enum PowerMode {
    MODE_TORQUE,
    MODE_RPM
  };

  enum KeyState {
    OFF = 0,
    ON = 1,
    RESERVED = 2,
    NOACTION = 3
  };

  enum OperationState {
    DISABLED = 0,
    STANDBY = 1,
    ENABLE = 2,
    POWERDOWN = 3
  };

public:
  void handleCanFrame(CANFrame& frame);
  void handleTick();
  void setupDevice();
  void setOpState(OperationState op);
  void setGear(Gears gear);

  DMOC(CanHandler *canbus);
  Device::DeviceId getDeviceID();
  void setPowerMode(PowerMode mode);
  PowerMode getPowerMode();

private:
  Gears selectedGear;
  Gears actualGear;
  OperationState operationState; //the op state we want
  OperationState actualState; //what the controller is reporting it is
  int step;
  byte online; //counter for whether DMOC appears to be operating
  byte alive;
  PowerMode powerMode;

  void sendCmd1();
  void sendCmd2();
  void sendCmd3();
  void sendCmd4();
  void sendCmd5();
  byte calcChecksum(CANFrame thisFrame);

};

#endif /* DMOC_H_ */

