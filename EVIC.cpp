/*
 * EVIC.cpp
 *
 * Class to interface with the Electric Vehicle Interface Controller EVIC by Andromeda Interfaces.  This Class
 * will extract operating data from the GEVCU and send to EVIC via CAN message for display.
 * It will also RECEIVE data from a JLD505 pack measurement unit if it is available and use that as a more 
 * accurate source for some data.
 *
 Copyright (c) 2015 Jack Rickard

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

#include "EVIC.h"

//This just gives us a different way of printing to serial port by streaming.
template<class T> inline Print &operator <<(Print &obj, T arg) { obj.print(arg); return obj; } 

  

/*
 * Constructor. 
 */
EVIC::EVIC() : Device()
{
    prefsHandler = new PrefHandler(EVICTUS);
    
    commonName = "Andromeda Interfaces EVIC Display";
}


/*
 * Initialization of hardware and parameters
 */
void EVIC::setup() {

	Logger::info("add device: EVIC (id: %X, %X)", EVICTUS, this);

	TickHandler::getInstance()->detach(this);//Turn off tickhandler 
        
        loadConfiguration(); //Retrieve any persistant variables
        
	Device::setup(); // run the parent class version of this function
	
	// register ourselves as observer of all 0x404 and 0x505 can frames from JLD505
        CanHandler::getInstanceCar()->attach(this, 0x404, 0x7ff, false);
        CanHandler::getInstanceCar()->attach(this, 0x505, 0x7ff, false);
	CanHandler::getInstanceCar()->attach(this, CAN_SWITCH, 0x7ff, false);
           
         MotorController* motorController = DeviceManager::getInstance()->getMotorController();
         nominalVolt=(motorController->nominalVolts); //Get default nominal volts and capacity from motorcontroller
         capacity=(motorController->capacity);//If we do NOT have a JLD505, we will use these.
        
      tickCounter = 0;
       testMode=0;
       AHf=AH;
       SOC=0;
       SOCf=0.001;
       milliAH=0.00;
       DCV=0;
       DCA=0;
       TEMPM=0;
       TEMPI=0;
       CellHi=63;
       Cello=60;
      
       elapsedtime=timemark = timemark2=millis();
	rpm=0;  //Increment all our test variables each time
        
	TickHandler::getInstance()->attach(this, CFG_TICK_INTERVAL_EVIC);

}


//This method handles particular received CAN frames we have registered for
void EVIC::handleCanFrame(CAN_FRAME *frame) 
{
	
        Logger::debug("EVIC received msg: %X   %X   %X   %X   %X   %X   %X   %X  %X", frame->id, frame->data.bytes[0],
        frame->data.bytes[1],frame->data.bytes[2],frame->data.bytes[3],frame->data.bytes[4],
        frame->data.bytes[5],frame->data.bytes[6],frame->data.bytes[7]);
        
	switch (frame->id) 
        {
  
          case 0x404:  //Battery measurement message 
              dcVoltage= word(frame->data.bytes[0],frame->data.bytes[1]);
              dcVoltage=dcVoltage;
              dcCurrent= word(frame->data.bytes[2],frame->data.bytes[3]);
              dcCurrent=dcCurrent;
              AH= word(frame->data.bytes[4],frame->data.bytes[5]);              
              capacity=frame->data.bytes[6];
              SOC=frame->data.bytes[7];
            
             
    Logger::debug("JLD404 DC Voltage: %d Amps: %d AH: %d Capacity: %d SOC: %d", dcVoltage,dcCurrent,AH,capacity,SOC);
	      timemark=millis();  //We'll use this to indicate how long since we received from a 505.
          break;

          case 0x505:    //System Status Message
          
             Power= word(frame->data.bytes[0],frame->data.bytes[1]);
             kWh= word(frame->data.bytes[2],frame->data.bytes[3]);
             kWh=kWh/10;
             celltemp1=frame->data.bytes[4]-40;
             celltemp2=frame->data.bytes[5]-40;
             celltemp3=frame->data.bytes[6]-40;
             celltemp4=frame->data.bytes[7]-40;
 
             CellHi=Cello=celltemp1;
             if(CellHi<celltemp2){CellHi=celltemp2;} //CellT to hold highest value 
             if(Cello>celltemp2){Cello=celltemp2;} //CellT to hold highest value 
             if(CellHi<celltemp3){CellHi=celltemp3;} //CellT to hold highest value 
             if(Cello>celltemp3){Cello=celltemp3;} //CellT to hold highest value 
             if(CellHi<celltemp4){CellHi=celltemp4;} //CellT to hold highest value 
             if(Cello>celltemp4){Cello=celltemp4;} //CellT to hold highest value 
         
             
            
     Logger::debug("JLD505 Message Received Power output: %d kiloWatt-Hours: %d ", Power/10,kWh/10);
             timemark=millis();
          break;
	}
}

//This method handles periodic tick calls received from the tasker. 

void EVIC::handleTick() 
{
    
   if(testMode==0)
      {
        sendCmdCurtis();
        sendCmdOrion();
       }		
    else
       {
        sendTestCmdCurtis();
        sendTestCmdOrion();
        }				
}


/*
EVIC Curtis only HAS a single command CAN bus frame - address 0x601  Everything is stuffed into this one frame. It has an 8 byte payload.

Curtis	250kbps	Motorola byte order	CAN_1	0x601	100ms Cyclic

motor_rpm          8	16	1	0	-100	10000	rpm      signed
motor_temp	  16	8	1	0	-20	200    celsius   signed
controller_temp	  24	8	1	0	-20	200    celsius   signed
motor_amps	  40	16	0.1	0	-1000	2000	amps     signed
motor_voltage	  56	16	0.1	0	0	1000	volts    signed
*/

void EVIC::sendTestCmdCurtis() 
{
 	rpm++;  //Increment all our test variables each time
        DCV++;
        DCA++;
        TEMPM++;
        TEMPI++;
        
	CAN_FRAME output;
	  output.length = 8;
	  output.id = 0x601;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
          output.fid=0;
          output.priority=0;
	  output.data.bytes[0] = highByte(rpm); 
          output.data.bytes[1] = lowByte(rpm);
          output.data.bytes[2] = TEMPM;
          output.data.bytes[3] = TEMPI;
          output.data.bytes[4] = highByte(DCA);
          output.data.bytes[5] = lowByte(DCA);
          output.data.bytes[6] = highByte(DCV);
          output.data.bytes[7] = lowByte(DCV);
      
       CanHandler::getInstanceCar()->sendFrame(output);  //Mail it.
      
      timestamp();

      Logger::debug("EVIC Message: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);
          
}

void EVIC::sendTestCmdOrion() 
{
         AH--;	
 	CAN_FRAME output;
	  output.length = 8;
	  output.id = 0x150;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
output.fid=0;
	  output.data.bytes[0] = highByte(DCA); 
          output.data.bytes[1] = lowByte(DCA);
          output.data.bytes[2] = highByte(DCV);
          output.data.bytes[3] = lowByte(DCV);
          output.data.bytes[4] = highByte(AH);
          output.data.bytes[5] = lowByte(AH);  //Pack capacity in tenths of ampere-hours
          output.data.bytes[6] = 0;  //Cell temp
          output.data.bytes[7] = 0; //Cell temp
             
	CanHandler::getInstanceCar()->sendFrame(output);  //Mail it.
        timestamp();

        Logger::debug("Orion Message1: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);

	//CAN_FRAME output;
	  output.length = 8;
	  output.id = 0x650;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
          output.fid=0;
	  output.data.bytes[0] = SOC--; //SOC in percent 0-100 
          output.data.bytes[1] = 0;  //pack internal resistance MSB
          output.data.bytes[2] = 0;  //pack internal resistance LSB
          output.data.bytes[3] = 100;//pack health 0-100%;
          output.data.bytes[4] = lowByte(DCV);
          output.data.bytes[5] = highByte(DCV); 
          output.data.bytes[6] = 0;  //pack cycles MSB
          output.data.bytes[7] = 0; //pack cycles LSB
      
    CanHandler::getInstanceCar()->sendFrame(output);  //Mail it.
    timestamp();

    Logger::debug("Orion Message2: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);
          
}

void EVIC::sendCmdCurtis() 
{
 
      MotorController* motorController = DeviceManager::getInstance()->getMotorController();
  
  if(millis()-timemark>2000)
    {
        dcCurrent=motorController->getDcCurrent();
        dcVoltage=motorController->getDcVoltage();
      
    }
    CAN_FRAME output;
	  output.length = 8;
	  output.id = 0x601;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
          output.fid=0;
	  output.data.bytes[0] = highByte(motorController->getSpeedActual()); 
          output.data.bytes[1] = lowByte(motorController->getSpeedActual());
          output.data.bytes[2] = (motorController->getTemperatureMotor()/10);
          output.data.bytes[3] = (motorController->getTemperatureInverter()/10);
          output.data.bytes[4] = highByte(dcCurrent);
          output.data.bytes[5] = lowByte(dcCurrent);
          output.data.bytes[6] = highByte(dcVoltage);
          output.data.bytes[7] = lowByte(dcVoltage);
            
      CanHandler::getInstanceCar()->sendFrame(output);  //Mail it. 
      timestamp();
      Logger::debug("EVIC Message: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);
          
}

void EVIC::sendCmdOrion() 
{
    elapsedtime = (millis() - timemark2);  
    timemark2=millis();
    
  if(millis()-timemark>2000) // Checks to see how long its been since JLD505 message was received.  
  //If more than 2 seconds, we'll use MotorController values and calculate what we need.
    {
      MotorController* motorController = DeviceManager::getInstance()->getMotorController();
      dcCurrent=(motorController->getDcCurrent());
      dcVoltage=(motorController->getDcVoltage());
        
          //dcVoltage=3320; //Test value
          Logger::debug("DC Voltage: %i Nominal Voltage: %i  Capacity: %i",dcVoltage/10,nominalVolt/10,capacity);
 
      if(!(dcVoltage<nominalVolt))
        {
          AHf=0.0; //If our reported voltage exceeds nominalVolt value, reset our amp hours to zero
          saveConfiguration();
        }  
         
         //dcCurrent=500;         //Test value
            
   Logger::debug("ElapsedTimeE: %d current %d",elapsedtime,(dcCurrent/10));
   
        AHf+= ((elapsedtime*dcCurrent/10.0)/360000); //One zero less in divisor so we have tenths in the eventual 16 bit word
        Logger::debug("Accumulated AMPERE-HOURS: %f capacity %i",AHf/10,capacity);
        SOCf=(((capacity-AHf/10)/capacity)*100);
        SOC=round(SOCf);
        AH=round(AHf);  //convert floating point to uint16_t
      
    Logger::debug("STATE OF CHARGE: %i AH: %f",SOC,AH/10.0);
       
    }	
    //Provisional calculation from motorcontroller values is complete.  Or else we skipped all that anyway.
    //Assemble the output frame
      CAN_FRAME output;
	  output.length = 8;
	  output.id = 0x150;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
          output.fid=0;
	  output.data.bytes[0] = highByte(dcCurrent); //Pack Current in tenths of an ampere.
          output.data.bytes[1] = lowByte(dcCurrent);
          output.data.bytes[2] = lowByte(dcVoltage); //Pack voltage in whole volts
          output.data.bytes[3] = highByte(dcVoltage);
          output.data.bytes[4] = highByte(AH);  //current ampere-hours in tenths of an ampere-hour
          output.data.bytes[5] = lowByte(AH);  
          output.data.bytes[6] = CellHi;  //Cell temp
          output.data.bytes[7] = Cello; //Cell temp
             
	CanHandler::getInstanceCar()->sendFrame(output);  //Mail it.
        timestamp();
Logger::debug("Orion Message1: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);

	//Assemble our 650 frame output;
	  output.length = 8;
	  output.id = 0x650;
	  output.extended = 0; //standard frame
	  output.rtr = 0;
          output.fid=0;
         
          if(SOC>110){SOC=0;}
          
          output.data.bytes[0] = SOC*2; //SOC in percent 0-100 EVIC expects this as 0-200 with 200=100%.
          output.data.bytes[1] = 0;  //pack internal resistance MSB
          output.data.bytes[2] = 0;  //pack internal resistance LSB
          output.data.bytes[3] = 100;//pack health 0-100%;
          output.data.bytes[4] = highByte(dcVoltage);
          output.data.bytes[5] = lowByte(dcVoltage); 
          output.data.bytes[6] = 0;  //pack cycles MSB
          output.data.bytes[7] = 0; //pack cycles LSB
      
    CanHandler::getInstanceCar()->sendFrame(output);  //Mail it.
    timestamp();
    Logger::debug("Orion Message2: %X  %X %X %X %X %X %X %X %X  %d:%d:%d.%d",output.id, output.data.bytes[0],
output.data.bytes[1],output.data.bytes[2],output.data.bytes[3],output.data.bytes[4],output.data.bytes[5],output.data.bytes[6],output.data.bytes[7], hours, minutes, seconds, milliseconds);
          
}


void EVIC::timestamp()
{
    milliseconds = (int) (millis()/1) %1000 ;
    seconds = (int) (millis() / 1000) % 60 ;
    minutes = (int) ((millis() / (1000*60)) % 60);
    hours   = (int) ((millis() / (1000*60*60)) % 24);
    // char buffer[9]; 
    //sprintf(buffer,"%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
   // Serial<<buffer<<"\n";
}
/*
 * Calculate the runtime in hh:mm:ss
   This runtime calculation is good for about 50 days of uptime.
   Of course, the sprintf is only good to 99 hours so that's a bit less time.
 */
char *EVIC::getTimeRunning() {
	uint32_t ms = millis();
	int seconds = (int) (ms / 1000) % 60;
	int minutes = (int) ((ms / (1000 * 60)) % 60);
	int hours = (int) ((ms / (1000 * 3600)) % 24);
	sprintf(buffer, "%02d:%02d:%02d", hours, minutes, seconds);
	return buffer;
}

DeviceType EVIC::getType() {
	return DEVICE_MISC;
}

DeviceId EVIC::getId() {
	return (EVICTUS);
}


void EVIC::loadConfiguration() {
EVICConfiguration *config = (EVICConfiguration *)getConfiguration();

        if (prefsHandler->checksumValid()) 
          { //checksum is good, read in the values stored in EEPROM	
            prefsHandler->read(EESYS_CAPACITY, &capacity);
            prefsHandler->read(EESYS_AH, &AH);
      
          }
          else
            {
              capacity = BatteryCapacity;  //Get capacity value from config.h
              prefsHandler->write(EESYS_CAPACITY, capacity); //and write it to EEPROM
             // prefsHandler->write(EESYS_AH,AH);
              prefsHandler->saveChecksum();
            }
          

}

void EVIC::saveConfiguration() {
  EVICConfiguration *config = (EVICConfiguration *)getConfiguration();

  
	//Device::saveConfiguration();  //call parent save routine

        prefsHandler->write(EESYS_CAPACITY, capacity);  //save current values to EEPROM
        prefsHandler->write(EESYS_AH, AH);  //save current values to EEPROM
        prefsHandler->saveChecksum();
        
        loadConfiguration();

}

int16_t EVIC::getdcVoltage()
 {
   return dcVoltage;
 }
int16_t  EVIC::getdcCurrent()
 {
   return dcCurrent;
 }
uint16_t  EVIC::getAH()
 {
   return AH;
 }
uint8_t  EVIC::getCapacity()
 {
   return capacity;
 }
void EVIC::setCapacity(uint8_t capacity)
 {
   capacity=capacity;
 }
uint8_t  EVIC::getSOC()
 {
   return SOC;
 }
int16_t  EVIC::getPower()
 {
   return Power;
 }
int16_t  EVIC::getkWh()
 {
   return kWh;
 }
int16_t  EVIC::getTemperatureMotor()
 {
   return temperatureMotor;
 }
int16_t  EVIC::getTemperatureInverter()
 {
   return temperatureInverter;
 }
int16_t  EVIC::getRPM()
 {
   return rpm;
 }
uint8_t  EVIC::getCellTemp1()
 {
   return celltemp1;
 }
uint8_t  EVIC::getCellTemp2()
 {
   return celltemp2;
 }
uint8_t  EVIC::getCellTemp3()
 {
   return celltemp3;
 }
uint8_t  EVIC::getCellTemp4()
 {
   return celltemp4;
 }
uint8_t  EVIC::getCellHi()
 {
   return CellHi;
 }
uint8_t  EVIC::getCello()
 {
   return Cello;
 }
    




