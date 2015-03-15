/*
 * EVIC.h
 *
 * * Class to interface with the Electric Vehicle Interface Controller EVIC by Andromeda Interfaces.  This Class will extract operating data from the GEVCU and send to EVIC via CAN message for display.
 *
 *
 * Created: 1/28/2015
  *  Author: Jack Rickard

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

#ifndef EVIC_H_
#define EVIC_H_

#include <Arduino.h>
#include "config.h"
#include "constants.h"
#include "Device.h"
#include "TickHandler.h"
#include "CanHandler.h"
#include "DeviceManager.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"

extern PrefHandler *sysPrefs;


class EVICConfiguration: public DeviceConfiguration {
public:
  //uint8_t capacity;
};


class EVIC: public Device, CanObserver {
    public:
    
    EVIC();
    //EVIC(USARTClass *which);
    virtual void handleTick();
    virtual void handleCanFrame(CAN_FRAME *frame);
	
    virtual void setup(); //initialization on start up
    void timestamp();
    DeviceType getType();
    DeviceId getId();
    
    char *getTimeRunning();
    void loadConfiguration();
    void saveConfiguration();
    int16_t getdcVoltage();
    int16_t getdcCurrent();
    uint16_t getAH();
    uint8_t getCapacity();
    void setCapacity(uint8_t capacity);
    uint8_t getSOC();
    int16_t getPower();
    int16_t getkWh();
    int16_t getTemperatureMotor();
    int16_t getTemperatureInverter();
    int16_t getRPM();
    uint8_t getCellTemp1();
    uint8_t getCellTemp2();
    uint8_t getCellTemp3();
    uint8_t getCellTemp4();
    uint8_t getCellHi();
    uint8_t getCello();
    
        unsigned long timemark;
	unsigned long timemark2;
	int16_t torqueActual;
	int16_t speedActual;
	int16_t dcVoltage;
        int16_t dcCurrent;
        uint16_t nominalVolt;
        uint16_t AH;
        uint8_t capacity;
        uint8_t SOC;
        int16_t Power;
        int16_t kWh;
	int16_t temperatureMotor;
	int16_t temperatureInverter;
        int16_t rpm;
        uint8_t celltemp1;
        uint8_t celltemp2;
        uint8_t celltemp3;
        uint8_t celltemp4;
        uint8_t CellHi;
        uint8_t Cello;
  
    private:
    void sendTestCmdCurtis();
    void sendTestCmdOrion(); 
    void sendCmdCurtis();
    void sendCmdOrion(); 

        double AHf;
        double milliAH;
        float  SOCf;
        int milliseconds  ;
        int seconds;
        int minutes;
        int hours ;
  	unsigned long elapsedtime;
        int16_t DCV;
        int16_t DCA;
        int8_t TEMPM;
        int8_t TEMPI;
  	
    int tickCounter;
    char buffer[30]; // a buffer for various string conversions
    uint32_t lastSentTime;
    boolean weHave505;
    boolean testMode;
    
	      
};

#endif

