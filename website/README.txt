The GEVCU uses the Mini Socket iWifi module from ConnectOne. Documentation can be found at

http://www.connectone.com/?page_id=217

To update the website on the iChip, you have to use their packing tool which is part of their iChipConfig utility and only runs on Windows XP.  If you use Windows 7 you can use Windows XP mode:

http://windows.microsoft.com/en-us/windows7/install-and-use-windows-xp-mode-in-windows-7

Download and install the following:

http://www.connectone.com/wp-content/uploads/2012/06/iChipConfigExternal_2.4.95_setup7.zip

Then to update the website after changes,

1. Launch iChipConfig, click through warnings about not having the com port and limited functionality
2. Choose the "Site Pack" tool
3. Browse to the website/src dir, select the index.htm as default
4. Select "C02128" as the platform and click "Pack". At this point the parameters should show.
5. Select all the parameters, enter 20 for the max length value to fill and click "Fill"
6. Click "Save" and point it to website.img

At this point you upload the image to the GEVCU using http://192.168.3.10/ichip and power cycle the GEVCU to have it activated.
