/*
 * device.h
 *
 * Created: 1/20/2013 10:14:51 PM
 *  Author: Collin Kidder
 */

#ifndef DEVICE_H_
#define DEVICE_H_

#include <Arduino.h>
#include "config.h"
#include "eeprom_layout.h"
#include "CanHandler.h"
#include "PrefHandler.h"

class Device {
public:
	enum DeviceType {
		DEVICE_ANY,
		DEVICE_MOTORCTRL,
		DEVICE_BMS,
		DEVICE_CHARGER,
		DEVICE_DISPLAY,
		DEVICE_THROTTLE,
		DEVICE_MISC,
		DEVICE_NONE
	};
	enum DeviceId { //unique device ID for every piece of hardware possible
		DMOC645 = 0x1000,
		BRUSACHARGE = 0x1010,
		TCCHCHARGE = 0x1020,
		POTACCELPEDAL = 0x1030,
		INVALID = 0xFFFF
	};

protected:
	CanHandler *canHandler;
	PrefHandler *prefs;

public:
	Device();

	virtual void handleFrame(CANFrame& frame);
	virtual void handleTick();
	virtual void handleMessage(uint32_t msgType, void* message);
	virtual void setupDevice();
	virtual DeviceType getDeviceType();
	virtual DeviceId getDeviceID();
	Device(CanHandler *canbus);
};

#endif /* DEVICE_H_ */
