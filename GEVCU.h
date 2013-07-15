/*
 * GEVCU.h
 *
 *  Created: 4/28/2013
 *   Author: Michael Neuweiler
 */

#ifndef GEVCU_H_
#define GEVCU_H_

#include <Arduino.h>
#include "config.h"
#include "throttle.h"
#include "PotThrottle.h"
#include "device.h"
#include "MotorController.h"
#include "DmocMotorController.h"
#include "HeartbeatDevice.h"
#include "sys_io.h"
#include "CanHandler.h"
#include "MemCache.h"


#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

#define SYSTEM_DUE        20

void printMenu();
void serialEvent();

#endif /* GEVCU_H_ */
