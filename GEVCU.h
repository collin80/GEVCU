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
#include "Throttle.h"
#include "CanThrottle.h"
#include "PotThrottle.h"
#include "Device.h"
#include "MotorController.h"
#include "DmocMotorController.h"
#include "Heartbeat.h"
#include "sys_io.h"
#include "CanHandler.h"
#include "MemCache.h"
#include "ThrottleDetector.h"


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
