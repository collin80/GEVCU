/*
 * tick_handler.cpp
 *
 *
 *  Created: 7/11/2013
 *   Author: Michael Neuweiler
 */

#include "TickHandler.h"

TickHandler::TickHandler() {
	running = false;
    for (int i = 0; i < CFG_MAX_TICK_DEVICES; i++) {
        tickDevice[i].device = NULL;
    }
}

void TickHandler::registerDevice(DEVICE* device, uint32_t tickInterval) {
    uint32_t frequency = 1000000ul / tickInterval;
    int timer = findTimer(NULL);
    tickDevice[timer].device = device;
    switch (timer) {
    case 1:
        startTimer1(frequency, timer1Interrupt);
        break;
    case 2:
        startTimer2(frequency, timer2Interrupt);
        break;
    case 3:
        startTimer3(frequency, timer3Interrupt);
        break;
    case 4:
        startTimer4(frequency, timer4Interrupt);
        break;
    case 5:
        startTimer5(frequency, timer5Interrupt);
        break;
    case 6:
        startTimer6(frequency, timer6Interrupt);
        break;
    case 7:
        startTimer7(frequency, timer7Interrupt);
        break;
    case 8:
        startTimer8(frequency, timer8Interrupt);
        break;
    case 9:
        startTimer9(frequency, timer9Interrupt);
        break;
    case 0:
        Logger::error("Could not register tick device with interval %d", tickInterval);
    }
}

void TickHandler::unregisterDevice(DEVICE* device) {
    int timer = findTimer(device);
    switch (timer) {
    case 1:
        stopTimer1();
        break;
    case 2:
        stopTimer2();
        break;
    case 3:
        stopTimer3();
        break;
    case 4:
        stopTimer4();
        break;
    case 5:
        stopTimer5();
        break;
    case 6:
        stopTimer6();
        break;
    case 7:
        stopTimer7();
        break;
    case 8:
        stopTimer8();
        break;
    case 9:
        stopTimer9();
        break;
    }
    tickDevice[timer].device = NULL;
}

int TickHandler::findTimer(DEVICE *device) {
    for (int i = 0; i < CFG_MAX_TICK_DEVICES; i++) {
        if (tickDevice[i].device == device)
            return i + 1;
    }
    return 0;
}

void TickHandler::handleInterrupt(int device) {
    tickDevice[device - 1].device->handleTick();
}

volatile void timer1Interrupt() {
    TickHandler::handleInterrupt(1);
}
volatile void timer2Int() {
    TickHandler::handleInterrupt(2);
}
volatile void timer3Int() {
    TickHandler::handleInterrupt(3);
}
volatile void timer4Int() {
    TickHandler::handleInterrupt(4);
}
volatile void timer5Int() {
    TickHandler::handleInterrupt(5);
}
volatile void timer6Int() {
    TickHandler::handleInterrupt(6);
}
volatile void timer7Int() {
    TickHandler::handleInterrupt(7);
}
volatile void timer8Int() {
    TickHandler::handleInterrupt(8);
}
volatile void timer9Int() {
    TickHandler::handleInterrupt(9);
}