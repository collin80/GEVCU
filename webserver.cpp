/*
 * A webserver which displays status information and lets the user configure
 * various parameters.
 *
 *  Created on: 29.04.2013
 *      Author: Michael Neuweiler
 *
 */

#include "webserver.h"

#ifdef CFG_WEBSERVER_ENABLED

extern MotorController *motorController;
NICHandler nic = NICHandler();
PrefHandler prefs = PrefHandler();

// Http header token delimiters
const char *spDelimiters = " \r\n";
const char *stxDelimiter = "\002";    // STX - ASCII start of text character
const char *paramDelimiter = "=";

// page uri's (must co-relate to the PageIndex enum in webserver.h)
const char http_uri0[] = "/";
const char http_uri1[] = "/status";
const char http_uri2[] = "/config";
const char http_uri3[] = "/syslog";
const char http_uri4[] = "/fault";
const char http_uri5[] = "/about";
const char *http_uris[] = { http_uri0, http_uri1, http_uri2, http_uri3, http_uri4, http_uri5 };

#define NUM_URIS  sizeof(http_uris)  / sizeof(char *)
#define BUFFER_SIZE 128

#define BUTTON_SAVE_TEXT "Save"

#define CONFIG_ENTRY_COLUMNS 2 // specify how many columns for config entries should be used in the config screen

// all fields listed in this array will be automatically handled by the webserver. New prefs might be added anytime here
//TODO currently String, Date, Time are only partially supported !
WebServer::ConfigEntry configEntry[] = {
		{ "Motor Controller", WebServer::Header, 0, 0, 0, 0, 0 },
		{ "Max RPM", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MAX_RPM, 2, 5, 0, 20000 },
		{ "Max Torque (1/10Nm)", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MAX_TORQUE, 2, 5, 0, 5000 },
		{ "Limp Scale (% of power)", WebServer::Numeric, EE_MOTORCTL_START + EEMC_LIMP_SCALE, 1, 3, 0, 100 },
		{ "Max Regen (% of max torque)", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MAX_REGEN, 1, 3, 0, 100 },
		{ "Regen Scale (% for all regen)", WebServer::Numeric, EE_MOTORCTL_START + EEMC_REGEN_SCALE, 1, 3, 0, 100 },
		{ "Cooling &#x00b0;C", WebServer::Numeric, EE_MOTORCTL_START + EEMC_COOLING, 1, 3, 0, 999 },
		{ "Motor Temp &#x00b0;C Min Gauge", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MIN_TEMP_MOTOR, 2, 3, 0, 999 },
		{ "Motor Temp &#x00b0;C Max Gauge", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MAX_TEMP_MOTOR, 2, 3, 0, 999 },
		{ "Inverter Temp &#x00b0;C Min Gauge", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MIN_TEMP_INV, 2, 3, 0, 999 },
		{ "Inverter Temp &#x00b0;C Max Gauge", WebServer::Numeric, EE_MOTORCTL_START + EEMC_MAX_TEMP_INV, 2, 3, 0, 999 },
		{ "Precharge Relay", WebServer::Checkbox, EE_MOTORCTL_START + EEMC_PRECHARGE_RELAY, 1, 0, 0, 0 },
		{ "Active High", WebServer::Bitfield, EE_MOTORCTL_START + EEMC_ACTIVE_HIGH, 1, 4, 0, 0 },
		{ "Contactor Relay", WebServer::Checkbox, EE_MOTORCTL_START + EEMC_CONTACTOR_RELAY, 1, 0, 0, 0 },

		{ "Throttle", WebServer::Header, 0, 0, 0, 0, 0 },
		{ "Min Channel 1", WebServer::Numeric, EE_THROTTLE_START + EETH_MIN_ONE, 2, 4, 0, 1023 },
		{ "Max Channel 1", WebServer::Numeric, EE_THROTTLE_START + EETH_MAX_ONE, 2, 4, 0, 1023 },
		{ "Min Channel 2", WebServer::Numeric, EE_THROTTLE_START + EETH_MIN_TWO, 2, 4, 0, 1023 },
		{ "Max Channel 2", WebServer::Numeric, EE_THROTTLE_START + EETH_MAX_TWO, 2, 4, 0, 1023 },
		{ "Pedal Pos Regen (1-1000)", WebServer::Numeric, EE_THROTTLE_START + EETH_REGEN, 2, 4, 0, 1000 },
		{ "Pedal Pos Forward Start (1-1000)", WebServer::Numeric, EE_THROTTLE_START + EETH_FWD, 2, 4, 0, 1000 },
		{ "Pedal Pos 50% (1-1000)", WebServer::Numeric, EE_THROTTLE_START + EETH_MAP, 2, 4, 0, 1000 },
		{ "Level Min Brake", WebServer::Numeric, EE_THROTTLE_START + EETH_BRAKE_MIN, 2, 4, 0, 1023 },
		{ "Level Max Brake", WebServer::Numeric, EE_THROTTLE_START + EETH_BRAKE_MAX, 2, 4, 0, 1023 },
		{ "Max Accel Regen (%)", WebServer::Numeric, EE_THROTTLE_START + EETH_MAX_ACCEL_REGEN, 1, 3, 0, 100 },
		{ "Max Brake Regen (%)", WebServer::Numeric, EE_THROTTLE_START + EETH_MAX_BRAKE_REGEN, 1, 3, 0, 100 },

		{ "System", WebServer::Header, 0, 0, 0, 0, 0 },
		{ "ADC0 Gain", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC0_GAIN, 2, 4, 0, 9999 },
		{ "ADC0 Offset (0-4095)", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC0_OFFSET, 2, 4, 0, 4095 },
		{ "ADC1 Gain", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC1_GAIN, 2, 4, 0, 9999 },
		{ "ADC1 Offset (0-4095)", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC1_OFFSET, 2, 4, 0, 4095 },
		{ "ADC2 Gain", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC2_GAIN, 2, 4, 0, 9999 },
		{ "ADC2 Offset (0-4095)", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC2_OFFSET, 2, 4, 0, 4095 },
		{ "ADC3 Gain", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC3_GAIN, 2, 4, 0, 9999 },
		{ "ADC3 Offset (0-4095)", WebServer::Numeric, EE_SYSTEM_START + EESYS_ADC3_OFFSET, 2, 4, 0, 4095 },
		{ "CAN0 Speed (kbps, 0=disable)", WebServer::Numeric, EE_SYSTEM_START + EESYS_CAN0_BAUD, 2, 4, 0, 1000 },
		{ "CAN1 Speed (kbps, 0=disable)", WebServer::Numeric, EE_SYSTEM_START + EESYS_CAN1_BAUD, 2, 4, 0, 1000 },
		{ "Serial Speed (x10 baud)", WebServer::Numeric, EE_SYSTEM_START + EESYS_SERUSB_BAUD, 2, 5, 0, 11520 },
		{ "TWI Speed (x1000 baud)", WebServer::Numeric, EE_SYSTEM_START + EESYS_TWI_BAUD, 2, 4, 0, 1000 },
		{ "Tick Rate (1=15us to 65536=1sec)", WebServer::Numeric, EE_SYSTEM_START + EESYS_TICK_RATE, 2, 4, 1, 65536 },
		{ "System Time", WebServer::Time, EE_SYSTEM_START + EESYS_RTC_TIME, 4, 10, 0, 4294967295 },
		{ "System Date", WebServer::Date, EE_SYSTEM_START + EESYS_RTC_DATE, 4, 10, 0, 4294967295 },

		{ "WiFi", WebServer::Header, 0, 0, 0, 0, 0 },
		{ "SSID", WebServer::String, EE_SYSTEM_START + EESYS_WIFIX_SSID, 32, 32, 0, 0 },
		{ "Channel (1-11)", WebServer::Numeric, EE_SYSTEM_START + EESYS_WIFIX_CHAN, 1, 2, 1, 11 },
		{ "DHCP (0=off,1=server,2=client)", WebServer::Numeric, EE_SYSTEM_START + EESYS_WIFIX_DHCP, 1, 1, 0, 2 },
		{ "Enable g-Mode (off=b)", WebServer::Checkbox, EE_SYSTEM_START + EESYS_WIFIX_MODE, 1, 1, 0, 0 },
		{ "IP Address", WebServer::Numeric, EE_SYSTEM_START + EESYS_WIFIX_IPADDR, 4, 15, 0, 4294967295 },
		{ "Key (13 bytes=WEP,8-40=WPA)", WebServer::String, EE_SYSTEM_START + EESYS_WIFIX_KEY, 40, 40, 0, 0 },
		{ "LogLevel (0=debug, 3=error)", WebServer::Numeric, EE_SYSTEM_START + EESYS_LOG_LEVEL, 1, 1, 0, 3 }
};

/*
 * Constructor of the webserver class
 *
 * \param motorController	The motor controller - required to query status information
 */
WebServer::WebServer(MotorController *motor_controller) {
	motorController = motor_controller;
	nic.init();
}

/*
 * Handle a tick from the main loop. Verify if a request is pending and handle it.
 *
 */
volatile void WebServer::handleTick() {
	if (nic.serverAvailable()) {
		PageIndex pageIndex;

		Logger::info("Webserver: client connected");

		MethodType callMethod = readHttpRequest(pageIndex);
		Logger::debug("method: %d, page: %d", callMethod, pageIndex);

		switch (pageIndex) {
		case Root:
			case Status:
			renderStatusPage();
			break;
		case Config:
			renderConfigPage();
			break;
		case SysLog:
			renderSysLogPage();
			break;
		case Fault:
			renderFaultPage();
			break;
		case About:
			renderAboutPage();
			break;
		default:
			renderPageNotFound();
			break;
		}

		delay(1); // give the web browser time to receive the data
		nic.stop();
		Logger::info("Webserver: client disonnected");
	}
}

/*
 * Render the page not found response.
 *
 */
void WebServer::renderPageNotFound() {
	nic.print("HTTP/1.1 404 Not Found\nServer: GEVCU\nContent-Type: text/html\n\n");
	htmlHeader();
	nic.print("<h1>Error 404: Sorry, that page cannot be found!</h1>");
	htmlFooter();
}

/*
 * Render the status page.
 *
 */
void WebServer::renderStatusPage() {
	httpResponseHeader();
	htmlHeader();
	pageTitle();
	menu();

	nic.print("<h2>Status</h2>");
	nic.print("<h3>Motor Controller</h3>");
	tableBegin();
	tableCell("Running", (char *) (motorController->isRunning() ? "true" : "false"));
	tableCell("Faulted", (char *) (motorController->isFaulted() ? "true" : "false"));
	tableNewRow();
	tableCell("Motor Temp (C)", motorController->getMotorTemp() / 10.0);
	tableCell("Inverter Temp (C)", motorController->getInverterTemp() / 10.0);
	tableNewRow();
	tableCell("Requested Throttle", motorController->getThrottle());
	char *gearSwitch;
	switch (motorController->getGearSwitch()) {
	case MotorController::GS_NEUTRAL:
		gearSwitch = "neutral";
		break;
	case MotorController::GS_FORWARD:
		gearSwitch = "forward";
		break;
	case MotorController::GS_REVERSE:
		gearSwitch = "reverse";
		break;
	case MotorController::GS_FAULT:
		gearSwitch = "fault";
		break;
	}
	tableCell("Gearswitch", gearSwitch);
	tableNewRow();
	tableCell("Requested Torque (Nm)", motorController->getRequestedTorque() / 10.0);
	tableCell("Actual Torque (Nm)", motorController->getActualTorque() / 10.0);
	tableNewRow();
	tableCell("Requested RPM", motorController->getRequestedRpm());
	tableCell("Actual RPM", motorController->getActualRpm());
	tableEnd();

	htmlFooter();
}

/*
 * Render the configuration page. The page is rendered generically from the array of configEntry.
 *
 */
void WebServer::renderConfigPage() {
	uint8_t value8;
	uint16_t value16;
	uint32_t value32;

	httpResponseHeader();
	htmlHeader();
	pageTitle();
	menu();

	nic.print("<h2>Configuration</h2>");
	nic.print("<form method=\"post\" action=\"config\">");

	int column = 0;
	for (int i = 0; i < sizeof(configEntry) / sizeof(ConfigEntry); i++) {
		if (column >= 0 && !(column % CONFIG_ENTRY_COLUMNS))
			tableNewRow();

		createConfigEntry(&configEntry[i]);

		column++;
		if (configEntry[i].type == Header)
			column = 0;
	}
	tableEnd(); // close the last table (opened by the last header entry)

	nic.print("<input type=\"submit\" name=\"b_save\" value=\"" BUTTON_SAVE_TEXT "\"/>");
	nic.print("</form>");

	htmlFooter();
}

void WebServer::renderSysLogPage() {
	httpResponseHeader();
	htmlHeader();
	pageTitle();
	menu();

	nic.print("<h2>System Log</h2>");
	nic.print("work in progress");

	htmlFooter();
}

void WebServer::renderFaultPage() {
	httpResponseHeader();
	htmlHeader();
	pageTitle();
	menu();

	nic.print("<h2>Faults</h2>");
	nic.print("work in progress");

	htmlFooter();

}

/*
 * Render the about page.
 *
 */
void WebServer::renderAboutPage() {
	httpResponseHeader();
	htmlHeader();
	pageTitle();
	menu();
	nic.print("<h2>About</h2>");
	nic.print("General Electric Vehicle Control Unit, created by<ul>");
	nic.print("<li>Collin Kidder</li>");
	nic.print("<li>Michael Neuweiler</li>");
	nic.print("<li>others ?</li>");
	nic.print("</ul>");
	htmlFooter();
}

/*
 * Insert the title of a page.
 *
 */
void WebServer::pageTitle() {
	nic.print("<h1>" CFG_VERSION "</h1>");
}

/*
 * Insert the menu structure on a page.
 *
 */
void WebServer::menu() {
	tableBegin();
	nic.print("<td class=\"menu\"><a href=\"status\">Status</a></td>");
	nic.print("<td class=\"menu\"><a href=\"config\">Configuration</a></td>");
	nic.print("<td class=\"menu\"><a href=\"syslog\">System Log</a></td>");
	nic.print("<td class=\"menu\"><a href=\"fault\">Faults</a></td>");
	nic.print("<td class=\"menu\"><a href=\"about\">About</a></td>");
	tableEnd();
}

/*
 * Insert the HTTP response header
 *
 */
void WebServer::httpResponseHeader() {
	nic.println("HTTP/1.1 200 OK");
	nic.println("Content-Type: text/html");
	nic.println("Cache-Control: no-store, no-cache, must-revalidate");
	nic.println("Pragma:no-cache");
	nic.println("Connection: close");
	nic.println("");
	nic.println("<!DOCTYPE HTML>");
}

/**
 * Insert the HTML header including the <body> element.
 *
 */
void WebServer::htmlHeader() {
	nic.print("<html><head>");
	nic.print("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">");
	nic.print("<title>" CFG_VERSION "</title>");
	htmlCSS();
	nic.print("</head><body>");
}

/**
 * Insert the stylesheet information.
 *
 */
void WebServer::htmlCSS() {
	nic.print("<style>");
	nic.print("body {color: #775E55; background-color: #FFFFFF; font-family: arial, verdana, sans-serif;}");
	nic.print("a {color: #775E55;}");
	nic.print("h1 {font-size: 3em; margin-top: 5px; margin-bottom: 5px;}");
	nic.print("h2 {color: #DE7945; margin-top: 10px; padding: 0px 0px 0px 0px;}");
	nic.print("table {width: 800px}");
	nic.print("td.menu {width: 160px; text-align: center;}");
//	nic.print("th.box {width: 30px; background-color: #DE7945;}");
//	nic.print("th.header {height: 30px; text-align: left; font-size: 1.2em; padding-left: 10px; color: #FFFFFF; background-color: #003E61;}");
//	nic.print("table.links {font-size: 1em; width: 100%;}");
	nic.print(".small {font-size: 0.6em;}");
	nic.print("table.topborder {border-top-width: 1; border-top-style: solid; border-top-color: #003E61;}");
	nic.print("</style>");
}

/*
 * Insert the HTML footer
 *
 */
void WebServer::htmlFooter() {
	nic.print("</body></html>");
}

/*
 * Insert the start of a table and the first row.
 *
 */
void WebServer::tableBegin() {
	nic.print("<table><tr>");
}

/*
 * End the last row and the table.
 *
 */
void WebServer::tableEnd() {
	nic.print("</tr></table>");
}

/*
 * Insert a pair of table cells for a label and its data
 *
 * \param label	The label to print in the left cell of the pair
 * \param data	The data to print next to the label in the right cell
 */
void WebServer::tableCell(char *label, char *data) {
	nic.print("<td>");
	nic.print(label);
	nic.print("</td><td>");
	nic.print(data);
	nic.print("</td>");
}
/*
 * Insert a pair of table cells for a label and its data.
 *
 * \param label	The label to print in the left cell of the pair
 * \param value	The float value to print next to the label in the right cell
 */
void WebServer::tableCell(char *label, float value) {
	nic.print("<td>");
	nic.print(label);
	nic.print("</td><td>");
	nic.print(value);
	nic.print("</td>");
}

// some temporary variables for read/write (global, so they allocated only once)
uint8_t val8;
uint16_t val16;
uint32_t val32;

/*
 * Insert a pair of table cells for a label and an input field.
 *
 * \param label	The label to print in the left cell of the pair
 * \param fieldId	The identifier of the input field
 * \param value	the pre-fill value of the input field
 */
void WebServer::createConfigEntry(ConfigEntry *entry) {

	if (entry->type == Header) {
		if (entry != &configEntry[0]) {
			tableEnd();
		}
		nic.print("<h3>");
		nic.print(entry->label);
		nic.print("</h3>");
		tableBegin();
	} else {
		nic.print("<td>");
		nic.print(entry->label);
		nic.print("</td><td>");

		switch (entry->type) {
		case Numeric:
			case Time:
			case Date:
			nic.print("<input name=\"f");
			nic.print(entry->address);
			nic.print("\" type=\"text\" value=\"");
			switch (entry->numBytes) {
			case 1:
				prefs.read(entry->address, &val8);
				nic.print(val8);
				break;
			case 2:
				prefs.read(entry->address, &val16);
				nic.print(val16);
				break;
			case 4:
				prefs.read(entry->address, &val32);
				nic.print(val32);
				break;
			default:
				Logger::error("can't process numeric with %d bytes", entry->numBytes);
				break;
			}
			nic.print("\" maxlength=\"");
			nic.print(entry->maxLength);
			if (entry->type == Numeric) {
				nic.print("\" min=\"");
				nic.print(entry->min);
				nic.print("\" max=\"");
				nic.print(entry->max);
			}
			nic.print("\"/>");
			break;
		case Bitfield:
			prefs.read(entry->address, &val8);
			for (int i = entry->maxLength - 1; i >=0 ; i--) {
				nic.print("<input type=\"checkbox\" name=\"f");
				nic.print(entry->address);
				nic.print("\" value=\"");
				nic.print((uint8_t)i);
				nic.print("\"");
				if (val8 & 1 << i)
					nic.print(" checked");
				nic.print("/>");
			}
			break;
		case Checkbox:
			prefs.read(entry->address, &val8);
			nic.print("<input type=\"checkbox\" name=\"f");
			nic.print(entry->address);
			nic.print("\" value=\"1\"");
			if (val8)
				nic.print(" checked");
			nic.print("/>");
			break;
		case Radio:
			//TODO not yet supported
			break;
		case String:
			//TODO not yet supported
			break;
		}
		nic.print("</td>");
	}
}

/*
 * Insert the end beginning of a table row.
 */
void WebServer::tableNewRow() {
	nic.print("</tr><tr>");
}

/*
 * Parse the request parameters in a GET or POST request.
 * As the length of the request might exceed available memory, the parameters are read one by one from
 * the stream instead of allocating one big chunk of memory.
 *
 * \param readBuffer	the temporary buffer to read data into
 * \param method	the request method (POST, GET, ...)
 * \param pageIndex	the index of the page to be displayed
 * \param isUrlEncoded specifies if + characters need to be replaced with spaces
 * \param contentLength the length of the parameter block in a POST request
 */
void WebServer::parseRequestParameters(char *readBuffer, MethodType method, PageIndex pageIndex, bool isUrlEncoded,
		int contentLength) {
	int readChars = 0;
	int bufferIndex = 0;
	char c;

	if (pageIndex == Config) {
		// clear parameters which do not get sumbitted if de-selected (checkboxes)
		for (int i = 0; i < sizeof(configEntry) / sizeof(ConfigEntry); i++) {
			if (configEntry[i].type == Bitfield || configEntry[i].type == Checkbox)
				prefs.write(configEntry[i].address, (uint8_t)0);
		}
	}

	for (;;) {
		c = nic.read();
		readChars++;

		// a CRLF or -1 terminates
		if (c == -1 || c == '\n') {
			readBuffer[bufferIndex] = 0;
			processParameter(readBuffer);
			break;
		}

		// a space means, the version string of a GET request follows, read until CRLF
		if (c == ' ' && contentLength == 0) {
			readBuffer[bufferIndex] = 0;
			processParameter(readBuffer);
			do {
				c = nic.read();
			} while (c != -1 && c != '\n');
			break;
		}

		// if isUrlEncoded is true, then the + must be replaced with a space
		if (isUrlEncoded && c == '+')
			c = ' ';

		// & separates parameters
		if (c == '&') {
			readBuffer[bufferIndex] = 0;
			processParameter(readBuffer);
			bufferIndex = 0;
		} else {
			if (bufferIndex < BUFFER_SIZE)
				readBuffer[bufferIndex++] = c;
		}

		// if contentLenght is > 0 then do not read more than content length
		if (contentLength > 0 && readChars >= contentLength)
			break;
	}

	// calculate and save the checksum
	prefs.saveChecksum();
}

/*
 * Update the stored preference values for a key/value pair in a GET or POST request
 *
 * \param parameter the key-value pair as delivered by the webbrowser, separated by a '='
 */
void WebServer::processParameter(char *parameter) {
	char *key = strtok(parameter, paramDelimiter);
	char *value = strtok(NULL, paramDelimiter);

	Logger::debug("key: %s, value: %s", key, value);

	if(key[0] != 'f') {
		Logger::debug("rejecting key %s", key);
	}
	key++;

	ConfigEntry *entry = findConfigEntry(key);
	if (entry) {
		Logger::debug("storing '%s' into %s", value, entry->label);
		switch (entry->type) {
		case Numeric:
		case Date:
		case Time:
			// perform minimalistic input validation
			val32 = atol(value);
			if (val32 > entry->max)
				val32 = entry->max;
			if (val32 < entry->min)
				val32 = entry->min;

			switch (entry->numBytes) {
			case 1:
				prefs.write(entry->address, (uint8_t)val32);
				break;
			case 2:
				prefs.write(entry->address, (uint16_t)val32);
				break;
			case 4:
				prefs.write(entry->address, (uint32_t)val32);
				break;
			default:
				Logger::error("can't store numeric with %d bytes", entry->numBytes);
			}
			break;
			case Bitfield:
				prefs.read(entry->address, &val8);
				val8 += 1 << atoi(value);
				prefs.write(entry->address, val8);
				break;
			case Checkbox:
				prefs.write(entry->address, (uint8_t)1);
				break;
			case Radio:
				//TODO not supported yet
				break;
			case String:
				//TODO not supported yet
				break;
		}

		if (entry->address ==  EE_SYSTEM_START + EESYS_LOG_LEVEL)
			Logger::setLoglevel((Logger::LogLevel) atoi(value));
	}
}

/*
 * Find the configEntry by its address
 *
 * \param addr string containing the address of the configEntry to look for
 * \retval the found config entry or NULL
 */
WebServer::ConfigEntry *WebServer::findConfigEntry(char *addr) {
	uint16_t address = atol(addr);
	if (address > 0) {
		for (int i = 0; i < sizeof(configEntry) / sizeof(ConfigEntry); i++) {
			if (configEntry[i].address == address)
				return &configEntry[i];
		}
	}
	Logger::warn("could not find configEntry by addr '%s'", addr);
	return (NULL);
}

/*
 * Read HTTP request, setting pageIndex, the requestContent and returning the method type.
 *
 */
WebServer::MethodType WebServer::readHttpRequest(WebServer::PageIndex &pageIndex) {
	char readBuffer[BUFFER_SIZE];
	int contentLength = 0;
	bool isUrlEncoded;

	MethodType method = readRequestLine(readBuffer, pageIndex);
	readRequestHeaders(readBuffer, contentLength, isUrlEncoded);

	// it was a POST request with parameters
	if (contentLength > 0) {
		parseRequestParameters(readBuffer, method, pageIndex, isUrlEncoded, contentLength);
	}
	return method;
}

/*
 * Read the first line of the HTTP request, setting pageIndex and returning the method type.
 * If it is a GET method with parameters, then process whatever follows the '?' separately.
 *
 * \param readBuffer	the temporary buffer to read data into
 * \param pageIndex	will be set with the recognized page index
 * \retval specifies the request method used (GET, POST, ...)
 *
 */
WebServer::MethodType WebServer::readRequestLine(char *readBuffer, WebServer::PageIndex &pageIndex) {
	MethodType method = MethodUnknown;

	bool processQuery = readRequestLine(readBuffer, '?');

	char * methodStr = strtok(readBuffer, spDelimiters);
	if (strcmp(methodStr, "GET") == 0)
		method = MethodGet;
	if (strcmp(methodStr, "POST") == 0)
		method = MethodPost;
	if (strcmp(methodStr, "HEAD") == 0)
		method = MethodHead;

	char * uri = strtok(NULL, spDelimiters);
	pageIndex = getPageIndex(uri);

	if (processQuery) {
		parseRequestParameters(readBuffer, method, pageIndex, true, 0);
	}
	return method;
}

/*
 * Read each header entry of the request until the terminating CRLF.
 *
 * \param readBuffer	the temporary buffer to read data into
 * \param contentLength	if specified in the request header, the length of a POST content will be set into this variable
 * \param isUrlEncoded specifies if the url is encoded or not
 */
void WebServer::readRequestHeaders(char *readBuffer, int & contentLength, bool & isUrlEncoded) {
	contentLength = 0;
	isUrlEncoded = true;

	do {
		readRequestLine(readBuffer, 0);
		char *fieldName = strtok(readBuffer, spDelimiters);
		char *fieldValue = strtok(NULL, spDelimiters);

		if (strcmp(fieldName, "Content-Length:") == 0) {
			contentLength = atoi(fieldValue);
		} else if (strcmp(fieldName, "Content-Type:") == 0) {
			if (strcmp(fieldValue, "application/x-www-form-urlencoded") != 0)
				isUrlEncoded = false;
		}
	} while (strlen(readBuffer) > 0);
}

/*
 * Analyze the URI and try to identify the page to be displayed (GET and POST).
 *
 * \param uri	the page part of the uri
 * \retval The identified page index (PageNotFound if not identified)
 */
WebServer::PageIndex WebServer::getPageIndex(char *uri) {
	PageIndex pageIndex = PageNotFound;

	Logger::debug("URI: %s", uri);
	for (int i = 0; i < NUM_URIS; i++) {
		if (strcmp(uri, http_uris[i]) == 0) {
			pageIndex = (PageIndex) i;
			break;
		}
	}
	return pageIndex;
}

/*
 * Read the next HTTP header record which is CRLF delimited. The CRLF is replaced with a string terminating null.
 * If a line is longer than the buffer, the rest of the line will be ignored !
 *
 * \param readBuffer	the buffer to receive the data into
 * \param stopChar		if not 0, the line will not be parsed further (required for stream-processing GET attributes)
 * \retval true if the stopChar was found and the remainder of the line needs to be read separately
 */
bool WebServer::readRequestLine(char *readBuffer, char stopChar) {
	bool stopCharFound = false;
	readBuffer[0] = 0;

	if (nic.connected() && nic.available()) {
		char c;
		int bufferIndex = 0;

		for (;;) {
			c = nic.read();

			// abort if read was not possible or the LF was reached
			if (c < 0 || c == '\n')
				break;

			if (stopChar && c == stopChar) {
				stopCharFound = true;
				break;
			}

			if (bufferIndex < BUFFER_SIZE)
				readBuffer[bufferIndex++] = c;
		}
		if (bufferIndex > 0 && readBuffer[bufferIndex - 1] == '\r')
			bufferIndex--;
		readBuffer[bufferIndex] = 0;
		Logger::debug("HTTP request line: %s", readBuffer);
	}
	return (stopCharFound);
}

#endif
