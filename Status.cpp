/*
 * Status.cpp
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

#include "Status.h"

// initialize the static system state variable to "unknown"
Status::SystemState Status::systemState = Status::unknown;

/*
 * Retrieve the current system state.
 */
Status::SystemState Status::getSystemState() {
    return systemState;
}

/*
 * Set a new system state. The new system state is validated if the
 * transition is allowed from the old state. If an invalid transition is
 * attempted, the new state will be 'error'.
 */
Status::SystemState Status::setSystemState(SystemState newSystemState) {
    if (newSystemState == error) {
        systemState = error;
    } else {
        switch (systemState) {
        case unknown:
            if (newSystemState == init) {
                systemState = init;
            }
            break;
        case init:
            if (newSystemState == preCharge || newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case preCharge:
            if (newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case ready:
            if (newSystemState == running) {
                systemState = newSystemState;
            }
            break;
        case running:
            if (newSystemState == ready) {
                systemState = newSystemState;
            }
            break;
        case error:
            if (newSystemState == init) {
                systemState = newSystemState;
            }
            break;
        }
    }
    if (systemState == newSystemState) {
        Logger::info("switched to system state '%s'", systemStateToStr(systemState));
    } else {
        Logger::error("switching from system state '%s' to '%s' is not allowed", systemStateToStr(systemState), systemStateToStr(newSystemState));
        systemState = error;
    }
    return systemState;
}

/*
 * Convert the current state into a string.
 */
char *Status::systemStateToStr(SystemState state) {
    switch(state) {
    case unknown:
        return "unknown";
    case init:
        return "init";
    case preCharge:
        return "pre-charge";
    case ready:
        return "ready";
    case running:
        return "running";
    case error:
        return "error";
    }
    Logger::error("the system state is invalid, contact your support!");
    return "invalid";
}

/*
 * Calculate the bit field 1 from the respective flags for faster transmission
 * to the web site.
 */
uint32_t Status::getBitField1() {
    uint32_t bitfield = 0;

    //TODO: implement calculation logic

    return bitfield;
}

/*
 * Calculate the bit field 2 from the respective flags for faster transmission
 * to the web site.
 */
uint32_t Status::getBitField2() {
    uint32_t bitfield = 0;

    //TODO: implement calculation logic

    return bitfield;
}

/*
 * Calculate the bit field 3 from the respective flags for faster transmission
 * to the web site.
 */
uint32_t Status::getBitField3() {
    uint32_t bitfield = 0;

    //TODO: implement calculation logic

    return bitfield;
}
