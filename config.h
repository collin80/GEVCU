/*
 * config.h
 *
 * Defines the components to be used in the GEVCU and allows the user to configure
 * static parameters.
 * To differentiate between different platforms, the pre-compiler definitions __arm__
 * (Arduino Due) and __AVR__ (Arduino Uno based Machina) are used to compile the
 * machine dependent code.
 *
 * Note: Make sure with all pin defintions of your hardware that each pin number is
 *       only defined once.
 *
 *  Created on: 25.04.2013
 *      Author: Michael Neuweiler
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#if defined(__arm__) // Arduino Due specific implementation

#include <due_can.h>
#define CFG_DUE_CAN0_SPEED CAN_BPS_500K // specify the speed of the CAN0 bus
#define CFG_DUE_CAN1_SPEED CAN_BPS_500K // specify the speed of the CAN1 bus

#elif defined(__AVR__) // Machina specific implementation

#define CFG_MACHINA_CAN_PIN_CS 85 // specify the number of the CS pin
#define CFG_MACHINA_CAN_PIN_RESET 7 // specify the number of the RESET pin
#define CFG_MACHINA_CAN_PIN_INT 84 // specify the number of the INT pin
#define CFG_MACHINA_CAN_SPEED 500 // specify the speed of the CAN bus in kbps
#define SerialUSB Serial // a workaround to map the missing SerialUSB in the Uno to the standard Serial output

#else
#error Hardware not supported.
#endif

/*
 * configure the LCD monitor
 */
//#define CFG_LCD_MONITOR_ENABLED // comment out if no LCD is desired
#define CFG_LCD_MONITOR_PINS 8, 9, 4, 5, 6, 7 // specify the pin sequence used for LiquidCrystal initialisation
#define CFG_LCD_MONITOR_COLUMNS 16 // specify the number of columns of the display
#define CFG_LCD_MONITOR_ROWS 2 // specify the number of rows of the display


/*
 * specify if the webserver should be included in the build
 */
#define CFG_WEBSERVER_ENABLED // comment out if no webserver is desired

#endif /* CONFIG_H_ */
