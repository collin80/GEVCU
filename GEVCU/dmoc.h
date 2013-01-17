/*
 * dmoc.h
 *
 * Note that the dmoc needs to have some form of input for gear selector (drive/neutral/reverse)
 * 
 *
 * Created: 1/13/2013 9:18:36 PM
 *  Author: Collin
 */ 


#ifndef DMOC_H_
#define DMOC_H_

#include "MCP2515.h"

enum GEARS {
	NEUTRAL = 0,
	DRIVE = 1,
	REVERSE = 2,
	ERROR = 3
};
	
enum STEP {
	RPM = 0,
	TORQUE = 1,
	WATTS = 2
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

class DMOC {
  private:
	uint16_t requestedTorque;
	uint16_t requestedRPM;
	uint16_t actualTorque;
	uint16_t actualRPM;
	uint16_t MaxTorque;	//maximum torque in 0.1 Nm 
	uint16_t MaxRPM; 
	int requestedThrottle;
	int selectedGear;
	int step;
	MCP2515 * can;

    void sendCmd1();
	void sendCmd2();
	void sendCmd3();
	byte calcChecksum(Frame thisFrame);

  public:
	void handleFrame(Frame& frame);
	void handleTick();
	void setThrottle(int throt);
	DMOC(MCP2515 *canlib);
	
	
};



#endif /* DMOC_H_ */