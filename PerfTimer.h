/*
 * PerfTimer.h
 *
 * Provides the ability to track performance of a code section in terms of runtime.
 *
Copyright (c) 2014 Collin Kidder, Michael Neuweiler, Charles Galpin

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


#ifndef PERF_TIME_H_
#define PERF_TIME_H_

#include <Arduino.h>
#include "config.h"

class PerfTimer {
public:
	PerfTimer();
	void start();
	void stop();
	uint32_t getMin();
	uint32_t getMax();
	uint32_t getAvg();
	void condenseAvg();
	void reset();
	void printValues();
protected:
private:
	uint32_t timeMin; //the lowest time we've seen
	uint32_t timeMax;  //the highest time we've seen
	uint32_t timeAccum; //accumulation of all the values we've stored
	uint32_t accumVals; //total # of values accumulated so far
	uint32_t startTime;
	uint32_t endTime;
};

#endif