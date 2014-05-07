/*
 * PerfTimer.cpp
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


#include "PerfTimer.h"
#include "Logger.h"

PerfTimer::PerfTimer() 
{
	reset();
}

void PerfTimer::start()
{
	startTime = micros();
}

void PerfTimer::stop()
{
	int numTicks;

	endTime = micros();

    numTicks = endTime - startTime;

    if (numTicks < timeMin) timeMin = numTicks;
    if (numTicks > timeMax) timeMax = numTicks;

	accumVals++;
	timeAccum += numTicks;

	//Auto condense the average if it starts to get too close to the upper limit
	if (timeAccum > 3500000000ul) condenseAvg();

}

uint32_t PerfTimer::getMin()
{
	return timeMin;
}

uint32_t PerfTimer::getMax()
{
	return timeMax;
}

uint32_t PerfTimer::getAvg()
{
	if (accumVals == 0) return 0;
	return timeAccum / accumVals;
}

void PerfTimer::condenseAvg()
{
	if (accumVals == 0) return;
	timeAccum /= accumVals;
	accumVals = 1;
}

void PerfTimer::reset()
{
	timeMin = 1000000;
	timeMax = 0;
	timeAccum = 0;
	accumVals = 0;
	startTime = 0;
	endTime = 0;
}

void PerfTimer::printValues()
{
	Logger::console("Min/Max/Avg (uS) -> %i/%i/%i", getMin(), getMax(), getAvg());
	Logger::console("Min/Max/Avg (approx cycles) -> %i/%i/%i", getMin() * 84, getMax() * 84, getAvg() * 84);
}
