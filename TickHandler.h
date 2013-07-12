#ifndef TICKHANDLER_H_
#define TICKHANDLER_H_

//By: Michael Neuweiler

#include "device.h"
#include "config.h"

class TickHandler {

    typedef struct {
        DEVICE *device; // pointer to device which should be ticked
        uint32_t tickInterval; // interval at which the device should be ticked (in microseconds)
        uint32_t lastTick; // time stamp at which the device was last ticked
    } TickDevice;

private:
    static bool running;
    static TickDevice tickDevice[CFG_MAX_TICK_DEVICES]; // array which holds the registered TickDevices
    void tickDevices(); // tick all devices which are due
	int findTimer(DEVICE *device);
	void handleInterrupt(int device);

protected:

public:
    TickHandler();
    static void registerDevice(DEVICE *, uint32_t);
    static void unregisterDevice(DEVICE *);
    static void start();
    static void stop();
};

#endif