/*
 GEVCU.ino

 Created: 1/4/2013 1:34:14 PM
 Author: Collin Kidder

 New, new plan: Allow for an arbitrary # of devices that can have both tick and canbus handlers. These devices register themselves
 into the handler framework and specify which sort of device they are. They can have custom tick intervals and custom can filters.
 The system automatically manages when to call the tick handlers and automatically filters canbus and sends frames to the devices.
 There is a facility to send data between devices by targetting a certain type of device. For instance, a motor controller
 can ask for any throttles and then retrieve the current throttle position from them.

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

/*Changelog removed. All changes are logged to GIT */

/*
Random comments on things that should be coded up soon:
4. It is a possibility that there should be support for actually controlling the power to some of the devices.
    For instance, power could be controlled to the +12V connection at the DMOC so that it can be power cycled
    in software. But, that uses up an input and people can just cycle the key (though that resets the GEVCU too)
5. Some people (like me, Collin) have a terrible habit of mixing several coding styles. It would be beneficial to
    continue to harmonize the source code - Perhaps use a tool to do this.
6. It should be possible to limit speed and/or torque in reverse so someone doesn't kill themselves or someone else
    while gunning it in reverse - The configuration variable is there and settable now. Just need to integrate it.
7. The DMOC code duplicates a bunch of functionality that the base class also used to implement. We've got to figure
    out where the overlaps are and fix it up so that as much as possible is done generically at the base MotorController
    class and not directly in the Dmoc class.
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
//    deviceManager.addDevice(new ELM327Emu());
    deviceManager.addDevice(new ICHIPWIFI());
    deviceManager.addDevice(new CanIO());
    deviceManager.addDevice(new CanOBD2());
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
//    delayStart(10);
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

    serialConsole.printMenu();

    wifiDevice = deviceManager.getDeviceByID(ICHIP2128);
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
        ((ICHIPWIFI*) wifiDevice)->loop();
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
