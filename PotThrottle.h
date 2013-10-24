/*
 * PotThrottle.h
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

#ifndef PEDAL_POT_H_
#define PEDAL_POT_H_

#include <Arduino.h>
#include "config.h"
#include "Throttle.h"
#include "sys_io.h"
#include "TickHandler.h"
#include "Logger.h"

#define THROTTLE_INPUT_BRAKELIGHT  2

/*
 * The extended configuration class with additional parameters for PotThrottle
 */
class PotThrottleConfiguration: public ThrottleConfiguration {
public:
	/*
	 * Allows subclasses to have sub types for their pedal type
	 * 0 - unknown type (prefs will return 0 if never set)
	 * 1 - standard linear potentiometer (low-high). If 2 pots, both are low-high and the 2nd mirrors the 1st.
	 * 2 - inverse potentiometer (high-low). If 2 pots, then 1st is low-high and 2nd is high-low)
	 */
	uint8_t throttleSubType;
	uint16_t minimumLevel1, maximumLevel1, minimumLevel2, maximumLevel2; // values for when the pedal is at its min and max for each input
	uint8_t numberPotMeters; // the number of potentiometers to be used. Should support three as well since some pedals really do have that many
};

class PotThrottle: public Throttle {
public:
	PotThrottle(uint8_t throttle1, uint8_t throttle2);
	void setup();
	void handleTick();
	DeviceId getId();
	RawSignalData *acquireRawSignal();

	void loadConfiguration();
	void saveConfiguration();

protected:
	bool validateSignal(RawSignalData *);
	uint16_t calculatePedalPosition(RawSignalData *);

private:
	uint8_t throttle1AdcPin, throttle2AdcPin; //which ADC pin each are on
	RawSignalData rawSignal;
};

#endif /* POT_THROTTLE_H_ */
