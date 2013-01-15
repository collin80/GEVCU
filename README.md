GEVCU
=====

Generalized Electric Vehicle Control Unit

A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

AtmelStudio6 was used for development. All needed files are otherwise 
included in this project:

The avrdude directory contains a working copy of AVRDude for sending
firmware to the Macchina board via USB interface

The arduino_core directory contains a slightly modified copy of the arduino
core library files (ver 1.0.3) The only changes were to prevent compile errors on the
newer GCC/G++ that AS6 uses (compared to Arduino 1.0.3)


If you really are using AtmelStudio 6 then you'll want to set up serial
programming. Under "Tools" select "External Tools". Now, set up the external
tool as so:
Title: Serial Programming
Command: $(ProjectDir)\..\avrdude\avrdude.exe
Arguments: -C$(ProjectDir)\..\avrdude\avrdude.conf -patmega2560 -cwiring -PCOM7 -b115200 -Uflash:w:"$(ProjectDir)Release\$(ItemFileName).hex":i
And select "Use output window"
From now on you can go to Tools->Serial Programming to program the Macchina board
You must currently have the GEVCU.cpp file selected in order to do this.

Right now this is all in a state of very early testing. Who knows what'll
happen if you run it. Your dog and/or wife might leave you. Boiling rain may
fall from the Heavens. You might get open sores and writhe in agony.
It might not work.

Eventually alternative build systems should be supported. At the least there
is no reason a codeblocks project can't be added and/or a generic makefile
for use with the avr-gcc toolchain on linux/cygwin