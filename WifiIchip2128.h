/*
 * ichip_2128.h
 *
 * Class to interface with the ichip 2128 based wifi adapter we're using on our board
 *
 * Created: 4/18/2013 8:10:00 PM
 *  Author: Collin Kidder
 */

/* The wifi adapter must be able to either create an adhoc network or connect to an existing network
 * the EEPROM has three entries for storing networks we'd like to be able to automatically join
 * If none of those networks can be found then the device will automatically try to form an
 * adhoc network with the 4th entry which is dedicated to that purpose.
 * All commands to the wifi start with "AT+i" and end with CR (ascii 13)
 *
 * Commands:
 * RP11 - get list of access points in range
 * RP7 - Status report (bit 10 if remote has changed a param)
 * RP8 - get system time
 * WWW - turn on HTTP server
 * WNXT - get next changed parameter (returns <param>=<value> or blank line at end)
 * WLTR - get/set transfer rate WIFI (0=max, 1=slow - 13 = max)
 * WRFU - turn on WIFI
 * WRFD - turn off wifi
 * WLBM - B wifi mode
 * WLGM - G wifi mode
 * AWPS - activate WPS mode
 * DIP - default IP address (set to 0.0.0.0 for DHCP client)
 * IPG - gateway address
 * SNET - subnet mask
 * DNS1 - DNS server
 * IPA - our IP address
 * DPSZ - DHCP server pool size (0 to disable server)
 * DSLT - DHCP server lease time
 * NTOD=1 - turn on time client (NTP)
 * WLCH - set WIFI channel
 * WLSI - SSID
 * WLWM - WEP MODE (0=disabled 1=64bit 2=128bit)
 * WLKx - WEP key
 * WLPP - WPA-PSK key
 * WSEC - 0 = TKIP 1 = AES
 * WSIx - x=0-9 = set SSIDs to associate with
 * WPPx - x=0-9 WPA pass phrases
 * WKYx - x=0-9 WEP keys
 * WSTx - x=0-9 security type = (0=none, 1=wep64, 2=WEP128,3=WPA/TKIP 4=WPA2
 *
 *
 * Web Parameters getting/setting
 * <param>=<value>   set
 * <param>?     get
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

#ifndef ICHIP2128_H_
#define ICHIP2128_H_

#include <Arduino.h>
#include "config.h"
#include "DeviceManager.h"
#include "Sys_Messages.h"
#include "DeviceTypes.h"
#include "ELM327Processor.h"
#include "WebSocket.h"
#include "SocketProcessor.h"
#include "Wifi.h"

static const String ichipErrorString = "I/ERROR";
static const String disconnect = "_DISCONNECT_";


enum ICHIP_COMM_STATE {IDLE, GET_PARAM, SET_PARAM, START_TCP_LISTENER, GET_ACTIVE_SOCKETS, POLL_SOCKET, SEND_SOCKET, GET_SOCKET};

struct Socket {
    int8_t handle;
    SocketProcessor *processor;
};

struct SendBuff {
    String cmd;
    ICHIP_COMM_STATE state;
    Socket *socket;
};

class WifiIchip2128 : public Wifi
{
public:
    WifiIchip2128();
    WifiIchip2128(USARTClass *which);
    void setup(); //initialization on start up
    void tearDown();
    void handleTick(); //periodic processes
    void handleMessage(uint32_t messageType, void* message);
    void handleStateChange(Status::SystemState, Status::SystemState);
    DeviceType getType();
    DeviceId getId();
    void process();

private:
    void requestNextParam(); //get next changed parameter
    void requestParamValue(String paramName);  //try to retrieve the value of the given parameter
    void setParam(String paramName, String value);  //set the given parameter with the given string
    void sendCmd(String cmd);
    void sendCmd(String cmd, ICHIP_COMM_STATE cmdstate);
    void sendCmd(String cmd, ICHIP_COMM_STATE cmdstate, Socket *socket);
    void sendBufferedCommand();
    void sendToSocket(Socket *socket, String data);
    void sendSocketUpdate();
    void processStartSocketListenerRepsonse();
    void processActiveSocketListResponse();
    void processIncomingSocketData();
    void processSocketSendResponse();
    void processParameterChange(char *buffer);
    void closeAllSockets();
    void closeSocket(Socket *socket);

    void reset();
    void factoryDefaults();
    void processSocketResponseSize();
    void requestIncomingSocketData();
    void requestActiveSocketList();
    void startSocketListener();

    char incomingBuffer[CFG_WIFI_BUFFER_SIZE]; //storage for one incoming line
    int ibWritePtr; //write position into above buffer
    SendBuff sendBuffer[CFG_SERIAL_SEND_BUFFER_SIZE];
    int psWritePtr;
    int psReadPtr;
    int tickCounter;
    char buffer[30]; // a buffer for various string conversions
    ICHIP_COMM_STATE state;
    bool didParamLoad;
    bool didTCPListener;
    int socketListenerHandle;
    uint32_t lastSendTime;
    uint32_t timeStarted;
    Socket *lastSendSocket;
    int remainingSocketRead;
    uint8_t watchdogCounter; // a counter to find out if ichip has crashed
    Socket socket[CFG_WIFI_NUM_SOCKETS];

    const String disconnect = "_DISCONNECT_";
    const String ichipCommandPrefix = "AT+i";
};

#endif
