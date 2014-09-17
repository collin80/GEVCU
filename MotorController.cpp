/*
 * MotorController.cpp
 *
 * Parent class for all motor controllers.
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
 
#include "MotorController.h"
 
MotorController::MotorController() : Device() {
	ready = false;
	running = false;
	faulted = false;
	warning = false;

	temperatureMotor = 0;
	temperatureInverter = 0;
	temperatureSystem = 0;

	statusBitfield1 = 0;
	statusBitfield2 = 0;
	statusBitfield3 = 0;
	statusBitfield4 = 0;

	powerMode = modeTorque;
	throttleRequested = 0;
	speedRequested = 0;
	speedActual = 0;
	torqueRequested = 0;
	torqueActual = 10;
	torqueAvailable = 0;
	mechanicalPower = 0;

	selectedGear = NEUTRAL;
        operationState=ENABLE;

	dcVoltage = 0;
	dcCurrent = 0;
	acCurrent = 0;
	kiloWattHours = 0;
	nominalVolts = 0;

	donePrecharge = false;  
        prelay = false;
        coolflag = false;
        skipcounter=0;
        testenableinput=0;
        testreverseinput=0;
        premillis=0;


}

DeviceType MotorController::getType() {
	return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {

      MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
     
    //Set status annunciators
     if(ready) statusBitfield1 |=1 << 15;else statusBitfield1 &= ~(1 <<15);
     if(running) statusBitfield1 |=1 << 14;else statusBitfield1 &= ~(1 <<14);
     if(warning) statusBitfield1 |=1 << 10;else statusBitfield1 &= ~(1 <<10);
     if(faulted) statusBitfield1 |=1 << 9;else statusBitfield1 &= ~(1 <<9);
     
     //Calculate killowatts and kilowatt hours 
       mechanicalPower=dcVoltage*dcCurrent/10000; //In kilowatts. DC voltage is x10
       if (dcVoltage>nominalVolts && torqueActual>0) {kiloWattHours=1;} //If our voltage is higher than fully charged with no regen, zero our kwh meter
       if (milliStamp>millis()) {milliStamp=0;} //In case millis rolls over to zero while running
       kiloWattHours+=(millis()-milliStamp)*mechanicalPower;//We assume here that power is at current level since last tick and accrue in kilowattmilliseconds.
       milliStamp=millis();              //reset our kwms timer for next check
       
       
     //Throttle check
	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	if (accelerator)
		throttleRequested = accelerator->getLevel();
	if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) //if the brake has been pressed it overrides the accelerator.
		throttleRequested = brake->getLevel();
	//Logger::debug("Throttle: %d", throttleRequested);


      if (!donePrecharge)checkPrecharge();
          

	if(skipcounter++ > 30)    //A very low priority loop for checks that only need to be done once per second.
	{  
            skipcounter=0; //Reset our laptimer
              
                
                //Some test simulations if precharge time is set to 12345
		if(config->prechargeR==12345)
                  {
		    dcVoltage--;  
	            if (torqueActual < -500)
                      {
                        torqueActual=20;
                      }
                     else 
                       {
                         torqueActual=-650;
                       }
                        if (dcCurrent < 0)
                      {
                        dcCurrent=120;
                      }
                     else 
                       {
                         dcCurrent=-65;
                       }
                    if (temperatureInverter < config->coolOn*10)
                      {
                        temperatureInverter=(config->coolOn+2)*10;
                      }
                      else 
                        {
                          temperatureInverter=(config->coolOff-2)*10;
                        }
                
                    if (throttleRequested < 500)
                      {
                        throttleRequested=500;
                      }
                      else 
                      {
                        throttleRequested=0;
                      }
                    if(testenableinput)
                      {
                        testenableinput=false;
                      }
                      else 
                        {
                          testenableinput=true;
                        }
                        
                    if(testreverseinput)
                      {
                        testreverseinput=false;
                      }
                      else 
                        {
                          testreverseinput=true;
                        }
		  }	
            coolingcheck();
            checkBrakeLight();
            checkEnableInput();
            checkReverseInput();
            checkReverseLight();
          
            //Store kilowatt hours, but only once in awhile.
            prefsHandler->write(EEMC_KILOWATTHRS, kiloWattHours);
	    prefsHandler->saveChecksum();

   	}
}


void MotorController::checkPrecharge()
{
  	
        int prechargetime=getprechargeR();
        int contactor=getmainContactorRelay();
        int relay=getprechargeRelay();
          
        if (relay>7 || relay<0 || contactor<0 || contactor>7)  //We don't have a contactor and a precharge relay
          {
            donePrecharge=true;         //Let's end this charade.
            return;
          }
          
	  if ((millis()-premillis)< prechargetime) //Check milliseconds since startup against our entered delay in milliseconds
	    {           
              if(!prelay)
                {
	          setOutput(contactor, 0); //Make sure main contactor off
                  statusBitfield2 &= ~(1 << 17); //clear bitTurn off MAIN CONTACTOR annunciator
                  statusBitfield1 &= ~(1 << contactor);//clear bitTurn off main contactor output annunciator
                  setOutput(relay, 1); //ok.  Turn on precharge relay
                  statusBitfield2 |=1 << 19; //set bit to turn on  PRECHARGE RELAY annunciator
                  statusBitfield1 |=1 << relay; //set bit to turn ON precharge OUTPUT annunciator
                  throttleRequested = 0; //Keep throttle at zero during precharge
                  prelay=true;
                  Logger::info("Starting precharge sequence - wait %i milliseconds", prechargetime);
              
                }
            } 
            else
              {
	        setOutput(contactor, 1); //Main contactor on
	        statusBitfield2 |=1 << 17; //set bit to turn on MAIN CONTACTOR annunciator
	        statusBitfield1 |=1 << contactor;//setbit to Turn on main contactor output annunciator
		Logger::info("Precharge sequence complete after %i milliseconds", prechargetime);
                Logger::info("MAIN CONTACTOR ENABLED...DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d,DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3),getOutput(4), getOutput(5), getOutput(6), getOutput(7));
                donePrecharge=true; //Time's up.  Let's don't do ANY of this on future ticks.
                //Generally, we leave the precharge relay on.  This doesn't hurt much in any configuration.  But when using two contactors
                //one positive with a precharge resistor and one on the negative leg to act as precharge, we need to leave precharge on.
	       
            }
}

//This routine is used to set an optional cooling fan output to on if the current temperature 
//exceeds a specified value.  Annunciators are set on website to indicate status.
void MotorController::coolingcheck()
 {
	int coolfan=getCoolFan();
	            
	if(coolfan>=0 and coolfan<8)    //We have 8 outputs 0-7 If they entered something else, there is no point in doing this check.
	  {          
	    if(temperatureInverter/10>getCoolOn())  //If inverter temperature greater than COOLON, we want to turn on the coolingoutput
	      {
		if(!coolflag)
		  {
		    coolflag=1;
	            setOutput(coolfan, 1); //Turn on cooling fan output
                    statusBitfield1 |=1 << coolfan; //set bit to turn on cooling fan output annunciator
                    statusBitfield3 |=1 << 9; //Set bit to turn on OVERTEMP annunciator    
		  } 
	      }

	    if(temperatureInverter/10<getCoolOff()) //If inverter temperature falls below COOLOFF, we want to turn cooling off.
	      {
		if(coolflag)
		  {
		    coolflag=0;
		    setOutput(coolfan, 0); //Set cooling fan output off
                    statusBitfield1 &= ~(1 << coolfan); //clear bit to turn off cooling fan output annunciator
                    statusBitfield3 &= ~(1 << 9); //clear bit to turn off OVERTEMP annunciator  
		  } 
	      }
	  }
 }

//If we have a brakelight output configured, this will set it anytime regen greater than 10 Newton meters
void MotorController::checkBrakeLight()
{
 
  if(getBrakeLight() >=0 && getBrakeLight()<8)  //If we have one configured ie NOT 255 but a valid output
  {
   int brakelight=getBrakeLight();  //Get brakelight output once
 
   if(getTorqueActual() < -100)  //We only want to turn on brake light if we are have regen of more than 10 newton meters
       {
         setOutput(brakelight, 1); //Turn on brake light output
         statusBitfield1 |=1 << brakelight; //set bit to turn on brake light output annunciator
       }
       else
         {
           setOutput(brakelight, 0); //Turn off brake light output
           statusBitfield1 &= ~(1 << brakelight);//clear bit to turn off brake light output annunciator
         }
  }

}

//If a reverse light output is configured, this will turn it on anytime the gear state is in REVERSE
void MotorController::checkReverseLight()
{
  uint16_t reverseLight=getRevLight(); 
  if(reverseLight >=0 && reverseLight <8) //255 means none selected.  We don't have a reverselight output configured.
   {
     if(selectedGear==REVERSE)  //If the selected gear IS reverse
       {
         setOutput(reverseLight, true); //Turn on reverse light output
         statusBitfield1 |=1 << reverseLight; //set bit to turn on reverse light output annunciator
       }
       else
         {
           setOutput(reverseLight, false); //Turn off reverse light output
           statusBitfield1 &= ~(1 << reverseLight);//clear bit to turn off reverselight OUTPUT annunciator
         }
    }
}

//If we have an ENABLE input configured, this will set opstation to ENABLE anytime it is true (12v), DISABLED if not.
void MotorController:: checkEnableInput()
{
  uint16_t enableinput=getEnableIn();
  if(enableinput >= 0 && enableinput<4) //Do we even have an enable input configured ie NOT 255.
    {
       if((getDigital(enableinput))||testenableinput) //If it's ON let's set our opstate to ENABLE
        {
          setOpState(ENABLE);
          statusBitfield2 |=1 << enableinput; //set bit to turn on ENABLE annunciator
	  statusBitfield2 |=1 << 18;//set bit to turn on enable input annunciator
          } 
        else 
        {
          setOpState(DISABLED);//If it's off, lets set DISABLED.  These two could just as easily be reversed
          statusBitfield2 &= ~(1 << 18); //clear bit to turn off ENABLE annunciator
          statusBitfield2 &= ~(1 << enableinput);//clear bit to turn off enable input annunciator
         }           
    }
}

//IF we have a reverse input configured, this will set our selected gear to REVERSE any time the input is true, DRIVE if not
void MotorController:: checkReverseInput()
{
  uint16_t reverseinput=getReverseIn();
  if(reverseinput >= 0 && reverseinput<4)  //If we don't have a Reverse Input, do nothing
    {
    if((getDigital(reverseinput))||testreverseinput)
      {
       setSelectedGear(REVERSE); 
       statusBitfield2 |=1 << 16; //set bit to turn on REVERSE annunciator
       statusBitfield2 |=1 << reverseinput;//setbit to Turn on reverse input annunciator
       } 
        else 
        {
          setSelectedGear(DRIVE); //If it's off, lets set to DRIVE. 
          statusBitfield2 &= ~(1 << 16); //clear bit to turn off REVERSE annunciator
          statusBitfield2 &= ~(1 << reverseinput);//clear bit to turn off reverse input annunciator
          }                       
    }
}


void MotorController::setup() {
  
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	statusBitfield1 = 0;
	statusBitfield2 = 0;
	statusBitfield3 = 0;
	statusBitfield4 = 0;
        prefsHandler->read(EEMC_KILOWATTHRS, &kiloWattHours); //retrieve kilowatt hours from EEPROM
        nominalVolts=config->nominalVolt;
        donePrecharge=false;
        premillis=millis();
        

    if(config->prechargeR==12345)
      {  
	torqueActual=2;
        dcCurrent=1501;
        dcVoltage=3320;
        
      }

    Logger::console("PRELAY=%i - Current PreCharge Relay output", config->prechargeRelay);
    Logger::console("MRELAY=%i - Current Main Contactor Relay output", config->mainContactorRelay);
    Logger::console("PREDELAY=%i - Precharge delay time", config->prechargeR);
	 
    //show our work
    Logger::console("PRECHARGING...DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d,DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3),getOutput(4), getOutput(5), getOutput(6), getOutput(7));
    coolflag=false;

    Device::setup();
   
}

bool MotorController::isRunning() {
	return running;
}

bool MotorController::isFaulted() {
	return faulted;
}

bool MotorController::isWarning() {
	return warning;
}
void MotorController::setOpState(OperationState op) {
	operationState = op;
}

MotorController::OperationState MotorController::getOpState() {
	return operationState;
}
MotorController::PowerMode MotorController::getPowerMode() {
	return powerMode;
}

void MotorController::setPowerMode(PowerMode mode) {
	powerMode = mode;
}



uint32_t MotorController::getStatusBitfield1() {
	return statusBitfield1;
}

uint32_t MotorController::getStatusBitfield2() {
	return statusBitfield2;
}

uint32_t MotorController::getStatusBitfield3() {
	return statusBitfield3;
}

uint32_t MotorController::getStatusBitfield4() {
	return statusBitfield4;
}

int8_t MotorController::getCoolFan() {
  MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->coolFan;
}
int8_t MotorController::getCoolOn() {
  MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
      return config->coolOn;
}

int8_t MotorController::getCoolOff() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->coolOff;
}
int8_t MotorController::getBrakeLight() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->brakeLight;
}
int8_t MotorController::getRevLight() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->revLight;
}
int8_t MotorController::getEnableIn() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->enableIn;
}
int8_t MotorController::getReverseIn() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->reverseIn;
}

int16_t MotorController::getprechargeR() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->prechargeR;
}


int8_t MotorController::getprechargeRelay() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->prechargeRelay;
}


int8_t MotorController::getmainContactorRelay() {
    MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
	return config->mainContactorRelay;
}

int16_t MotorController::getThrottle() {
	return throttleRequested;
}

int16_t MotorController::getSpeedRequested() {
	return speedRequested;
}

int16_t MotorController::getSpeedActual() {
	return speedActual;
}

int16_t MotorController::getTorqueRequested() {
	return torqueRequested;
}

int16_t MotorController::getTorqueActual() {
	return torqueActual;
}

 MotorController::Gears MotorController::getSelectedGear() {
	return selectedGear;
}
void MotorController::setSelectedGear(Gears gear) {
	selectedGear=gear;
}


int16_t MotorController::getTorqueAvailable() {
	return torqueAvailable;
}

uint16_t MotorController::getDcVoltage() {
	return dcVoltage;
}

int16_t MotorController::getDcCurrent() {
	return dcCurrent;
}

uint16_t MotorController::getAcCurrent() {
	return acCurrent;
}

int16_t MotorController::getnominalVolt() {
	return nominalVolts;
}

uint32_t MotorController::getKiloWattHours() {
	return kiloWattHours;
}

int16_t MotorController::getMechanicalPower() {
	return mechanicalPower;
}

int16_t MotorController::getTemperatureMotor() {
	return temperatureMotor;
}

int16_t MotorController::getTemperatureInverter() {
	return temperatureInverter;
}

int16_t MotorController::getTemperatureSystem() {
	return temperatureSystem;
}



uint32_t MotorController::getTickInterval() {
        return CFG_TICK_INTERVAL_MOTOR_CONTROLLER;
}

bool MotorController::isReady() {
	return false;
}

void MotorController::loadConfiguration() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	Device::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED
	if (false) {
#else
	if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
		prefsHandler->read(EEMC_MAX_RPM, &config->speedMax);
		prefsHandler->read(EEMC_MAX_TORQUE, &config->torqueMax);
		prefsHandler->read(EEMC_RPM_SLEW_RATE, &config->speedSlewRate);
		prefsHandler->read(EEMC_TORQUE_SLEW_RATE, &config->torqueSlewRate);
		prefsHandler->read(EEMC_REVERSE_LIMIT, &config->reversePercent);
		prefsHandler->read(EEMC_KILOWATTHRS, &config->kilowattHrs);
		prefsHandler->read(EEMC_PRECHARGE_R, &config->prechargeR);
		prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
		prefsHandler->read(EEMC_PRECHARGE_RELAY, &config->prechargeRelay);
		prefsHandler->read(EEMC_CONTACTOR_RELAY, &config->mainContactorRelay);
		prefsHandler->read(EEMC_COOL_FAN, &config->coolFan);
        prefsHandler->read(EEMC_COOL_ON, &config->coolOn);
        prefsHandler->read(EEMC_COOL_OFF, &config->coolOff);
        prefsHandler->read(EEMC_BRAKE_LIGHT, &config->brakeLight);
		prefsHandler->read(EEMC_REV_LIGHT, &config->revLight);
		prefsHandler->read(EEMC_ENABLE_IN, &config->enableIn);
		prefsHandler->read(EEMC_REVERSE_IN, &config->reverseIn);

	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		config->speedMax = MaxRPMValue;
		config->torqueMax = MaxTorqueValue;
		config->speedSlewRate = RPMSlewRateValue;
		config->torqueSlewRate = TorqueSlewRateValue;
		config->reversePercent = ReversePercent;
		config->kilowattHrs = KilowattHrs;
		config->prechargeR = PrechargeR;
		config->nominalVolt = NominalVolt;
		config->prechargeRelay = PrechargeRelay;
		config->mainContactorRelay = MainContactorRelay;
		config->coolFan = CoolFan;
		config->coolOn = CoolOn;
		config->coolOff = CoolOff;
		config->brakeLight = BrakeLight;
		config->revLight = RevLight;
		config->enableIn = EnableIn;
		config->reverseIn = ReverseIn;

	}
           //DeviceManager::getInstance()->sendMessage(DEVICE_WIFI, ICHIP2128, MSG_CONFIG_CHANGE, NULL);

	Logger::info("MaxTorque: %i MaxRPM: %i", config->torqueMax, config->speedMax);
}

void MotorController::saveConfiguration() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	Device::saveConfiguration(); // call parent

	prefsHandler->write(EEMC_MAX_RPM, config->speedMax);
	prefsHandler->write(EEMC_MAX_TORQUE, config->torqueMax);
	prefsHandler->write(EEMC_RPM_SLEW_RATE, config->speedSlewRate);
	prefsHandler->write(EEMC_TORQUE_SLEW_RATE, config->torqueSlewRate);
	prefsHandler->write(EEMC_REVERSE_LIMIT, config->reversePercent);
	prefsHandler->write(EEMC_KILOWATTHRS, config->kilowattHrs);
	prefsHandler->write(EEMC_PRECHARGE_R, config->prechargeR);
	prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
	prefsHandler->write(EEMC_CONTACTOR_RELAY, config->mainContactorRelay);
	prefsHandler->write(EEMC_PRECHARGE_RELAY, config->prechargeRelay);
	prefsHandler->write(EEMC_COOL_FAN, config->coolFan);
	prefsHandler->write(EEMC_COOL_ON, config->coolOn);
	prefsHandler->write(EEMC_COOL_OFF, config->coolOff);
	prefsHandler->write(EEMC_BRAKE_LIGHT, config->brakeLight);
	prefsHandler->write(EEMC_REV_LIGHT, config->revLight);
	prefsHandler->write(EEMC_ENABLE_IN, config->enableIn);
	prefsHandler->write(EEMC_REVERSE_IN, config->reverseIn);


	prefsHandler->saveChecksum();
	loadConfiguration();
}
