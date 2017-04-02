/*
 * PotThrottle.cpp
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

#include "PotThrottle.h"

/*
 * Constructor
 */
PotThrottle::PotThrottle() : Throttle()
{
    prefsHandler = new PrefHandler(POTACCELPEDAL);
    commonName = "Potentiometer (analog) accelerator";
}

/*
 * Setup the device.
 */
void PotThrottle::setup()
{
    Throttle::setup(); //call base class
    ready = true;

    //set digital ports to inputs and pull them up all inputs currently active low
    //pinMode(THROTTLE_INPUT_BRAKELIGHT, INPUT_PULLUP); //Brake light switch

    tickHandler.attach(this, CFG_TICK_INTERVAL_POT_THROTTLE);
}

/*
 * Process a timer event.
 */
void PotThrottle::handleTick()
{
    Throttle::handleTick(); // Call parent which controls the workflow
}

/*
 * Retrieve raw input signals from the throttle hardware.
 */
RawSignalData *PotThrottle::acquireRawSignal()
{
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
    systemIO.ADCPoll();

    rawSignal.input1 = systemIO.getAnalogIn(config->AdcPin1);
    rawSignal.input2 = systemIO.getAnalogIn(config->AdcPin2);
    return &rawSignal;
}

/*
 * Perform sanity check on the ADC input values. The values are normalized (without constraining them)
 * and the checks are performed on a 0-1000 scale with a percentage tolerance
 */
bool PotThrottle::validateSignal(RawSignalData *rawSignal)
{
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
    int32_t calcThrottle1, calcThrottle2;

    calcThrottle1 = normalizeInput(rawSignal->input1, config->minimumLevel, config->maximumLevel);

    if (config->numberPotMeters == 1 && config->throttleSubType == 2) { // inverted
        calcThrottle1 = 1000 - calcThrottle1;
    }

    if (calcThrottle1 > (1000 + CFG_THROTTLE_TOLERANCE)) {
        if (throttleStatus == OK) {
            Logger::error(this, "ERR_HIGH_T1: throttle 1 value out of range: %ld", calcThrottle1);
        }
		faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_HIGH_A, true);

        throttleStatus = ERR_HIGH_T1;
        return false;
    }
	else
	{
		faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_HIGH_A);
	}

    if (calcThrottle1 < (0 - CFG_THROTTLE_TOLERANCE)) {
        if (throttleStatus == OK) {
            Logger::error(this, "ERR_LOW_T1: throttle 1 value out of range: %ld ", calcThrottle1);
        }
		faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_LOW_A, true);

        throttleStatus = ERR_LOW_T1;
        return false;
    }
	else
	{
		faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_LOW_A);
	}

    if (config->numberPotMeters > 1) {
        calcThrottle2 = normalizeInput(rawSignal->input2, config->minimumLevel2, config->maximumLevel2);

        if (calcThrottle2 > (1000 + CFG_THROTTLE_TOLERANCE)) {
            if (throttleStatus == OK) {
                Logger::error(this, "ERR_HIGH_T2: throttle 2 value out of range: %ld", calcThrottle2);
            }
			faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_HIGH_B, true);

            throttleStatus = ERR_HIGH_T2;
            return false;
        }
		else
		{
			faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_HIGH_B);
		}

        if (calcThrottle2 < (0 - CFG_THROTTLE_TOLERANCE)) {
            if (throttleStatus == OK) {
                Logger::error(this, "ERR_LOW_T2: throttle 2 value out of range: %ld", calcThrottle2);
            }
			faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_LOW_B);

            throttleStatus = ERR_LOW_T2;
            return false;
        }
		else
		{
			faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_LOW_B);
		}

        if (config->throttleSubType == 2) {
            // inverted throttle 2 means the sum of the two throttles should be 1000
            if (abs(1000 - calcThrottle1 - calcThrottle2) > CFG_THROTTLE_MAX_ERROR) {
                if (throttleStatus == OK)
                    Logger::error(this, "Sum of throttle 1 (%ld) and throttle 2 (%ld) exceeds max variance from 1000 (%ld)",
                                  calcThrottle1, calcThrottle2, CFG_THROTTLE_MAX_ERROR);

                throttleStatus = ERR_MISMATCH;
				faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_MISMATCH_AB, true);
                return false;
            }
			else 
			{
				faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_MISMATCH_AB);
			}
        } else {
            if ((calcThrottle1 - CFG_THROTTLE_MAX_ERROR) > calcThrottle2) {  //then throttle1 is too large compared to 2
                if (throttleStatus == OK) {
                    Logger::error(this, "throttle 1 too high (%ld) compared to 2 (%ld)", calcThrottle1, calcThrottle2);
                }

                throttleStatus = ERR_MISMATCH;
				faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_MISMATCH_AB, true);
                return false;
            }

           else if ((calcThrottle2 - CFG_THROTTLE_MAX_ERROR) > calcThrottle1) {  //then throttle2 is too large compared to 1
                if (throttleStatus == OK) {
                    Logger::error(this, "throttle 2 too high (%ld) compared to 1 (%ld)", calcThrottle2, calcThrottle1);
                }
				faultHandler.raiseFault(POTACCELPEDAL, FAULT_THROTTLE_MISMATCH_AB, true);

                throttleStatus = ERR_MISMATCH;
                return false;
            } else {
				faultHandler.cancelOngoingFault(POTACCELPEDAL, FAULT_THROTTLE_MISMATCH_AB);
			}
        }
    }

    // all checks passed -> throttle is ok
    if (throttleStatus != OK) {
        Logger::info(this, (char *) Constants::normalOperation);
    }

    throttleStatus = OK;
    return true;
}

/*
 * Convert the raw ADC values to a range from 0 to 1000 (per mille) according
 * to the specified range and the type of potentiometer.
 */
uint16_t PotThrottle::calculatePedalPosition(RawSignalData *rawSignal)
{
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();
    uint16_t calcThrottle1, calcThrottle2;

    calcThrottle1 = normalizeInput(rawSignal->input1, config->minimumLevel, config->maximumLevel);

    if (config->numberPotMeters > 1) {
        calcThrottle2 = normalizeInput(rawSignal->input2, config->minimumLevel2, config->maximumLevel2);

        if (config->throttleSubType == 2) { // inverted
            calcThrottle2 = 1000 - calcThrottle2;
        }

        calcThrottle1 = (calcThrottle1 + calcThrottle2) / 2; // now the average of the two
    }

    return calcThrottle1;
}

/*
 * Return the device ID
 */
DeviceId PotThrottle::getId()
{
    return (POTACCELPEDAL);
}

/*
 * Load the device configuration.
 * If possible values are read from EEPROM. If not, reasonable default values
 * are chosen and the configuration is overwritten in the EEPROM.
 */
void PotThrottle::loadConfiguration()
{
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

    if (!config) { // as lowest sub-class make sure we have a config object
        config = new PotThrottleConfiguration();
        setConfiguration(config);
    }

    Throttle::loadConfiguration(); // call parent

#ifdef USE_HARD_CODED

    if (false) {
#else

    if (prefsHandler->checksumValid()) { //checksum is good, read in the values stored in EEPROM
#endif
        prefsHandler->read(EETH_LEVEL_MIN_TWO, &config->minimumLevel2);
        prefsHandler->read(EETH_LEVEL_MAX_TWO, &config->maximumLevel2);
        prefsHandler->read(EETH_NUM_THROTTLES, &config->numberPotMeters);
        prefsHandler->read(EETH_THROTTLE_TYPE, &config->throttleSubType);
        prefsHandler->read(EETH_ADC_1, &config->AdcPin1);
        prefsHandler->read(EETH_ADC_2, &config->AdcPin2);

        // ** This is potentially a condition that is only met if you don't have the EEPROM hardware **
        // If preferences have never been set before, numThrottlePots and throttleSubType
        // will both be zero.  We really should refuse to operate in this condition and force
        // calibration, but for now at least allow calibration to work by setting numThrottlePots = 2
        if (config->numberPotMeters == 0 && config->throttleSubType == 0) {
            Logger::debug(this, "THROTTLE APPEARS TO NEED CALIBRATION/DETECTION - choose 'z' on the serial console menu");
            config->numberPotMeters = 2;
        }
    } else { //checksum invalid. Reinitialize values and store to EEPROM
        config->minimumLevel2 = 0;
        config->maximumLevel2 = 0;
        config->numberPotMeters = 1;
        config->throttleSubType = 1;
        config->AdcPin1 = 0;
        config->AdcPin2 = 1;

        saveConfiguration();
    }

    Logger::info(this, "T2 MIN: %ld, T2 MAX: %ld", config->minimumLevel2, config->maximumLevel2);
    Logger::info(this, "# of pots: %d, subtype: %d", config->numberPotMeters, config->throttleSubType);
}

/*
 * Store the current configuration to EEPROM
 */
void PotThrottle::saveConfiguration()
{
    PotThrottleConfiguration *config = (PotThrottleConfiguration *) getConfiguration();

    Throttle::saveConfiguration(); // call parent

    prefsHandler->write(EETH_LEVEL_MIN_TWO, config->minimumLevel2);
    prefsHandler->write(EETH_LEVEL_MAX_TWO, config->maximumLevel2);
    prefsHandler->write(EETH_NUM_THROTTLES, config->numberPotMeters);
    prefsHandler->write(EETH_THROTTLE_TYPE, config->throttleSubType);
    prefsHandler->write(EETH_ADC_1, config->AdcPin1);
    prefsHandler->write(EETH_ADC_2, config->AdcPin2);
    prefsHandler->saveChecksum();
}
