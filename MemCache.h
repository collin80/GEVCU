/*
 * MemCache.h
 *
 * Handles caching of EEPROM pages to RAM
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

#ifndef MEM_CACHE_H_
#define MEM_CACHE_H_

#include <Arduino.h>
#include "config.h"
#include "TickHandler.h"
#include <due_wire.h>

//Total # of allowable pages to cache. Limits RAM usage
#define NUM_CACHED_PAGES   16

//maximum allowable age of a cache
#define MAX_AGE  128

/* # of system ticks per aging cycle. There are 128 aging levels total so
// multiply 128 by this aging period and multiple that by system tick duration
// to determine how long it will take for a page to age out fully and get written
// if dirty. For instance, 128 levels * 500 aging period * 10ms (100Hz tick) = 640 seconds
// EEPROM handles about 1 million write cycles. So, a flush time of 100 seconds means that
// continuous writing would last 100M seconds which is 3.17 years
*/
#define AGING_PERIOD       200

class MemCache: public TickObserver {
  public:
  void setup();
  void handleTick();
  void FlushSinglePage();
  void FlushAllPages();
  void FlushPage(uint8_t page);
  void InvalidatePage(uint8_t page);
  void InvalidateAddress(uint32_t address);
  void InvalidateAll();
  void AgeFullyPage(uint8_t page);
  void AgeFullyAddress(uint32_t address);
  
  boolean Write(uint32_t address, uint8_t valu);
  boolean Write(uint32_t address, uint16_t valu);
  boolean Write(uint32_t address, uint32_t valu);
  boolean Write(uint32_t address, void* data, uint16_t len);
  
  //It's sort of weird to make the read function take a reference but it allows for overloading
  boolean Read(uint32_t address, uint8_t* valu);
  boolean Read(uint32_t address, uint16_t* valu);
  boolean Read(uint32_t address, uint32_t* valu);
  boolean Read(uint32_t address, void* data, uint16_t len);
  
  MemCache();
  
  private:
  typedef struct {
    uint8_t data[256];
    uint32_t address; //address of start of page
    uint8_t age; //
    boolean dirty;
  } PageCache;

  PageCache pages[NUM_CACHED_PAGES];
  boolean isWriting();
  uint8_t cache_hit(uint32_t address);
  void cache_age();
  uint8_t cache_findpage();
  uint8_t cache_readpage(uint32_t addr);
  boolean cache_writepage(uint8_t page);
  uint8_t agingTimer;
};

#endif /* MEM_CACHE_H_ */
