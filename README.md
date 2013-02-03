GEVCU
=====

Generalized Electric Vehicle Control Unit

A project to create a fairly Arduino compatible ECU firmware
to interface with various electric vehicle hardware over canbus
(and possibly other comm channels)

The blessed/supported build environment is Code Blocks Arduino Edition 
version 1/31/2013 which can be downloaded at this link: 
http://sourceforge.net/projects/arduinodev/files/CodeBlocks-Arduino-20130131.7z/download
There is also a linux version of it. I haven't tested that yet but I've got 
no reason to think it won't work. If you use a different version of C:B Arduino it 
might work but I can't promise it. If/when I change versions I'll post about it.

The arduino support files are part of the GEVCU project so that I can control the 
version of those files and keep builds working but you do need the drivers from 
the arduino 1.0.3 download. If you don't have the Arduino IDE you can either 
download it or snag a copy of the drivers I archived at: http://www.kkmfg.com/drivers.zip

Once you've downloaded everything and gotten the Macchina to associate to a com port 
you'll need to update the project to let it know the com port to use. Once you do this 
it will attempt to automatically send the compiled firmware to your Macchina every time 
you build the project. So, find the com port associated with the Macchina in the Device Manager 
Start -> Control Panel -> Hardware and Sound -> Device Manager. 

Once you've figured out the com port you set it in the project like so (with the project loaded): 
right click on GEVCU project -> Properties -> Project's Build Options -> Custom Variables -> set UPLOAD_PORT 
to the proper port.

Lastly, the macchina comes, by default, unterminated. The canbus is supposed to be terminated 
on both ends of the bus. If you are testing with a DMOC and Macchina then you've got two devices, 
each on opposing ends of the bus. So, both really should be terminated but for really short canbus 
lines you will probably get away with terminating just one side. The macchina can be reasonably 
easily terminated by soldering jumpers SJ9 and SJ10. These two are close to each other on the 
bottom of the board, somewhat near the middle. The DMOC can be terminated by soldering a 
120 ohm resistor between the canbus lines. I did this on my DMOC and hid the resistor inside the plug shroud. 