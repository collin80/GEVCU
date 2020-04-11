GEVCU
=====

Generalized Electric Vehicle Control Unit

A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

The project builds in the Arduino IDE. So, use it to compile, send the firmware to the Arduino, and monitor serial.
It is suggested though to use the Eclipse based Arduino Plugin by Sloeber which provides far superior capabilities.

The master branch of this project provides a stable code base. The main development happens on the development branch or feature branches if necessary.
If the code on development branch is stable (end tested in practice by the developers), it is merged into the master branch.

You will need the following to have any hope of compiling and running the firmware:
- A GEVCU board. (Versions from 2 upward are supported)
- THe latest Arduino IDE (or preferably Eclipse with ArduinoPlugin by Sloeber)
- due_can library by Collin Kidder
- due_wire library by Collin Kidder
- DueTimer library by Collin Kidder
- PID library by Brett Beauregard

All libraries belong in %USERPROFILE%\Documents\Arduino\libraries (Windows) or ~/Arduino/libraries (Linux/Mac).
You will need to remove -master or any other postfixes. Your library folders should be named as above.

If you use a GEVCU 2.x, please change the pin assignments for CFG_EEPROM_WRITE_PROTECT and CFG_WIFI_RESET in config.h according to comment.      

This software is MIT licensed:

Copyright (c) 2014-2020 Collin Kidder, Michael Neuweiler, Charles Galpin, Jack Rickard

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
