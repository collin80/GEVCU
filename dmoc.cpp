/*
 * dmoc.c
 *
 * Interface to the DMOC - Handles sending of commands and reception of status frames
 *
 * Created: 1/13/2013 6:10:00 PM
 *  Author: Collin Kidder
 */

/*
Notes on things to add very soon:
The DMOC code should be a finite state machine which tracks what state the controller is in as opposed to the desired state and properly
transitions states. For instance, if we're in disabled mode and we want to get to enabled we've got to first set standby and wait
for the controller to signal that it has gotten there and then switch to enabled and wait until the controller has gotten there
then we can apply torque commands.

Also, the code should take into consideration the RPM for regen purposes. Of course, the controller probably already does that.

Standby torque needs to be available for some vehicles when the vehicle is placed in enabled and forward or reverse.

Should probably add EEPROM config options to support max output power and max regen power (in watts). The dmoc supports it
and I'll bet  other controllers do as well. The rest can feel free to ignore it.
*/


#include "dmoc.h"


DMOC::DMOC(CANHandler *canhandler) : MOTORCTRL(canhandler) {
	step = SPEED_TORQUE;
	selectedGear = NEUTRAL;
	opstate = DISABLED;
        actualstate = DISABLED;
	MaxTorque = 500; //in tenths so 50Nm max torque. This is plenty for testing
	MaxRPM = 6000; //also plenty for a bench test
        online = 0;
        powerMode = MODE_TORQUE; 
}


/*
Finally, the firmware actually processes some of the status messages from the DMOC
However, currently the alive and checksum bytes aren't checked for validity.
To be valid a frame has to have a different alive value than the last value we saw
and also the checksum must match the one we calculate. Right now we'll just assume
everything has gone according to plan.
*/

void DMOC::handleFrame(CANFrame& frame) {
  int RotorTemp,invTemp, StatorTemp;
  int temp;
  online = 1; //if a frame got to here then it passed the filter and must have been from the DMOC
  switch (frame.id) {
    case 0x651: //Temperature status
      RotorTemp = frame.data[0];
      invTemp = frame.data[1];
      StatorTemp = frame.data[2];
      inverterTemp = invTemp * 10;
      //now pick highest of motor temps and report it
      if (RotorTemp > StatorTemp) {
        motorTemp = RotorTemp * 10;
      }
      else {
        motorTemp = StatorTemp * 10;
      }
      break;
    case 0x23A: //torque report
      actualTorque = ((frame.data[0] * 256) + frame.data[1]) - 30000;
      break;
    case 0x23B: //speed and current operation status
      actualRPM = ((frame.data[0] * 256) + frame.data[1]) - 20000;
      temp = (OPSTATE)(frame.data[6] >> 4);
      //actually, the above is an operation status report which doesn't correspond
      //to the state enum so translate here.
      switch (temp) {
        case 0: //Initializing
          actualstate = DISABLED;
          break;
        case 1: //disabled 
          actualstate = DISABLED;
          break;
        case 2: //ready (standby)
          actualstate = STANDBY;
          break;
        case 3: //enabled
          actualstate = ENABLE;
          break;
        case 4: //Power Down
          actualstate = POWERDOWN;
          break;
        case 5: //Fault
          actualstate = DISABLED;
          break;        
        case 6: //Critical Fault
          actualstate = DISABLED;
          break;        
        case 7: //LOS
          actualstate = DISABLED;
          break;        
      }
//      SerialUSB.println(temp);
      break;
    case 0x23E: //electrical status
      //gives volts and amps for D and Q but does the firmware really care?
      break;
  }
}

void DMOC::setupDevice() {
  MOTORCTRL::setupDevice(); //first run the parent class version of this function
}

/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
  rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
*/
void DMOC::handleTick() {
  
  MOTORCTRL::handleTick(); //kick the ball up to papa
  
  switch (step) {
  case SPEED_TORQUE:
    //if (online == 1) { //only send out commands if the controller is really there.
      step = CHAL_RESP;
      sendCmd1();
      sendCmd2();
      sendCmd3();
      //sendCmd4();
      //sendCmd5();
    //}
    break;
  case CHAL_RESP:
    step = SPEED_TORQUE;
    break;
  }
}

//Commanded RPM plus state of key and gear selector
void DMOC::sendCmd1() {
	CANFrame output;
    OPSTATE newstate;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x232;
	output.ide = 0; //standard frame
	output.rtr = 0;

	if (requestedThrottle > 0 && opstate == ENABLE && selectedGear != NEUTRAL && powerMode == MODE_RPM)
		requestedRPM = 20000 + (((long)requestedThrottle * (long)MaxRPM) / 1000);
	else 
		requestedRPM = 20000;
	output.data[0] = (requestedRPM & 0xFF00) >> 8;
	output.data[1] = (requestedRPM & 0x00FF);
	output.data[2] = 0; //not used
	output.data[3] = 0; //not used
	output.data[4] = 0; //not used
	output.data[5] = ON; //key state

        //handle proper state transitions
        newstate = DISABLED;
        if (actualstate == DISABLED && (opstate == STANDBY || opstate == ENABLE)) newstate = STANDBY;
        if ((actualstate == STANDBY || actualstate == ENABLE) && opstate == ENABLE) newstate = ENABLE;
        if (opstate == POWERDOWN) newstate = POWERDOWN;
        
        output.data[6] = alive + ((byte)selectedGear << 4) + ((byte)newstate << 6); //use new automatic state system.
	//output.data[6] = alive + ((byte)selectedGear << 4) + ((byte)opstate << 6); //use old manual system
        //actualstate = opstate;
 
	output.data[7] = calcChecksum(output);

	canbus->sendFrame(5, output);
}

//Torque limits
void DMOC::sendCmd2() {
	CANFrame output;
	output.dlc = 8;
	output.id = 0x233;
	output.ide = 0; //standard frame
	output.rtr = 0;
	//30000 is the base point where torque = 0
	//MaxTorque is in tenths like it should be.
	//Requested throttle is [-1000, 1000]
	//data 0-1 is upper limit, 2-3 is lower limit. They are set to same value to lock torque to this value
	//requestedTorque = 30000L + (((long)requestedThrottle * (long)MaxTorque) / 1000L);

    requestedTorque = 30000; //set upper torque to zero if not drive enabled
    if (powerMode == MODE_TORQUE) {
       if (actualstate == ENABLE) { //don't even try sending torque commands until the DMOC reports it is ready
          if (selectedGear == DRIVE) requestedTorque = 30000L + (((long)requestedThrottle * (long)MaxTorque) / 1000L);
          if (selectedGear == REVERSE) requestedTorque = 30000L - (((long)requestedThrottle * (long)MaxTorque) / 1000L);
       }		
       output.data[0] = (requestedTorque & 0xFF00) >> 8;
       output.data[1] = (requestedTorque & 0x00FF);
       output.data[2] = output.data[0];
       output.data[3] = output.data[1];
    }
    else { //RPM mode so request max torque as upper limit and zero torque as lower limit
       output.data[0] = ((30000L + MaxTorque) & 0xFF00) >> 8;
       output.data[1] = ((30000L + MaxTorque) & 0x00FF);
       output.data[2] = 0x75;
       output.data[3] = 0x30;
    }
       
	output.data[4] = 0x75; //msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
	output.data[5] = 0x30; //lsb
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

	canbus->sendFrame(6, output);
}

//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DMOC::sendCmd3() {
	CANFrame output;
	output.dlc = 8;
	output.id = 0x234;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.data[0] = 0xD0; //msb of regen watt limit
	output.data[1] = 0x84; //lsb
	output.data[2] = 0x6C; //msb of acceleration limit
	output.data[3] = 0x66; //lsb
	output.data[4] = 0; //not used
	output.data[5] = 60; //20 degrees celsius
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

	canbus->sendFrame(7, output);
}

//challenge/response frame 1 - Really doesn't contain anything we need I dont think
void DMOC::sendCmd4() {
	CANFrame output;
	output.dlc = 8;
	output.id = 0x235;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.data[0] = 37; //i don't know what all these values are
	output.data[1] = 11; //they're just copied from real traffic
	output.data[2] = 0;
	output.data[3] = 0;
	output.data[4] = 6;
	output.data[5] = 1;
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

	canbus->sendFrame(5, output);
}

//Another C/R frame but this one also specifies which shifter position we're in
void DMOC::sendCmd5() {
	CANFrame output;
	output.dlc = 8;
	output.id = 0x236;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.data[0] = 2;
	output.data[1] = 127;
	output.data[2] = 0;
	if (opstate == ENABLE && selectedGear != NEUTRAL)  {
		output.data[3] = 52;
		output.data[4] = 26;
		output.data[5] = 59; //drive
	}
	else {
		output.data[3] = 39;
		output.data[4] = 19;
		output.data[5] = 55; //neutral
	}
	//--PRND12
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

	canbus->sendFrame(6, output);
}

void DMOC::setOpState(OPSTATE op) {
	opstate = op;
}

void DMOC::setGear(GEARS gear) {
	selectedGear = gear;
}

byte DMOC::calcChecksum(CANFrame thisFrame) {
	byte cs;
	byte i;
	cs = thisFrame.id;
	for (i = 0; i < 7; i++) cs += thisFrame.data[i];
	i = cs + 3;
	cs = ((int)256 - i);
	return cs;
}

DEVICE::DEVID DMOC::getDeviceID() {
  return (DEVICE::DMOC645);
}

void DMOC::setPowerMode(POWERMODE mode) {
  powerMode = mode;
}

DMOC::POWERMODE DMOC::getPowerMode() {
  return powerMode;
}

