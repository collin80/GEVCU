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

	dcVoltage = 0;
	dcCurrent = 0;
	acCurrent = 0;
	kiloWattHours = 0;
	nominalVolts = 0;

	donePrecharge = false;  
    prelay = false;
    coolflag = false;
    skipcounter=0;

}

DeviceType MotorController::getType() {
	return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {
	uint8_t forwardSwitch, reverseSwitch;
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	//gearSwitch = GS_FORWARD;

	//Initializing annunciator panel
    // ready=0;
     //running=0;
     //warning=0;
     //faulted=0;
     if(ready) statusBitfield1 |=1 << 15;else statusBitfield1 &= ~(1 <<15);
     if(running) statusBitfield1 |=1 << 14;else statusBitfield1 &= ~(1 <<14);
     if(warning) statusBitfield1 |=1 << 10;else statusBitfield1 &= ~(1 <<10);
     if(faulted) statusBitfield1 |=1 << 9;else statusBitfield1 &= ~(1 <<9);
     
     //killowatts and kilowatt hours 
     
     mechanicalPower=dcVoltage*dcCurrent/10000; //In kilowatts. DC voltage is x10
     //Logger::console("POWER: %d", mechanicalPower);
      //Logger::console("dcVoltage: %d", dcVoltage);
      //Logger::console("nominalVolts: %d", nominalVolts);
    //Logger::console("kilowatthours: %d", kiloWattHours);
 
  
   if (dcVoltage>nominalVolts && torqueActual>0) {kiloWattHours=1;} //If our voltage is higher than fully charged with no regen, zero our kwh meter
     if (milliStamp>millis()) {milliStamp=0;} //In case millis rolls over to zero while running
     kiloWattHours+=(millis()-milliStamp)*mechanicalPower;//We assume here that power is at current level since last tick and accrue in kilowattmilliseconds.
     milliStamp=millis();//reset our kwms timer for next check

	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	if (accelerator)
		throttleRequested = accelerator->getLevel();
	if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) //if the brake has been pressed it overrides the accelerator.
		throttleRequested = brake->getLevel();

	if (!donePrecharge && config->prechargeR>0) 
	{
		if (millis()> config->prechargeR) //Check milliseconds since startup against our entered delay in milliseconds
		{           
			donePrecharge=1; //Time's up.  Let's don't do ANY of this on future ticks.
			if (config->mainContactorRelay!=255) //If we HAVE a main contactor, turn it on.
			{
			setOutput(config->mainContactorRelay, 1); //Main contactor on
			statusBitfield2 |=1 << 17; //set bit to turn on MAIN CONTACTOR annunciator
			statusBitfield1 |=1 << config->mainContactorRelay;//setbit to Turn on main contactor output annunciator
			//I've commented the below out to leave the precharge relay ON after precharge..see user guide for why.
            //setOutput(config->prechargeRelay, 0); //ok.  Turn off precharge output
            // statusBitfield2 &= ~(1 << 19); //clearbit to turn off PRECHARGE annunciator
            //statusBitfield1 &= ~(1<< config->prechargeRelay); //clear bit to turn off PRECHARGE output annunciator
            Logger::console("Precharge sequence complete after %i milliseconds", config->prechargeR);
            Logger::console("MAIN CONTACTOR ENABLED...DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d,DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3),getOutput(4), getOutput(5), getOutput(6), getOutput(7));
            }
        }
        else  //If time is not up and maybe this is our first tick, let's set the precharge relay IF we have one
             //and clear the main contactor IF we have one.We'll set PRELAY so we only have to do this once.
        {
			if (config->prechargeRelay==255 || config->mainContactorRelay==255){donePrecharge=1;}
			if(!prelay)
            {
				if (config->prechargeRelay!=255) 
                {
					setOutput(config->prechargeRelay, 1); //ok.  Turn on precharge
                    statusBitfield2 |=1 << 19; //set bit to turn on  PRECHARGE RELAY annunciator
                    statusBitfield1 |=1 << config->prechargeRelay; //set bit to turn ON precharge OUTPUT annunciator
                    throttleRequested = 0; //Keep throttle at zero during precharge
                    prelay=true;
                }
                if (config->mainContactorRelay!=255) 
                {
					setOutput(config->mainContactorRelay, 0); //Main contactor off
                    statusBitfield2 &= ~(1 << 17); //clear bitTurn off MAIN CONTACTOR annunciator
                    statusBitfield1 &= ~(1 << config->mainContactorRelay);//clear bitTurn off main contactor output annunciator
                    prelay=true;
                }
            }
        }
    }
	//Logger::debug("Throttle: %d", throttleRequested);
	if(skipcounter++ > 30)    //As how fast we turn on cooling is very low priority, we only check cooling every 24th lap or about once per second
	{
		if(config->prechargeR==12345)
        {
		dcVoltage--;  
	        if (torqueActual < -500)
				{torqueActual=20;}
            else {torqueActual=-650;}
		}	
//      Logger::console("New voltage: %d", dcVoltage);
//		Logger::console("Nominal voltage: %d", config->nominalVolt);
//		Logger::console("KiloWattHours: %d", kiloWattHours);
        coolingcheck();
        prefsHandler->write(EEMC_KILOWATTHRS, kiloWattHours);
		prefsHandler->saveChecksum();
	}
}

//This routine is used to set an optional cooling fan output to on if the current temperature 
//exceeds a specified value.  Annunciators are set on website to indicate status.
void MotorController::coolingcheck()
 {
	skipcounter=0; //Reset our laptimer
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();
                  
	if(config->coolFan<8)    //We have 8 outputs 0-7 If they entered something else, there is no point in doing this check.
	{          
	    if(temperatureInverter/10>config->coolOn)
	    {
			if(!coolflag)
			{
				coolflag=1;
	            setOutput(config->coolFan, 1); //Turn on cooling fan output
                statusBitfield1 |=1 << config->coolFan; //set bit to turn on cooling fan output annunciator
                statusBitfield3 |=1 << 9; //Set bit to turn on OVERTEMP annunciator    
			} 
		}

	    if(temperatureInverter/10<config->coolOff)
	    {
			if(coolflag)
			{
				coolflag=0;
				setOutput(config->coolFan, 0); //Set cooling fan output off
                statusBitfield1 &= ~(1 << config->coolFan); //clear bit to turn off cooling fan output annunciator
                statusBitfield3 &= ~(1 << 9); //clear bit to turn off OVERTEMP annunciator  
			} 
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

MotorController::PowerMode MotorController::getPowerMode() {
	return powerMode;
}

void MotorController::setPowerMode(PowerMode mode) {
	powerMode = mode;
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
