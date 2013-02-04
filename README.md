GEVCU
=====

Generalized Electric Vehicle Control Unit

A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

The project now builds in the Arduino IDE. So, use it to compile, send the firmware
to the macchina, and monitor serial. It all works very nicely.

You must manually download the newest copy of the MCP2515 library and place it
into the libraries directory of your Arduino installation.
Git:https://github.com/collin80/mcp2515.git
Zip:https://github.com/collin80/mcp2515/archive/master.zip

Lastly, the macchina comes, by default, unterminated. The canbus is supposed to be terminated 
on both ends of the bus. If you are testing with a DMOC and Macchina then you've got two devices, 
each on opposing ends of the bus. So, both really should be terminated but for really short canbus 
lines you will probably get away with terminating just one side. The macchina can be reasonably 
easily terminated by soldering jumpers SJ9 and SJ10. These two are close to each other on the 
bottom of the board, somewhat near the middle. The DMOC can be terminated by soldering a 
120 ohm resistor between the canbus lines. I did this on my DMOC and hid the resistor inside the plug shroud. 