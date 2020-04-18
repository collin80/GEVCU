/*
 GEVCU.ino

 Created: 1/4/2013 1:34:14 PM
 Author: Michael Neuweiler

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

#include "GEVCU.h"

// The following includes are required in the .ino file by the Arduino IDE in order to properly
// identify the required libraries for the build.
#include <due_can.h>
#include <due_wire.h>
#include <DueTimer.h>

//Evil, global variables
Device *wifiDevice;
Device *btDevice;
PerfTimer *mainLoopTimer;

void createDevices()
{
    deviceManager.addDevice(new Heartbeat());
    deviceManager.addDevice(new PotThrottle());
    deviceManager.addDevice(new CanThrottle());
    deviceManager.addDevice(new PotBrake());
    deviceManager.addDevice(new CanBrake());
    deviceManager.addDevice(new DmocMotorController());
    deviceManager.addDevice(new CodaMotorController());
    deviceManager.addDevice(new BrusaDMC5());
    deviceManager.addDevice(new BrusaBSC6());
    deviceManager.addDevice(new BrusaNLG5());
    deviceManager.addDevice(new ThinkBatteryManager());
    deviceManager.addDevice(new OrionBMS());
//    deviceManager.addDevice(new ELM327Emu());
    deviceManager.addDevice(new WifiIchip2128());
    deviceManager.addDevice(new WifiEsp32());
    deviceManager.addDevice(new CanIO());
    deviceManager.addDevice(new CanOBD2());
    deviceManager.addDevice(new StatusIndicator());
}

void delayStart(uint8_t seconds) {
    for (int i = seconds; i > 0; i--) {
        SerialUSB.println(i);
        delay(1000);
    }
}

void setup()
{
    SerialUSB.begin(CFG_SERIAL_SPEED);
//    delayStart(3);
    SerialUSB.println(CFG_VERSION);

    // resets CPU when power drops below 2.8V --> give the EEPROM enough time to finish an ongoing write at power-down
    SUPC->SUPC_SMMR = 0xA | (1<<8) | (1<<12);

    memCache.setup();
    faultHandler.setup();
    systemIO.setup();
    canHandlerEv.setup();
    canHandlerCar.setup();

    createDevices();
    /*
     *  We defer setting up the devices until here. This allows all objects to be instantiated
     *  before any of them set up. That in turn allows the devices to inspect what else is
     *  out there as they initialize. For instance, a motor controller could see if a BMS
     *  exists and supports a function that the motor controller wants to access.
     */
    status.setSystemState(Status::init);

    // if no dcdc converter is enabled, set status to true to keep high power devices running
    if (deviceManager.getDcDcConverter() == NULL) {
        status.dcdcRunning = true;
    }

    serialConsole.printMenu();

    wifiDevice = deviceManager.getDeviceByType(DEVICE_WIFI);
    btDevice = deviceManager.getDeviceByID(ELM327EMU);

    status.setSystemState(Status::preCharge);

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer = new PerfTimer();
#endif
}

void loop()
{
#ifdef CFG_EFFICIENCY_CALCS
	static int counts = 0;
	counts++;
	if (counts > 200000) {
		counts = 0;
		mainLoopTimer->printValues();
	}

	mainLoopTimer->start();
#endif

    tickHandler.process();
    canHandlerEv.process();
    canHandlerCar.process();

    serialConsole.loop();

    //TODO: this is dumb... shouldn't have to manually do this. Devices should be able to register loop functions
    if (wifiDevice != NULL) {
        ((Wifi*) wifiDevice)->process();
    }

//    if (btDevice != NULL) {
//        ((ELM327Emu*) btDevice)->loop();
//    }

    //this should still be here. It checks for a flag set during an interrupt
    systemIO.ADCPoll();

#ifdef CFG_EFFICIENCY_CALCS
	mainLoopTimer->stop();
#endif
}
