How to get the Eclipse IDE running with GEVCU:

* Install 32-bit or 64-bit version of Java JRE/JDK and "Eclipse IDE for C/C++ Developers Eclipse IDE for C/C++ Developers" (must match the arduino installation)
//* edit eclipse.ini and add the following line after the one that says "--vmargs":
//-Dgnu.io.rxtx.SerialPorts=/dev/ttyACM0:/dev/ttyACM1:/dev/ttyACM2:/dev/ttyACM3:/dev/ttyUSB0::/dev/ttyUSB1::/dev/ttyUSB2::/dev/ttyUSB3::/dev/ttyUSB4 
* Start eclipse with a new workspace and 32-bit or 64-bit JDK respectively ("eclipse -vm <path to jre>/bin")
* Install Arduino plug-in from http://www.baeyens.it/eclipse/V2 (Important: un-check "Group items by category" to see the plugin)
* Restart eclipse if asked
* Window -> Preferences -> C/C++ -> File Types -> New -> Pattern: *.ino  Type: C++ Source File
* Window -> Preferences -> Arduino: verify the Arduino IDE path (must point to 1.5.2) and
  the Private Library path (where you store additional libs), don't check the "Disable RTXT" checkbox
//* Copy the file ".project.copy" and rename the copy to ".project" (.project is in .gitignore which will prevent any upstream changes due to absolute paths in it)
* File -> Import... -> General -> Existing Projects into Workspace -> enter GEVCU source directory as root directory
* Right click the imported project -> Properties -> Arduino -> Change board and port (if necessary) -> OK
  This will cause the absolute paths to be corrected in the .project file
* Select the project and click on "Verify". The project should compile.


//Note 1: Creating a new Arduino Project does not work yet with this plugin in Eclipse Kelper.
//        You must import the existing project or stick with Eclipse Juno

//Note 2: We're currently working together with the author of the plug-in to remove the absolute paths in the
//        .project file and make the above installation much easier.

Note 3: In case you want to re-compile the entire souce tree, just delete the entire "Release" directory.
        It will be recreated automatically.
        
//Note 4: To upload a project under Linux, currently a manual erase of the flash and reset is required before
//        uploading the project. Looking for a solution with the developer
        
//Note 5: Linux: If the kernel assigns something like ttyACM0 for the Arduino board, the device can be entered
//        manually in the project properties. But the integrated serial monitor in eclipse doesn't work yet.
//        Looking for a solution with developer. As a workaround use a terminal program like GtkTerm.

Note 6: If the editor shows some errors in the code which should not be or if code completion doesn't
        complete as desired, right click the project -> Index -> Rebuild.