How to get the Eclipse IDE running with GEVCU:

* Install 32-bit or 64-bit version of Java JRE/JDK and "Eclipse IDE for C/C++ Developers Eclipse IDE for C/C++ Developers" (do not mix 32/64bit versions)
* Start eclipse with a new workspace and 32-bit or 64-bit JRE/JDK respectively ("eclipse -vm <path to jre>/bin")
* Install Arduino plug-in from http://www.baeyens.it/eclipse/V2 (Important: un-check "Group items by category" to see the plugin)
* Restart eclipse if asked
* Window -> Preferences -> C/C++ -> File Types -> New -> Pattern: *.ino  Type: C++ Source File
* Window -> Preferences -> Arduino: verify the Arduino IDE path (must point to 1.5.5) and
  the Private Library path (where you store additional libs), don't check the "Disable RTXT" checkbox
* Copy the file ".project.copy" and rename the copy to ".project" (.project is in .gitignore which will prevent any upstream changes due to absolute paths in it)
* File -> Import... -> General -> Existing Projects into Workspace -> enter GEVCU source directory as root directory
* Right click the imported project -> Properties -> Arduino -> Change board and port (if necessary) -> OK
  This will cause the absolute paths to be corrected in the .project file
* Select the project and click on "Verify". The project should compile.

Note 1: In case you want to re-compile the entire souce tree, just delete the entire "Release" directory.
        It will be recreated automatically.
        
Note 2: If the editor shows some errors in the code which should not be or if code completion doesn't
        complete as desired, right click the project -> Index -> Rebuild and open the file in the browser.
