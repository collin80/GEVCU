/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */ 

#include "timer.h"

volatile int8_t tickReady;
volatile int8_t AgingTimer;

volatile void due_timer_interrupt();

/*
Make the timer interrupt every specified number of microseconds
*/
void setupTimer(long microSeconds) {
  //sam3x timer routine wants frequency instead of microseconds so a conversion is necessary
  uint32_t freq = 1000000ul / microSeconds;
  startTimer9(freq, due_timer_interrupt);
}

volatile void due_timer_interrupt() {
  tickReady = true;
  AgingTimer++;
}
