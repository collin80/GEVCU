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
	torqueActual = 0;
	torqueAvailable = 0;
	mechanicalPower = 0;

	gearSwitch = GS_FAULT;

	dcVoltage = 0;
	dcCurrent = 0;
	acCurrent = 0;

	donePrecharge = false;  
    coolingfan=0;
    cooloff=0;
    coolon=0;
    coolflag=false;
    skipcounter=0;

}

DeviceType MotorController::getType() {
	return (DEVICE_MOTORCTRL);
}

void MotorController::handleTick() {
	uint8_t forwardSwitch, reverseSwitch;
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

	gearSwitch = GS_FORWARD;

	Throttle *accelerator = DeviceManager::getInstance()->getAccelerator();
	Throttle *brake = DeviceManager::getInstance()->getBrake();
	if (accelerator)
		throttleRequested = accelerator->getLevel();
	if (brake && brake->getLevel() < -10 && brake->getLevel() < accelerator->getLevel()) //if the brake has been pressed it overrides the accelerator.
		throttleRequested = brake->getLevel();

	

	if (!donePrecharge) 
	{
		throttleRequested = 0; //If we're not done precharging, reset throttle to zero no matter what it was.  We don't want to spin motor before
                               //closing contactor.
                               //Used just to watch it happen for testing.Logger::console("%i milliseconds since startup", millis());
		if (millis()> config->prechargeR) //Check milliseconds since startup against our entered delay in milliseconds
	    {
			donePrecharge=true;
            Logger::console("Precharge sequence complete after %i milliseconds", config->prechargeR);
            setOutput(config->mainContactorRelay, true); //Main contactor on
            setOutput(config->prechargeRelay, false); //ok.  Turn of precharge
            //show our work
            Logger::console("MAIN CONTACTOR ENABLED...DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d,DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3),getOutput(4), getOutput(5), getOutput(6), getOutput(7));
        }

		if(skipcounter++ > 23)    //As how fast we turn on cooling is very low priority, we only check cooling every 24th lap or about once per second
			coolingcheck();
	}

	//Logger::debug("Throttle: %d", throttleRequested);

}

void MotorController::coolingcheck()
 {
	//This routine is used to set an optional cooling fan output to on if the current temperature exceeds a specified value.
               
	skipcounter=0; //Reset our laptimer
               
	if(coolingfan<8)    //We have 8 outputs 0-7 If they entered something else, there is no point in doing this check.
	{          
		//Logger::info("Current temp: %i", temperatureInverter/10);
		//Logger::info("Cooling Fan Output: %i", coolingfan);
		//Logger::info("ON temp: %i", coolon);
		//Logger::info("OFF temp: %i", cooloff);
 
		if(temperatureInverter/10>coolon)
		{
			if(!coolflag)
			{
				setOutput(coolingfan, true);
				Logger::info("Inverter Temperature %i F exceeds %i F setting.", temperatureInverter/10, coolon);
				Logger::info("Cooling Fan Output: %i set to ON", coolingfan);
				coolflag=true;
			} 
		}
		if(temperatureInverter/10<cooloff)
		{
			if(coolflag)
			{
				setOutput(coolingfan, false);
				Logger::info("Inverter Temperature %i F below %i F setting", temperatureInverter/10, cooloff);
				Logger::info("Cooling Fan Output: %i set to OFF", coolingfan);
				coolflag=false;
			} 
		}
	}
 }



void MotorController::setup() {
	MotorControllerConfiguration *config = (MotorControllerConfiguration *)getConfiguration();

    Logger::console("PRELAY=%i - Current PreCharge Relay output", config->prechargeRelay);
    Logger::console("MRELAY=%i - Current Main Contactor Relay output", config->mainContactorRelay);
    Logger::console("PREDELAY=%i - Precharge delay time", config->prechargeR);
	 
    setOutput(config->prechargeRelay, true); //start the precharge right now
    setOutput(config->mainContactorRelay, false); //Make sure main contactor relay is off
    //show our work
    Logger::console("PRECHARGING...DOUT0:%d, DOUT1:%d, DOUT2:%d, DOUT3:%d,DOUT4:%d, DOUT5:%d, DOUT6:%d, DOUT7:%d", getOutput(0), getOutput(1), getOutput(2), getOutput(3),getOutput(4), getOutput(5), getOutput(6), getOutput(7));
    coolflag=false;


	Device::setup();
	/*
	if (config->prechargeC> 0 && config->prechargeRelay < NUM_OUTPUT) {
		//precharge time is 5RC which is (R*C / 1000) ms * 5 = RC/200 but ohms is in tenths so divide by another 10 = RC/2000
		prechargeTime = ((int)config->prechargeC * config->prechargeR) / 2000;
		Logger::info("RC precharge mode. C: %i  R: %i   Precharge time: %i ms", config->prechargeC, config->prechargeR, prechargeTime);
		setOutput(config->prechargeRelay, true); //start the precharge right now
		setOutput(config->mainContactorRelay, false); //just to be sure
	}
	else {
		Logger::info("Not precharging in RC mode");
	}
	*/
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

MotorController::GearSwitch MotorController::getGearSwitch() {
	return gearSwitch;
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
		prefsHandler->read(EEMC_PRECHARGE_C, &config->prechargeC);
		prefsHandler->read(EEMC_PRECHARGE_R, &config->prechargeR);
		prefsHandler->read(EEMC_NOMINAL_V, &config->nominalVolt);
		prefsHandler->read(EEMC_PRECHARGE_RELAY, &config->prechargeRelay);
		prefsHandler->read(EEMC_CONTACTOR_RELAY, &config->mainContactorRelay);

		PrefHandler *sysPrefs;
        sysPrefs = new PrefHandler(SYSTEM);
        sysPrefs->read(EESYS_COOLFAN, &coolingfan);
        sysPrefs->read(EESYS_COOLON, &coolon);
        sysPrefs->read(EESYS_COOLOFF, &cooloff);

	}
	else { //checksum invalid. Reinitialize values and store to EEPROM
		config->speedMax = MaxRPMValue;
		config->torqueMax = MaxTorqueValue;
		config->speedSlewRate = RPMSlewRateValue;
		config->torqueSlewRate = TorqueSlewRateValue;
		config->reversePercent = ReversePercent;
		config->prechargeC = PrechargeC;
		config->prechargeR = PrechargeR;
		config->nominalVolt = NominalVolt;
		config->prechargeRelay = PrechargeRelay;
		config->mainContactorRelay = MainContactorRelay;
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
	prefsHandler->write(EEMC_PRECHARGE_C, config->prechargeC);
	prefsHandler->write(EEMC_PRECHARGE_R, config->prechargeR);
	prefsHandler->write(EEMC_NOMINAL_V, config->nominalVolt);
	prefsHandler->write(EEMC_CONTACTOR_RELAY, config->mainContactorRelay);
	prefsHandler->write(EEMC_PRECHARGE_RELAY, config->prechargeRelay);
	prefsHandler->saveChecksum();
}
