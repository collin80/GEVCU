/*
The device manager keeps a list of all devices which are installed into the system.
Anything that needs either a tick handler, a canbus handler, or to communicate to other
devices on the system must be registered with the manager. The manager then handles
making sure the tick handler is set up (if needed), the canbus handler is set up (if need),
and that a device can send information to other devices by either type (BMS, motor ctrl, etc)
or by device ID. 

The device class itself has the handlers defined so the tick and canbus handling code
need only call these existing functions but the manager interface needs to
expose a way to register them with the system.
*/

#include "DeviceManager.h"

//Add the specified device to the list of registered devices
void DeviceManager::addDevice(Device *newdevice)
{
}

/*Add a new tick handler to the specified device. It should
//technically be possible to register for multiple intervals
//and be called for all of them but support for that is not
//immediately necessary
*/
void DeviceManager::addTickHandler(Device *newdevice, uint32_t freq)
{
}

/*Add a new filter that sends frames through to the device. There definitely has
to be support for multiple filters per device right from the beginning.
Mask, id, ext form the filter. canbus sets whether to attach to 
CAN0 or CAN1.
*/
void DeviceManager::addCANHandler(Device *newdevice, uint32_t mask, uint32_t id, bool ext, uint8_t canbus)
{
}

/*
Send an inter-device message. Devtype has to be filled out but could be DEVICE_ANY.
If devId is anything other than INVALID (0xFFFF) then the message will be targetted to only
one device. Otherwise it will broadcast to any device that matches the device type (or all
devices in the case of DEVICE_ANY).
DeviceManager.h has a list of standard message types but you're allowed to send
whatever you want. The standard message types are to enforce standard messages for easy
intercommunication. 
*/
void DeviceManager::sendMessage(Device::DeviceType devType, Device::DeviceId devId, uint32_t msgType, void* message)
{
}

