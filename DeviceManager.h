/*
 * DeviceManager.h
 *
Copyright (c) 2013 Collin Kidder, Michael Neuweiler, Charles Galpin

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
*/

#ifndef DEVICEMGR_H_
#define DEVICEMGR_H_

#include "config.h"
#include "Throttle.h"
#include "MotorController.h"
#include "Charger.h"
#include "DcDcConverter.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"

class Device;
class MotorController; // cyclic reference between MotorController and DeviceManager

class DeviceManager
{
public:
    DeviceManager();
    void addDevice(Device *device);
    void removeDevice(Device *device);
    bool sendMessage(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, void* message);
    void setParameter(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, char *key, char *value);
    void setParameter(DeviceType deviceType, DeviceId deviceId, uint32_t msgType, char *key, uint32_t value);
    Throttle *getAccelerator();
    Throttle *getBrake();
    MotorController *getMotorController();
    Charger *getCharger();
    DcDcConverter *getDcDcConverter();
    Device *getDeviceByID(DeviceId);
    Device *getDeviceByType(DeviceType);
    void printDeviceList();

protected:

private:
    Device *devices[CFG_DEV_MGR_MAX_DEVICES];
    Throttle *throttle;
    Throttle *brake;
    MotorController *motorController;
    DcDcConverter *dcDcConverter;
    Charger *charger;

    int8_t findDevice(Device *device);
};

extern DeviceManager deviceManager;

#endif
