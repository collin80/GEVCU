/*
 * A wrapper for various NIC hardware.
 * Currently WIFI and Ethernet are supported.
 *
 *  Created on: 10.05.2013
 *      Author: Michael Neuweiler
 */

#include "nic_handler.h"

#if CFG_NIC_TYPE == CFG_NIC_TYPE_ETHERNET

//EthernetServer server(CFG_WEBSERVER_PORT);
EthernetClient client = NULL;

void NICHandler::init() {
//	byte mac[] = { CFG_WEBSERVER_MAC };
//	IPAddress ip(CFG_WEBSERVER_IP);
//
//	Ethernet.begin(mac, ip);
//	server.begin();
//	SerialUSB.print("Webserver is at ");
//	SerialUSB.println(Ethernet.localIP());
}

bool NICHandler::serverAvailable() {
//	client = server.available();
	return (client);
}

bool NICHandler::connected() {
	return client.connected();
}

int NICHandler::available() {
	return client.available();
}

void NICHandler::stop() {
	client.stop();
}

char NICHandler::read() {
	return client.read();
}

void NICHandler::print(char str[]) {
	client.print(str);
}

void NICHandler::print(uint8_t value) {
	client.print(value);
}

void NICHandler::print(uint16_t value) {
	client.print(value);
}

void NICHandler::print(uint32_t value) {
	client.print(value);
}

void NICHandler::print(float value) {
	client.print(value);
}

void NICHandler::println(char str[]) {
	client.println(str);
}

#elif CFG_NIC_TYPE == CFG_NIC_TYPE_WIFI

void NICHandler::init() {
}

bool NICHandler::serverAvailable() {
}

bool NICHandler::connected() {
}

int NICHandler::available() {
}

void NICHandler::stop() {
}

char NICHandler::read() {
}

void NICHandler::print(char str[]) {
}

void NICHandler::print(uint8_t value) {
}

void NICHandler::print(uint16_t value) {
}

void NICHandler::print(uint32_t value) {
}

void NICHandler::print(float value) {
}

void NICHandler::println(char str[]) {
}


#endif
