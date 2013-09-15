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

#include "config.h"
#ifdef CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645
#include "DmocMotorController.h"

extern bool runThrottle; //TODO: remove use of global variables !

DmocMotorController::DmocMotorController() : MotorController() {
	step = SPEED_TORQUE;
	selectedGear = NEUTRAL;
	operationState = DISABLED;
	actualState = DISABLED;
	online = 0;
	powerMode = MODE_TORQUE;
//	maxTorque = 2000;
}

/*
 Finally, the firmware actually processes some of the status messages from the DmocMotorController
 However, currently the alive and checksum bytes aren't checked for validity.
 To be valid a frame has to have a different alive value than the last value we saw
 and also the checksum must match the one we calculate. Right now we'll just assume
 everything has gone according to plan.
 */

void DmocMotorController::handleCanFrame(RX_CAN_FRAME *frame) {
	int RotorTemp, invTemp, StatorTemp;
	int temp;
	online = 1; //if a frame got to here then it passed the filter and must have been from the DMOC
        //Logger::debug("dmoc msg: %i", frame->id);
	switch (frame->id) {
	case 0x651: //Temperature status
		RotorTemp = frame->data[0];
		invTemp = frame->data[1];
		StatorTemp = frame->data[2];
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
		actualTorque = ((frame->data[0] * 256) + frame->data[1]) - 30000;
		break;
	case 0x23B: //speed and current operation status
		actualRPM = ((frame->data[0] * 256) + frame->data[1]) - 20000;
		temp = (OperationState) (frame->data[6] >> 4);
		//actually, the above is an operation status report which doesn't correspond
		//to the state enum so translate here.
		switch (temp) {
		case 0: //Initializing
			actualState = DISABLED;
			break;
		case 1: //disabled
			actualState = DISABLED;
			break;
		case 2: //ready (standby)
			actualState = STANDBY;
			break;
		case 3: //enabled
			actualState = ENABLE;
			break;
		case 4: //Power Down
			actualState = POWERDOWN;
			break;
		case 5: //Fault
			actualState = DISABLED;
			break;
		case 6: //Critical Fault
			actualState = DISABLED;
			break;
		case 7: //LOS
			actualState = DISABLED;
			break;
		}
//      Logger::debug("OpState: %d", temp);
		break;
	case 0x23E: //electrical status
		//gives volts and amps for D and Q but does the firmware really care?
		break;
	}
}

void DmocMotorController::setup() {
	TickHandler::getInstance()->detach(this);
	MotorController::setup(); // run the parent class version of this function

	// register ourselves as observer of 0x23x and 0x65x can frames
	CanHandler::getInstanceEV()->attach(this, 0x230, 0x7f0, false);
	CanHandler::getInstanceEV()->attach(this, 0x650, 0x7f0, false);

	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC);
}

/*Do note that the DMOC expects all three command frames and it expect them to happen at least twice a second. So, probably it'd be ok to essentially
 rotate through all of them, one per tick. That gives us a time frame of 30ms for each command frame. That should be plenty fast.
*/
void DmocMotorController::handleTick() {
	MotorController::handleTick(); //kick the ball up to papa

	//TODO: this check somehow duplicates functionality in MotorController !
	//if the first digital input is high we'll enable drive so we can go!
	if (getDigital(0)) {
		setGear(DRIVE);
		//runThrottle = true;
		setPowerMode(MODE_TORQUE);
	}

	//but, if the second input is high we cancel the whole thing and disable the drive.
	if (getDigital(1) || !getDigital(0)) {
		setOpState(DISABLED);
		//runThrottle = false;
	}

	//if (online == 1) { //only send out commands if the controller is really there.
	step = CHAL_RESP;
	sendCmd1();
	sendCmd2();
	sendCmd3();
	//sendCmd4();
	//sendCmd5();
	//}
	
	
}

//Commanded RPM plus state of key and gear selector
void DmocMotorController::sendCmd1() {
	TX_CAN_FRAME output;
	OperationState newstate;
	alive = (alive + 2) & 0x0F;
	output.dlc = 8;
	output.id = 0x232;
	output.ide = 0; //standard frame
	output.rtr = 0;

	if (requestedThrottle > 0 && operationState == ENABLE && selectedGear != NEUTRAL && powerMode == MODE_RPM)
		requestedRPM = 20000 + (((long) requestedThrottle * (long) maxRPM) / 1000);
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
	if (actualState == DISABLED && (operationState == STANDBY || operationState == ENABLE))
		newstate = STANDBY;
	if ((actualState == STANDBY || actualState == ENABLE) && operationState == ENABLE)
		newstate = ENABLE;
	if (operationState == POWERDOWN)
		newstate = POWERDOWN;

	if (actualState == ENABLE) {
		output.data[6] = alive + ((byte) selectedGear << 4) + ((byte) newstate << 6); //use new automatic state system.
	}
	else { //force neutral gear until the system is enabled.
		output.data[6] = alive + ((byte) NEUTRAL << 4) + ((byte) newstate << 6); //use new automatic state system.
	}

	output.data[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}

//Torque limits
void DmocMotorController::sendCmd2() {
	TX_CAN_FRAME output;
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
		if (actualState == ENABLE) { //don't even try sending torque commands until the DMOC reports it is ready
			if (selectedGear == DRIVE)
				//Logger::debug("Drive - ENABLED - Torque Mode - Go!");
                requestedTorque = 30000L + (((long) requestedThrottle * (long) maxTorque) / 1000L);
			if (selectedGear == REVERSE)
				requestedTorque = 30000L - (((long) requestedThrottle * (long) maxTorque) / 1000L);
		}
		output.data[0] = (requestedTorque & 0xFF00) >> 8;
		output.data[1] = (requestedTorque & 0x00FF);
		output.data[2] = output.data[0];
		output.data[3] = output.data[1];
	}
	else { //RPM mode so request max torque as upper limit and zero torque as lower limit
		output.data[0] = ((30000L + maxTorque) & 0xFF00) >> 8;
		output.data[1] = ((30000L + maxTorque) & 0x00FF);
		output.data[2] = 0x75;
		output.data[3] = 0x30;
	}

	//what the hell is standby torque? Does it keep the transmission spinning for automatics? I don't know.
	output.data[4] = 0x75; //msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
	output.data[5] = 0x30; //lsb
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

    //Logger::debug("max torque: %i", maxTorque);
        
    //Logger::debug("requested torque: %i",(((long) requestedThrottle * (long) maxTorque) / 1000L));

	CanHandler::getInstanceEV()->sendFrame(output);
}

//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
void DmocMotorController::sendCmd3() {
	TX_CAN_FRAME output;
	output.dlc = 8;
	output.id = 0x234;
	output.ide = 0; //standard frame
	output.rtr = 0;

	int regenCalc = 65000 - (MaxRegenWatts / 4);
	int accelCalc = (MaxAccelWatts / 4);
	output.data[0] = ((regenCalc & 0xFF00) >> 8); //msb of regen watt limit
	output.data[1] = (regenCalc & 0xFF); //lsb
	output.data[2] = ((accelCalc & 0xFF00) >> 8); //msb of acceleration limit
	output.data[3] = (accelCalc & 0xFF); //lsb
	output.data[4] = 0; //not used
	output.data[5] = 60; //20 degrees celsius
	output.data[6] = alive;
	output.data[7] = calcChecksum(output);

	CanHandler::getInstanceEV()->sendFrame(output);
}

//challenge/response frame 1 - Really doesn't contain anything we need I dont think
void DmocMotorController::sendCmd4() {
	TX_CAN_FRAME output;
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

	CanHandler::getInstanceEV()->sendFrame(output);
}

//Another C/R frame but this one also specifies which shifter position we're in
void DmocMotorController::sendCmd5() {
	TX_CAN_FRAME output;
	output.dlc = 8;
	output.id = 0x236;
	output.ide = 0; //standard frame
	output.rtr = 0;
	output.data[0] = 2;
	output.data[1] = 127;
	output.data[2] = 0;
	if (operationState == ENABLE && selectedGear != NEUTRAL) {
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

	CanHandler::getInstanceEV()->sendFrame(output);
}

void DmocMotorController::setOpState(OperationState op) {
	operationState = op;
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
byte DmocMotorController::calcChecksum(TX_CAN_FRAME thisFrame) {
	byte cs;
	byte i;
	cs = thisFrame.id;
	for (i = 0; i < 7; i++)
		cs += thisFrame.data[i];
	i = cs + 3;
	cs = ((int) 256 - i);
	return cs;
}

Device::DeviceId DmocMotorController::getId() {
	return (Device::DMOC645);
}

void DmocMotorController::setPowerMode(PowerMode mode) {
	powerMode = mode;
}

DmocMotorController::PowerMode DmocMotorController::getPowerMode() {
	return powerMode;
}

uint32_t DmocMotorController::getTickInterval() 
{
	return CFG_TICK_INTERVAL_MOTOR_CONTROLLER_DMOC;
}

#endif //CFG_ENABLE_DEVICE_MOTORCTRL_DMOC_645

