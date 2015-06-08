GEVCU
=====

Generalized Electric Vehicle Control Unit

Our website can be found at : http://www.gevcu.org

A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

The project now builds in the Arduino IDE. So, use it to compile, send the firmware to the Arduino, and monitor serial. It all works very nicely.

The master branch of this project is now switched to support the Due. The master branch is sparsely updated and only when the source tree is in stable shape.

The older Macchina code has been moved to its own branch. This code is now *VERY* old but should work to control a DMOC645.

ArduinoDue branch is more experimental than the master branch and includes the work of Michael Neuweiler.

The WIP branch is sync'd to EVTV's official changes and as such could be considered as a testing ground for the official source code distribution.

You will need the following to have any hope of compiling and running the firmware:
- A GEVCU board. Versions from 2 and up are supported.
- Arduino IDE 1.5.4 - Do not use newer versions of the IDE
- due_can library - There is a repo for this under Collin80
- due_rtc library - Also under Collin80
- due_wire library - once again
- DueTimer library - and again

All libraries belong in %USERPROFILE%\Documents\Arduino\libraries (Windows) or ~/Arduino/libraries (Linux/Mac).
You will need to remove -master or any other postfixes. Your library folders should be named as above.

The canbus is supposed to be terminated on both ends of the bus. If you are testing with a DMOC and GEVCU then you've got two devices, each on opposing ends of the bus. So, both really should be terminated but for really short canbus lines you will probably get away with terminating just one side.

If you are using a custom board then add a terminating resistor. 

If you are using the new prototype shield then it should already be terminated. The DMOC can be terminated by soldering a 120 ohm resistor between the canbus lines. I did this on my DMOC and hid the resistor inside the plug shroud. 

This software is MIT licensed:

Copyright (c) 2014 Collin Kidder, Michael Neuweiler, Charles Galpin, Jack Rickard

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

