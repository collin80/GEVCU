GEVCU
=====

Generalized Electric Vehicle Control Unit


A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

The project now builds in the Arduino IDE. So, use it to compile, send the firmware
to the macchina, and monitor serial. It all works very nicely.

The master branch of this project is now switched to support the Due. The older Macchina
code has been moved to its own branch. ArduinoDue branch will be bleeding edge and pushed
to the master branch when future revisions yield known good configurations. At this point the master
branch does work for motor control without need for a PC.

You will need the following to have any hope of compiling and running the firmware:
- An Arduino Due. This should be obvious.
- A canbus shield. You can make it or you can wait... sorry...
- Arduino IDE 1.5.2 or newer
- due_can library - There is a repo for this under Collin80
- due_rtc library - I have a repo for this too
- due_wire library - once again, in my repos
- DueTimer library - and again

All libraries belong in (from your root Arduino IDE folder) -> /hardware/arduino/sam/libraries. You will need to
remove -master or any other postfixes. Your library folders should be named as above. Below is what
the folder contents look like on my machine (you don't need all of these libraries, just the ones above)

Audio (not needed)
due_can (needed)
due_rtc (needed)
due_wire (needed)
DueTimer (needed)
Ethernet (not needed)
Scheduler (not needed)
Servo (not needed)
SPI (not needed)
USBHost (not needed)
WiFi (not needed)
Wire (not needed)

The canbus is supposed to be terminated on both ends of the bus. If you are testing with a DMOC 
and Due then you've got two devices, each on opposing ends of the bus. So, both really should be
terminated but for really short canbus lines you will probably get away with terminating just one side.
If you are using a custom board then add a terminating resistor. If you are using the new prototype shield
then it should already be terminated. The DMOC can be terminated by soldering a 120 ohm resistor 
between the canbus lines. I did this on my DMOC and hid the resistor inside the plug shroud. 

This software is MIT licensed:

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

