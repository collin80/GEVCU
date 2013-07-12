/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */ 

#include "timer.h"

volatile int8_t tickReady;
volatile int8_t AgingTimer;

void due_timer_interrupt();

/*
Make the timer0 interrupt every specified number of microseconds
*/
void setupTimer(long microSeconds) {
	//Setup timer 0 to operate
  //sam3x timer routine wants frequency instead of microseconds so a conversion is necessary
  uint32_t freq = 1000000ul / microSeconds;
  Timer3.setFrequency(freq).attachInterrupt(due_timer_interrupt).start();
  //startTimer3(freq, due_timer_interrupt);
}

void due_timer_interrupt() {
  tickReady = true;
  AgingTimer++;
}
