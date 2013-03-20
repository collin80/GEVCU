/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */ 

#include "Arduino.h"
#include "timer.h"
#include <DueTimer.h>

volatile int8_t tickReady;
volatile void due_timer_interrupt();
volatile int8_t AgingTimer;

/*
Make the timer0 interrupt every specified number of microseconds
*/
void setupTimer(long microSeconds) {
	//Setup timer 0 to operate
  //sam3x timer routine wants frequency instead of microseconds so a conversion is necessary
  uint32_t freq = 1000000ul / microSeconds;
  startTimer3(freq, due_timer_interrupt);
}

volatile void due_timer_interrupt() {
  tickReady = true;
  AgingTimer++;
}

