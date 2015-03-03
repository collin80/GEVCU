/*
 * ConfigObject.h - Forms the basis of a configuration object that can access the configuration of anything that stores data in EEPROM
   overloads the () operator to do this.
 *
Copyright (c) 2015 Collin Kidder

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

#ifndef CONF_OBJ_H_
#define CONF_OBJ_H_

#include <Arduino.h>
#include "config.h"
#include "eeprom_layout.h"
#include "MemCache.h"
#include "DeviceTypes.h"
#include "Logger.h"

extern MemCache *memCache;

class ConfigObject
{
public:
	int &operator()(int device, int location);

private:
	int getEELocation(int deviceid);
};

#endif