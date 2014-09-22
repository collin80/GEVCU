/*
 * DmocMotorController.cpp
 *
 * Interface to the DMOC - Handles sending of commands and reception of status frames
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

#include "DmocMotorController.h"

extern bool runThrottle; //TODO: remove use of global variables !


DmocMotorController::DmocMotorController() : MotorController() {
	prefsHandler = new PrefHandler(DMOC645);
	step = SPEED_TORQUE;
	
    selectedGear = NEUTRAL;
	operationState = DISABLED;
	actualState = DISABLED;
	online = 0;
	activityCount = 0;
//	maxTorque = 2000;
	commonName = "DMOC645 Inverter";
}

void DmocMotorController::setup() {
	TickHandler::getInstance()->detach(this);

	Logger::info("add device: DMOC645 (id:%X, %X)", DMOC645, this);

	loadConfiguration();
	MotorController::setup(); // run the parent class version of this function

	// register ourselves as observer of 0x23x and 0x65x can frames
	CanHandler::getInstanceEV()->attach(this, 0x230, 0x7f0, false);
	CanHandler::getInstanceEV()->attach(this, 0x650, 0x7f0, false);

	running = false;
	setPowerMode(modeTorque);
	

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC);
}

/*
 Finally, the firmware actually processes some of the status messages from the DmocMotorController
 However, currently the alive and checksum bytes aren't checked for validity.
 To be valid a frame has to have a different alive value than the last value we saw
 and also the checksum must match the one we calculate. Right now we'll just assume
 everything has gone according to plan.
 */

void DmocMotorController::handleCanFrame(CAN_FRAME *frame) {
	int RotorTemp, invTemp, StatorTemp;
	int temp;
	online = 1; //if a frame got to here then it passed the filter and must have been from the DMOC
      
        //Logger::debug("DMOC CAN received: %X  %X  %X  %X  %X  %X  %X  %X  %X", frame->id,frame->data.bytes[0] ,frame->data.bytes[1],frame->data.bytes[2],frame->data.bytes[3],frame->data.bytes[4],frame->data.bytes[5],frame->data.bytes[6],frame->data.bytes[70]);
  
  
	switch (frame->id) {
	case 0x651: //Temperature status
		RotorTemp = frame->data.bytes[0];
		invTemp = frame->data.bytes[1];
		StatorTemp = frame->data.bytes[2];
		temperatureInverter = (invTemp-40) *10;
		//now pick highest of motor temps and report it
		if (RotorTemp > StatorTemp) {
			temperatureMotor = (RotorTemp-40) *10;
		}
		else {
			temperatureMotor = (StatorTemp-40) *10;
		}
		activityCount++;
		break;
	case 0x23A: //torque report
		torqueActual = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]) - 30000;
		activityCount++;
		break;
		
	case 0x23B: //speed and current operation status
		speedActual = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]) - 20000;
		temp = (OperationState) (frame->data.bytes[6] >> 4);
		//actually, the above is an operation status report which doesn't correspond
		//to the state enum so translate here.
		switch (temp) {
		
		case 0: //Initializing
			actualState = DISABLED;
			faulted=false;
			break;
			
		case 1: //disabled
			actualState = DISABLED;
			faulted=false;
			break;
			
		case 2: //ready (standby)
			actualState = STANDBY;
			faulted=false;
			ready = true;
			break;
			
		case 3: //enabled
			actualState = ENABLE;
			faulted=false;
			break;
			
		case 4: //Power Down
			actualState = POWERDOWN;
			faulted=false;
			break;
			
		case 5: //Fault
			actualState = DISABLED;
			faulted=true;
			break;
			
		case 6: //Critical Fault
			actualState = DISABLED;
			faulted=true;
			break;
			
		case 7: //LOS
			actualState = DISABLED;
			faulted=true;
			break;
		}
      Logger::debug("OpState: %d", temp);
		activityCount++;
		break;
		
	//case 0x23E: //electrical status
		//gives volts and amps for D and Q but does the firmware really care?
		//break;
		
	case 0x650: //HV bus status
		dcVoltage = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]);
		dcCurrent = ((frame->data.bytes[2] * 256) + frame->data.bytes[3]) - 5000; //offset is 500A, unit = .1A
		activityCount++;
		break;
	}
}

/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
 rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
*/
void DmocMotorController::handleTick() {

	MotorController::handleTick(); //kick the ball up to papa

	if (activityCount > 0) {
		activityCount--;
		if (activityCount > 60) activityCount = 60;
		if (activityCount > 40) //If we are receiving regular CAN messages from DMOC, this will very quickly get to over 40. We'll limit
								// it to 60 so if we lose communications, within 20 ticks we will decrement below this value.
			{
			if(getEnableIn()>4)setOpState(ENABLE); //If we HAVE an enableinput 0-3, we'll let that handle opstate. Otherwise set it to ENABLE
			if(getReverseIn()>4)setSelectedGear(DRIVE); //If we HAVE a reverse input, we'll let that determine forward/reverse.  Otherwise set it to DRIVE
			running=true; //Lets's set this on as long as we are 40 frames ahead.   
		    }
	  }
	else {
		setSelectedGear(NEUTRAL); //We will stay in NEUTRAL until we get at least 40 frames ahead indicating continous communications.
		running=false;
	}

	

	sendCmd1();  //This actually sets our GEAR and our actualstate cycle
	sendCmd2();  //This is our torque command
	sendCmd3();
	//sendCmd4();  //These appear to be not needed.
	//sendCmd5();  //But we'll keep them for future reference
	
	
}

//Commanded RPM plus state of key and gear selector
void DmocMotorController::sendCmd1() {
	DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *)getConfiguration();
	CAN_FRAME output;
	OperationState newstate;
	alive = (alive + 2) & 0x0F;
	output.length = 8;
	output.id = 0x232;
	output.extended = 0; //standard frame
	output.rtr = 0;

	if (throttleRequested > 0 && operationState == ENABLE && selectedGear != NEUTRAL && powerMode == modeSpeed)
		speedRequested = 20000 + (((long) throttleRequested * (long) config->speedMax) / 1000);
	else
		speedRequested = 20000;
	output.data.bytes[0] = (speedRequested & 0xFF00) >> 8;
	output.data.bytes[1] = (speedRequested & 0x00FF);
	output.data.bytes[2] = 0; //not used
	output.data.bytes[3] = 0; //not used
	output.data.bytes[4] = 0; //not used
	output.data.bytes[5] = ON; //key state

	//handle proper state transitions
	newstate = DISABLED;
	if (actualState == DISABLED && (operationState == STANDBY || operationState == ENABLE))
		newstate = STANDBY;
	if ((actualState == STANDBY || actualState == ENABLE) && operationState == ENABLE)
		newstate = ENABLE;
	if (operationState == POWERDOWN)
		newstate = POWERDOWN;

	if (actualState == ENABLE) {
		output.data.bytes[6] = alive + ((byte) selectedGear << 4) + ((byte) newstate << 6); //use new automatic state system.
	}
	else { //force neutral gear until the system is enabled.
		output.data.bytes[6] = alive + ((byte) NEUTRAL << 4) + ((byte) newstate << 6); //use new automatic state system.
	}

	output.data.bytes[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}

//Torque limits
void DmocMotorController::sendCmd2() {
	DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *)getConfiguration();
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x233;
	output.extended = 0; //standard frame
	output.rtr = 0;
	//30000 is the base point where torque = 0
	//MaxTorque is in tenths like it should be.
	//Requested throttle is [-1000, 1000]
	//data 0-1 is upper limit, 2-3 is lower limit. They are set to same value to lock torque to this value
	//torqueRequested = 30000L + (((long)throttleRequested * (long)MaxTorque) / 1000L);

	torqueCommand = 30000; //set offset  for zero torque commanded
	
        torqueRequested=0;
		if (actualState == ENABLE) { //don't even try sending torque commands until the DMOC reports it is ready
			if (selectedGear == DRIVE)
                            torqueRequested = (((long) throttleRequested * (long) config->torqueMax) / 1000L);
			if (selectedGear == REVERSE)
			    torqueRequested = (((long) throttleRequested * -1 *(long) config->torqueMax) / 1000L);//If reversed, regen becomes positive torque and positive pedal becomes regen.  Let's reverse this by reversing the sign.  In this way, we'll have gradually diminishing positive torque (in reverse, regen) followed by gradually increasing regen (positive torque in reverse.)
		}
                  
                if(speedActual < config->speedMax){torqueCommand+=torqueRequested;} //If actual rpm is less than max rpm, add torque to offset
                   else {torqueCommand += torqueRequested /2;}                       // else torque is halved
		output.data.bytes[0] = (torqueCommand & 0xFF00) >> 8;
		output.data.bytes[1] = (torqueCommand & 0x00FF);
		output.data.bytes[2] = output.data.bytes[0];
		output.data.bytes[3] = output.data.bytes[1];
	
	//what the hell is standby torque? Does it keep the transmission spinning for automatics? I don't know.
	output.data.bytes[4] = 0x75; //msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
	output.data.bytes[5] = 0x30; //lsb
	output.data.bytes[6] = alive;
	output.data.bytes[7] = calcChecksum(output);

    //Logger::debug("max torque: %i", maxTorque);
        
    //Logger::debug("requested torque: %i",(((long) throttleRequested * (long) maxTorque) / 1000L));

	CanHandler::getInstanceEV()->sendFrame(output);
        timestamp();
        Logger::debug("Torque command: MSB: %X  LSB: %X  %X  %X  %X  %X  %X  CRC: %X  %d:%d:%d.%d",output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);
 
}

//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DmocMotorController::sendCmd3() {
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x234;
	output.extended = 0; //standard frame
	output.rtr = 0;

	int regenCalc = 65000 - (MaxRegenWatts / 4);
	int accelCalc = (MaxAccelWatts / 4);
	output.data.bytes[0] = ((regenCalc & 0xFF00) >> 8); //msb of regen watt limit
	output.data.bytes[1] = (regenCalc & 0xFF); //lsb
	output.data.bytes[2] = ((accelCalc & 0xFF00) >> 8); //msb of acceleration limit
	output.data.bytes[3] = (accelCalc & 0xFF); //lsb
	output.data.bytes[4] = 0; //not used
	output.data.bytes[5] = 60; //20 degrees celsius
	output.data.bytes[6] = alive;
	output.data.bytes[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}

//challenge/response frame 1 - Really doesn't contain anything we need I dont think
void DmocMotorController::sendCmd4() {
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x235;
	output.extended = 0; //standard frame
	output.rtr = 0;
	output.data.bytes[0] = 37; //i don't know what all these values are
	output.data.bytes[1] = 11; //they're just copied from real traffic
	output.data.bytes[2] = 0;
	output.data.bytes[3] = 0;
	output.data.bytes[4] = 6;
	output.data.bytes[5] = 1;
	output.data.bytes[6] = alive;
	output.data.bytes[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}

//Another C/R frame but this one also specifies which shifter position we're in
void DmocMotorController::sendCmd5() {
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x236;
	output.extended = 0; //standard frame
	output.rtr = 0;
	output.data.bytes[0] = 2;
	output.data.bytes[1] = 127;
	output.data.bytes[2] = 0;
	if (operationState == ENABLE && selectedGear != NEUTRAL) {
		output.data.bytes[3] = 52;
		output.data.bytes[4] = 26;
		output.data.bytes[5] = 59; //drive
	}
	else {
		output.data.bytes[3] = 39;
		output.data.bytes[4] = 19;
		output.data.bytes[5] = 55; //neutral
	}
	//--PRND12
	output.data.bytes[6] = alive;
	output.data.bytes[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}


void DmocMotorController::setGear(Gears gear) {
	selectedGear = gear;
	//if the gear was just set to drive or reverse and the DMOC is not currently in enabled
	//op state then ask for it by name
	if (selectedGear != NEUTRAL) {
		operationState = ENABLE;
	}
	//should it be set to standby when selecting neutral? I don't know. Doing that prevents regen
	//when in neutral and I don't think people will like that.
}

//this might look stupid. You might not believe this is real. It is. This is how you
//calculate the checksum for the DMOC frames.
byte DmocMotorController::calcChecksum(CAN_FRAME thisFrame) {
	byte cs;
	byte i;
	cs = thisFrame.id;
	for (i = 0; i < 7; i++)
		cs += thisFrame.data.bytes[i];
	i = cs + 3;
	cs = ((int) 256 - i);
	return cs;
}

DeviceId DmocMotorController::getId() {
	return (DMOC645);
}

uint32_t DmocMotorController::getTickInterval() 
{
	return CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC;
}

void DmocMotorController::loadConfiguration() {
	DmocMotorControllerConfiguration *config = (DmocMotorControllerConfiguration *)getConfiguration();

	if (!config) {
		config = new DmocMotorControllerConfiguration();
		setConfiguration(config);
	}

	MotorController::loadConfiguration(); // call parent
}

void DmocMotorController::saveConfiguration() {
	MotorController::saveConfiguration();
}

void DmocMotorController::timestamp()
{
   milliseconds = (int) (millis()/1) %1000 ;
   seconds = (int) (millis() / 1000) % 60 ;
    minutes = (int) ((millis() / (1000*60)) % 60);
    hours   = (int) ((millis() / (1000*60*60)) % 24);
    // char buffer[9]; 
    //sprintf(buffer,"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
   // Serial<<buffer<<"\n";
    

}
