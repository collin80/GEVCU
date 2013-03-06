/*
 * mem_cache.cpp
 *
 * Created: 3/6/2013
 *  Author: Collin Kidder
 */

#include "Arduino.h"
#include "mem_cache.h"

//this function flushes the first dirty page it finds. It should try to wait until enough time as elapsed since
//a previous page has been written.
void MEMCACHE::FlushSinglePage() 
{
  U8 c;
  for (c=0;c<NUM_CACHED_PAGES;c++) {
    if (pages[c].dirty) {
      cache_writepage(c);
      pages[c].dirty = false;
      pages[c].age = 0; //freshly flushed!
      return;
    }
  }
}

//Flush every dirty page. It will block for 7ms per page so maybe things will be blocked for a long, long time
//DO NOT USE THIS FUNCTION UNLESS YOU CAN ACCEPT THAT!
void MEMCACHE::FlushAllPages()
{
  U8 c;
  for (c = 0; c < NUM_CACHED_PAGES;c++) {
    if (pages[c].dirty) { //found a dirty page so flush it
      cache_writepage(c);
      pages[c].dirty = false;
      delay(7); 
    }
  }
}

void MEMCACHE::FlushPage(uint8_t page) {
  if (pages[page].dirty) {
    cache_writepage(page);
    pages[page].dirty = false;
    pages[page].age = 0; //freshly flushed!
  }	
}

void MEMCACHE::handleTick()
{
  U8 c;
  //  if (AgingTimer > AGING_PERIOD) {
  //    AgingTimer -= AGING_PERIOD;
  cache_age();
  for (c=0;c<NUM_CACHED_PAGES;c++) {
    if ((pages[c].age == MAX_AGE) && (pages[c].dirty)) {
      FlushPage(c);				
      return;
    }
  }
  //  }
}

void MEMCACHE::InvalidatePage(uint8_t page)
{
  if (page > NUM_CACHED_PAGES - 1) return; //invalid page, buddy!
  if (pages[page].dirty) {
    cache_writepage(page);
  }
  pages[page].dirty = false;
  pages[page].address = 0x3FF;
  pages[page].age = 0;
}

void MEMCACHE::InvalidateAddress(uint16_t address)
{
  uint16_t addr;
  uint8_t c;

  addr = address >> 6; //kick it down to the page we're talking about
  c = cache_hit(addr);
  if (c != 0xFF) InvalidatePage(c);
}

void MEMCACHE::InvalidateAll()
{
  uint8_t c;
  for (c=0;c<NUM_CACHED_PAGES;c++) {
    InvalidatePage(c);
  }	
}

void MEMCACHE::AgeFullyPage(uint8_t page)
{
  if (page < NUM_CACHED_PAGES) { //if we did indeed have that page in cache
    pages[page].age = MAX_AGE;
  }
}

void MEMCACHE::AgeFullyAddress(uint16_t address)
{
  U8 thisCache;
  U16 page_addr;

  page_addr = address >> 6; //kick it down to the page we're talking about
  thisCache = cache_hit(page_addr);

  if (thisCache != 0xFF) { //if we did indeed have that page in cache
    pages[thisCache].age = MAX_AGE;
  }
}


boolean MEMCACHE::Write(uint16_t address, uint8_t valu)
{
  uint16_t addr;
  uint8_t c;

  addr = address >> 6; //kick it down to the page we're talking about
  c = cache_hit(addr);
  if (c == 0xFF) 	{
    c = cache_findpage(); //try to free up a page
    if (c != 0xFF) c = cache_readpage(addr); //and populate it with the existing data
  }		
  if (c != 0xFF) {
    pages[c].data[(uint8_t)(address & 0x003F)] = valu;
    pages[c].dirty = true;
    pages[c].address = addr; //set this in case we actually are setting up a new cache page
    return true;
  }
  return false;
}

boolean MEMCACHE::Write(uint16_t address, uint16_t valu)
{
  boolean result;
  result = Write(address, &valu, 2);
  return result;
}

boolean MEMCACHE::Write(uint16_t address, uint32_t valu)
{
  boolean result;
  result = Write(address, &valu, 4);
  return result;
}

boolean MEMCACHE::Write(uint16_t address, void* data, uint16_t len)
{
  uint16_t addr;
  uint8_t c;
  uint16_t count;

  for (count = 0; count < len; count++) {
    addr = (address+count) >> 6; //kick it down to the page we're talking about
    c = cache_hit(addr);
    if (c == 0xFF) {
      c = cache_findpage(); //try to find a page that either isn't loaded or isn't dirty
      if (c != 0xFF) c = cache_readpage(addr); //and populate it with the existing data
    }
    if (c != 0xFF) { //could we find a suitable cache page to write to?
      pages[c].data[(uint8_t)((address+count) & 0x003F)] = *(uint8_t *)(data + count);
      pages[c].dirty = true;
      pages[c].address = addr; //set this in case we actually are setting up a new cache page	
    }
    else break;
  }		

  if (c != 0xFF) return true; //all ok!
  return false;
}

boolean MEMCACHE::Read(uint16_t address, uint8_t* valu)
{
  uint16_t addr;
  uint8_t c;

  addr = address >> 6; //kick it down to the page we're talking about
  c = cache_hit(addr);

  if (c == 0xFF) { //page isn't cached. Search the cache, potentially dump a page and bring this one in
    c = cache_readpage(addr);
  }

  if (c != 0xFF) {
    *valu = pages[c].data[(uint8_t)(address & 0x003F)];
    if (!pages[c].dirty) pages[c].age = 0; //reset age since we just used it
    return true; //all ok!
  }
  else {
    return false; 
  }	
}

boolean MEMCACHE::Read(uint16_t address, uint16_t* valu)
{
  boolean result;
  result = Read(address, valu, 2);
  return result;
}

boolean MEMCACHE::Read(uint16_t address, uint32_t* valu)
{
  boolean result;
  result = Read(address, valu, 4);
  return result;
}

boolean MEMCACHE::Read(uint16_t address, void* data, uint16_t len)
{
  uint16_t addr;
  uint8_t c;
  uint16_t count;

  for (count = 0; count < len; count++) {
    addr = (address + count) >> 6;
    c = cache_hit(addr);
    if (c == 0xFF) { //page isn't cached. Search the cache, potentially dump a page and bring this one in
      c = cache_readpage(addr);
    }		
    if (c != 0xFF) {
      *(uint8_t *)(data + count) = pages[c].data[(uint8_t)((address+count) & 0x003F)];
      if (!pages[c].dirty) pages[c].age = 0; //reset age since we just used it
    }
    else break; //bust the for loop if we run into trouble
  }

  if (c != 0xFF) return true; //all ok!
  return false;
}

MEMCACHE::MEMCACHE()
{
  U8 c;
  for (c = 0; c < NUM_CACHED_PAGES; c++) {
    pages[c].address = 0x3FF; //maximum number. This is way over what our chip will actually support so it signals unused
    pages[c].age = 0;
    pages[c].dirty = false;
  }		
  //WriteTimer = 0;
  //AgingTimer = 0;

}

boolean MEMCACHE::isWriting()
{
  //if (WriteTimer) return true;
  return false;

}

uint8_t MEMCACHE::cache_hit(U16 address)
{
  uint8_t c;
  for (c = 0; c < NUM_CACHED_PAGES; c++) {
    if (pages[c].address == address) {
      return c;
    }			
  }
  return 0xFF;
}

void MEMCACHE::cache_age()
{
  uint8_t c;
  for (c = 0; c < NUM_CACHED_PAGES; c++) {
    if (pages[c].age < MAX_AGE) {
      pages[c].age++;
    }
  }	
}

//try to find an empty page or one that can be removed from cache
uint8_t MEMCACHE::cache_findpage()
{
  uint8_t c;
  uint8_t old_c, old_v;
  for (c=0;c<NUM_CACHED_PAGES;c++) {
    if (pages[c].address == 0x3FF) { //found an empty cache page so populate it and return its number
      pages[c].age = 0;
      pages[c].dirty = false;
      return c;
    }
  }
  //if we got here then there are no free pages so scan to find the oldest one which isn't dirty
  old_c = 0xFF;
  old_v = 0;
  for (c=0;c<NUM_CACHED_PAGES;c++) {
    if (!pages[c].dirty && pages[c].age >= old_v) {
      old_c = c;
      old_v = pages[c].age;
    }		
  }
  if (old_c == 0xFF) { //no pages were not dirty - try to free one up
    FlushSinglePage(); //try to free up a page
    //now try to find the free page (if one was freed)
    old_v = 0;
    for (c=0;c<NUM_CACHED_PAGES;c++) {
      if (!pages[c].dirty && pages[c].age >= old_v) {
        old_c = c;
        old_v = pages[c].age;
      }
    }
    if (old_c == 0xFF) return 0xFF; //if nothing worked then give up
  }		

  //If we got to this point then we have a page to use
  pages[old_c].age = 0;
  pages[old_c].dirty = false;
  pages[old_c].address = 0x3FF; //mark it unused

  return old_c;	
}

uint8_t MEMCACHE::cache_readpage(uint16_t addr)
{
  uint8_t c,d,e;
  uint16_t address = addr << 6;
  uint8_t buffer[3];
  c = cache_findpage();

  if (c != 0xFF) {
    buffer[0] = ((address & 0xFF00) >> 8);
    buffer[1] = (address & 0x00FF);
    Wire.beginTransmission(0b01010000);  
    Wire.write(buffer, 2);
    Wire.endTransmission();
    delayMicroseconds(150); //give TWI some time to send and chip some time to get page

    Wire.requestFrom(0b01010000, 64);
    for (e = 0; e < 64; e++)
    {
      if(Wire.available())    
      { 
        d = Wire.read(); // receive a byte as character
        pages[c].data[e] = d;
      }
    }
    pages[c].address = address;
    pages[c].age = 0;
    pages[c].dirty = false;
  }
  return c;
}

boolean MEMCACHE::cache_writepage(uint8_t page)
{
  U8 d;
  U16 addr;
  uint8_t buffer[66];
  addr = pages[page].address << 6;
  buffer[0] = ((addr & 0xFF00) >> 8);
  buffer[1] = (addr & 0x00FF);
  for (d = 0; d < 64; d++) {  
    buffer[d + 2] = pages[page].data[d];
  }
  Wire.beginTransmission(0b01010000);
  Wire.write(buffer, 66);    
  Wire.endTransmission();
}

MEMCACHE memcache();

