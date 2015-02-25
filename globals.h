#include "CanHandler.h"
#include "Device.h"
#include "DeviceManager.h"
#include "Throttle.h"
#include "MotorController.h"
#include "BatteryManager.h"

#ifndef GLOBAL_H_
#define GLOBAL_H_
//the externs are here so that every module and class has easy access to these cheater objects
//which makes the code simpler and more straight forward.
extern CanHandler *canHandlerEV;
extern CanHandler *canHandlerCar;
extern Device *wifi;
extern Device *bluetooth;
extern MotorController *motorcontroller;
extern BatteryManager *bms;
extern Throttle *throttle;
extern Throttle *brake;
extern DeviceManager *deviceManager;
#endif