/*
 * nic_handler.h
 *
 *  Created on: 10.05.2013
 *      Author: Michael Neuweiler
 */

#ifndef NIC_HANDLER_H_
#define NIC_HANDLER_H_

#include <Arduino.h>
#include <stdint.h>
#include "config.h"

#if CFG_NIC_TYPE == CFG_NIC_TYPE_ETHERNET

#include <Ethernet.h>
#elif CFG_NIC_TYPE == CFG_NIC_TYPE_WIFI

#else
#error CFG_WEBSERVER_TYPE not supported, check config.h
#endif

class NICHandler {
public:
	void init();
	bool serverAvailable();
	bool connected();
	int available();
	void stop();
	char read();
	void print(char str[]);
	void print(uint8_t value);
	void print(uint16_t value);
	void print(uint32_t value);
	void print(float value);
	void println(char str[]);
};

#endif /* NIC_HANDLER_H_ */
