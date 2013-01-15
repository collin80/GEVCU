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

class DMOC {
  private:
	uint16_t requestedTorque;
	uint16_t requestedRPM;
	uint16_t actualTorque;
	uint16_t actualRPM;
	uint16_t MaxTorque;	//maximum torque in 0.1 Nm 


  public:
	void handleFrame(Frame& frame);
	void handleTick();
	
};



#endif /* DMOC_H_ */