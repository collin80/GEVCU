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
#include "pedal_pot.h"
#include "device.h"
#include "motorctrl.h"
#include "dmoc.h"
#include "timer.h"
#include "sys_io.h"
#include "can_handler.h"
#ifdef __arm__ // Arduino Due specific implementation
#include "mem_cache.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

void printMenu();
void serialEvent();

#endif /* GEVCU_H_ */
