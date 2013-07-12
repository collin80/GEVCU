/*
 * ichip_2128.cpp
 *
 * Class to interface with the ichip 2128 based wifi adapter we're using on our board
 *
 * Created: 4/18/2013 8:10:00 PM
 *  Author: Collin Kidder
 */
 
#include "ichip_2128.h"

//initialization of hardware and parameters
void WIFI::init() {
}

//periodic processes
void WIFI::handleTick() {
}

//turn on the web server
void WIFI::enableServer() {
}

//turn off the web server
void WIFI::disableServer() {
}

//Determine if a parameter has changed, which one, and the new value
String WIFI::getNextParam() {
}

//try to retrieve the value of the given parameter
String WIFI::getParamById(String paramName) {
 
}

//set the given parameter with the given string
String WIFI::setParam(String paramName, String valu) {
}

WIFI::WIFI() {
  serialInterface = &Serial3; //default is serial 3 because that should be what our shield really uses
}

WIFI::WIFI(USARTClass *which) {
  serialInterface = which;
}

