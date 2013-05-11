/*
 * webserver.h
 *
 *  Created on: 29.04.2013
 *      Author: Michael Neuweiler
 */

#ifndef WEBSERVER_H_
#define WEBSERVER_H_

#include <Arduino.h>
#include "config.h"
#include <SPI.h>
#include <stdlib.h>
#include "motorctrl.h"
#include "logger.h"
#include "nic_handler.h"
#include "itoa.h"

class WebServer {

public:
	enum MethodType {
		MethodUnknown, MethodGet, MethodPost, MethodHead
	};
	enum PageIndex {
		Root, Status, Config, SysLog, Fault, About, PageNotFound
	};
	enum ConfigEntryType {
		Header, Numeric, String, Bitfield, Checkbox, Radio, Time, Date
	};
	struct ConfigEntry {
		char *label;
		ConfigEntryType type;
		uint16_t address;
		uint8_t numBytes;
		uint16_t maxLength;
		uint32_t min;
		uint32_t max;
	};

	WebServer(MOTORCTRL *);
	void handleTick();

private:
	void init();
	void renderPageNotFound();
	void renderStatusPage();
	void renderConfigPage();
	void renderSysLogPage();
	void renderFaultPage();
	void renderAboutPage();
	void pageTitle();
	void menu();
	void httpResponseHeader();
	void htmlHeader();
	void htmlCSS();
	void htmlFooter();
	void tableBegin();
	void tableNewRow();
	void tableEnd();
	void tableCell(char *, char *);
	void tableCell(char *, float);
	void createConfigEntry(ConfigEntry *);
	void buttons();
	void parseRequestParameters(char *, MethodType, PageIndex, bool, int);
	void processParameter(char *);
	ConfigEntry *findConfigEntry(char *);
	MethodType readHttpRequest(PageIndex &);
	MethodType readRequestLine(char *, PageIndex &);
	void readRequestHeaders(char *, int &, bool &);
	void readEntityBody(int, char *);
	bool readRequestLine(char *, char);
	PageIndex getPageIndex(char *);
};

#endif /* WEBSERVER_H_ */
