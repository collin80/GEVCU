/*
 * CodaMotorController.cpp
 *
 * CAN Interface to the Coda flavored UQM Powerphase 100 inverter - 
   Handles sending of commands and reception of status frames to drive
   the inverter and thus motor.  Endianess is configurable in the firmware 
   inside the UQM inverter.  This object module * uses little endian format 
   - the least significant byte is the first in order with the MSB following.
 *
Copyright (c) 2014 Jack Rickard

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

#include "CodaMotorController.h"

template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 


extern bool runThrottle; 
const uint8_t swizzleTable[] = { 0xAA, 0x7F, 0xFE, 0x29, 0x52, 0xA4, 0x9D, 0xEF, 0xB, 0x16, 0x2C, 0x58, 0xB0, 0x60, 0xC0, 1 };


	
CodaMotorController::CodaMotorController() : MotorController() 
{

    prefsHandler = new PrefHandler(CODAUQM);
    operationState = ENABLE;
    online = 0;
    activityCount = 0;
    sequence=0;
    commonName = "Coda UQM Powerphase 100 Inverter";
  
}


void CodaMotorController::setup() 
{
	TickHandler::getInstance()->detach(this);

	Logger::info("add device: CODA UQM (id:%X, %X)", CODAUQM, this);

	loadConfiguration();

	MotorController::setup(); // run the parent class version of this function

	// register ourselves as observer of all 0x20x can frames for UQM
       CanHandler::getInstanceEV()->attach(this, 0x200, 0x7f0, false);
     
    
       TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_MOTOR_CONTROLLER_CODAUQM);
      
       operationState=ENABLE;
       selectedGear=DRIVE;
       running=true;

<<<<<<< HEAD
     
=======
        sendCmd2();  //CAN watchdog reset command

>>>>>>> FETCH_HEAD
}


void CodaMotorController::handleCanFrame(CAN_FRAME *frame) 
{
	int RotorTemp, invTemp, StatorTemp;
	int temp;
	online = 1; //if a frame got to here then it passed the filter and must come from UQM

        Logger::debug("UQM inverter msg: %X", frame->id);
        
	switch (frame->id) 
        {
  
        case 0x209:  //Accurate Feedback Message 
        
              torqueActual =  (((frame->data.bytes[1] * 256) + frame->data.bytes[0])-32128) ;
              dcVoltage = (((frame->data.bytes[3] * 256) + frame->data.bytes[2])-32128);
                if(dcVoltage<1000){dcVoltage=1000;}//Lowest value we can display on dashboard
	      dcCurrent = (((frame->data.bytes[5] * 256) + frame->data.bytes[4])-32128);
              speedActual = (((frame->data.bytes[7] * 256) + frame->data.bytes[6])-32128)/2;   
              Logger::debug("UQM Actual Torque: %d DC Voltage: %d Amps: %d RPM: %d", torqueActual/10,dcVoltage/10,dcCurrent/10,speedActual);
	      break;

        case 0x20A:    //System Status Message
            Logger::debug("UQM inverter 20A System Status Message Received");
            break;


        case 0x20B:    //Emergency Fuel Cutback Message
            Logger::debug("UQM inverter 20B Emergency Fuel Cutback Message Received");
            break;
        
        case 0x20C:    //Reserved Message     
            Logger::debug("UQM inverter 20C Reserved Message Received");
            break;
        
        case 0x20D:    //Limited Torque Percentage Message    
            Logger::debug("UQM inverter 20D Limited Torque Percentage Message Received");
            break;
        
        case 0x20E:     //Temperature Feedback Message
        
                invTemp = frame->data.bytes[2];
                RotorTemp = frame->data.bytes[3];
	        StatorTemp = frame->data.bytes[4];
	        temperatureInverter = (invTemp-40)*10;
                if (RotorTemp > StatorTemp) {temperatureMotor = (RotorTemp-40)*10;}
	          else {temperatureMotor = (StatorTemp-40)*10;}		
                Logger::debug("UQM 20E Inverter temp: %d Motor temp: %d", temperatureInverter,temperatureMotor);
    		break;

	
        case 0x20F:    //CAN Watchdog Status Message           
                Logger::debug("UQM 20F CAN Watchdog status error");
                warning=true;
                sendCmd2(); //If we get a Watchdog status, we need to respond with Watchdog reset
		break;
				
	
	}
}






void CodaMotorController::handleTick() {

	MotorController::handleTick(); //kick the ball up to papa
        sendCmd1();   //Send our torque command
 
}


/*
UQM only HAS a single command CAN bus frame - address 0x204  Everything is stuffed into this one frame. It has a 5 byte payload.

Byte 1 - always set to 0

Byte 2 - Command Byte   
	Left four bits contain enable disable and forward reverse
		Bits 7/6 DISABLED =01
		Bits 7/6 ENABLE =10
		Bits 5/4 REVERSE=01
		Bits 5/4 FORWARD=10
		Example:  Enabled and Forward: 1010

	Right four bits (3 to 0) is a sequence counter that counts 0000 to 0111 and back to 0000

Byte 3 LSB of Torque command value.  

Byte 4 MSB of Torque command value   Offset is 32128

Byte 5 Security CRC byte.  

Bytes must be sent IN SEQUENCE and the CRC byte must be appropriate for bytes 2, 3, and 4.
Values above 32128 are positive torque.  Values below 32128 are negative torque
*/

void CodaMotorController::sendCmd1() 
{
	CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *)getConfiguration();
	
	CAN_FRAME output;
	output.length = 5;
	output.id = 0x204;
	output.extended = 0; //standard frame
	output.rtr = 0;
	output.data.bytes[0] = 0x00; //First byte is always zero.
      
	
      if(operationState==ENABLE)
        	{ 
        	  output.data.bytes[1] = 0x80; //1000 0000
        	}
        	else
        	 {
        	  output.data.bytes[1] = 0x40; //0100 0000
        	  }

        if(selectedGear==DRIVE) 
        	{
        		output.data.bytes[1] |= 0x20; //xx10 0000
        	 }
        	 else 
        	 	{
        	 	output.data.bytes[1] |= 0x10;//xx01 0000
        	 	}
        	 
        sequence+=1; //Increment sequence
		if (sequence==8){sequence=0;} //If we reach 8, go to zero
		output.data.bytes[1] |= sequence; //This should retain left four and add sequence count
										  //to right four bits.
        //Requested throttle is [-1000, 1000]
       //Two byte torque request in 0.1NM Can be positive or negative  
      
        torqueCommand=32128; //Set our zero offset value
        torqueRequested = ((throttleRequested * config->torqueMax) / 1000); //Calculate torque command 
        if(speedActual<config->speedMax){torqueCommand += torqueRequested;} //If actual rpm less than max rpm, add torque command to offset

        output.data.bytes[3] = (torqueCommand & 0xFF00) >> 8;
        output.data.bytes[2] = (torqueCommand & 0x00FF);
        output.data.bytes[4] = genCodaCRC(output.data.bytes[1], output.data.bytes[2], output.data.bytes[3]);
            
	CanHandler::getInstanceEV()->sendFrame(output);
        timestamp();
<<<<<<< HEAD
        Logger::debug("Torque command: %X   %X  ControlByte: %X  LSB %X  MSB: %X  CRC: %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
=======
        Logger::debug("Torque command: %X  %X  ControlByte: %X  LSB %X  MSB: %X  CRC: %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
>>>>>>> FETCH_HEAD
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4], hours, minutes, seconds, milliseconds);
          
}


void CodaMotorController::sendCmd2() {
	CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *)getConfiguration();
	
	/*In the CODA CAN bus capture logs, this command, defined in the UQM manual as a
	  207 watchdog reset, is sent every 480msec.  It also always occurs after the last count of 
	  a sequence ie 57, 67, 97, or A7.  But this may just be coincidence.
	  By the book, this is to be sent in response to a watchdog timeout.  
	  We send this in response to receipt of a 20F Watchdog status.
	  */
	
	CAN_FRAME output;
	output.length = 8;
	output.id = 0x207;
	output.extended = 0; //standard frame
	output.data.bytes[0] = 0xa5; //This is simply three given values.  The 5A appears to be 
        output.data.bytes[1] = 0xa5; //the important one.
        output.data.bytes[2] = 0x5a;
        output.data.bytes[3] = 0x00;
        output.data.bytes[4] = 0x00;
        output.data.bytes[5] = 0x00;
        output.data.bytes[6] = 0x00;
        output.data.bytes[7] = 0x00;
	   
	CanHandler::getInstanceEV()->sendFrame(output); 
        timestamp();
Logger::debug("Watchdog reset: %X  %X  %X  %d:%d:%d.%d",output.data.bytes[0], output.data.bytes[1],
output.data.bytes[2], hours, minutes, seconds, milliseconds);
  
        warning=false;
}


DeviceId CodaMotorController::getId() {
	return (CODAUQM);
}

uint32_t CodaMotorController::getTickInterval() 
{
	return CFG_TICK_INTERVAL_MOTOR_CONTROLLER_CODAUQM;
}

void CodaMotorController::loadConfiguration() {
	CodaMotorControllerConfiguration *config = (CodaMotorControllerConfiguration *)getConfiguration();

	if (!config) {
		config = new CodaMotorControllerConfiguration();
		setConfiguration(config);
	}

	MotorController::loadConfiguration(); // call parent

}

void CodaMotorController::saveConfiguration() {
	MotorController::saveConfiguration();
}

uint8_t CodaMotorController::genCodaCRC(uint8_t cmd, uint8_t torq_lsb, uint8_t torq_msb) 
{
	int counter;
	uint8_t crc;
	uint16_t temp_torq = torq_lsb + (256 * torq_msb);
	crc = 0x7F; //7F is the answer if bytes 3 and 4 are zero. We build up from there.

	//this can be done a little more efficiently but this is clearer to read
	if (((cmd & 0xA0) == 0xA0) || ((cmd & 0x60) == 0x60)) temp_torq += 1;

	//Not sure why this happens except to obfuscate the result
	if ((temp_torq % 4) == 3) temp_torq += 4;

        //increment over the bits within the torque command
	//and applies a particular XOR for each set bit.
	for (counter = 0; counter < 16; counter++)
          {
            if ((temp_torq & (1 << counter)) == (1 << counter)) crc = (byte)(crc ^ swizzleTable[counter]);
          }
    return (crc);
}



void CodaMotorController::timestamp()
{
   milliseconds = (int) (millis()/1) %1000 ;
   seconds = (int) (millis() / 1000) % 60 ;
    minutes = (int) ((millis() / (1000*60)) % 60);
    hours   = (int) ((millis() / (1000*60*60)) % 24);
    // char buffer[9]; 
    //sprintf(buffer,"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
   // Serial<<buffer<<"\n";
}

