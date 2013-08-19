How to get the Eclipse IDE running with GEVCU:

* Install 32-bit version of "Eclipse IDE for C/C++ Developers Eclipse IDE for C/C++ Developers"
* Start eclipse with a new workspace
* Install Arduino plug-in from http://www.baeyens.it/eclipse/V2 (Important: un-check "Group items by category" to see the plugin)
* Restart eclipse if asked
* Window -> Preferences -> C/C++ -> File Types -> New -> Pattern: *.ino  Type: C++ Source File
* Window -> Preferences -> Arduino: verify the Arduino IDE path (must point to 1.5.2) and
  the Private Library path (where you store additional libs), don't check the "Disable RTXT" checkbox
* Copy the file ".project.copy" and rename the copy to ".project" (.project is in .gitignore which will prevent any upstream changes due to absolute paths in it)
* File -> Import... -> General -> Existing Projects into Workspace -> root directory = GEVCU source directory
* Right click the imported project -> Properties -> Arduino -> Change board and port (if necessary) -> OK
  This will cause the absolute paths to be corrected in the .project file
* Exit eclipse now !! This will write the necessary changes to .project file (absolute paths) 
* Start eclipse, select the project and c


Note: Creating a new Arduino Project does not work yet with this plugin in Eclipse Kelper.
      You must import the existing project or stick with Eclipse Juno

Note 2: We're currently working together with the author of the plug-in to remove the absolute paths in the
        .project file and make the above installation much easier.
