/*
 * timer.c
 *
 * Created: 1/6/2013 6:54:53 PM
 *  Author: Collin Kidder
 */ 

#include "timer.h"

volatile int8_t tickReady;
volatile int8_t AgingTimer;

#if defined(__arm__) // Arduino Due specific implementation

volatile void due_timer_interrupt();

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

#elif defined(__AVR__) // Machina specific implementation

void setupTimer(long microSeconds) {
	//Setup timer 0 to operate

	long clock = F_CPU / 1000000; // # of MHz
	clock *= microSeconds; //number of clocks we need to get the proper # of microseconds interrupt

	//the scaling modes are 1, 8, 64, 256, 1024
	//we have to check each one in turn to see if it results in a value
	//less than or equal to 255. Once we find such a value do the division
	//and set things accordingly. The easiest approach is to use a bunch of if/else
	//blocks - its not elegant or terribly efficient but it's easy to read
	//Also note that the interrupt rate could be different from the actual requested
	//rate. We can't help that. The timer only has a certain resolution. Using one of the 16 bit
	//timers would help here (if needed)
/*
	TCCR0A = 2; //CTC mode
	if (clock <= 255) {
		TCCR0B = 1; //Clock / 1 prescaler
		OCR0A = (int8_t)(clock);
	}
	else if ((clock / 8) <= 255) {
		TCCR0B = 2; //Clock / 8 prescaler
		OCR0A = (int8_t)(clock / 8);
	}
	else if ((clock / 64) <= 255) {
		TCCR0B = 3; //Clock / 64 prescaler
		OCR0A = (int8_t)(clock / 64);
	}
	else if ((clock / 256) <= 255) {
		TCCR0B = 4; //Clock / 256 prescaler
		OCR0A = (int8_t)(clock / 256);
	}
	else if ((clock / 1024) <= 255) {
		TCCR0B = 5; //Clock / 1024 prescaler
		OCR0A = (int8_t)(clock / 1024);
	}
	else { //it was still too long of a delay. Set the delay as long as possible and give up
		TCCR0B = 5; //Clock / 1024 scaler
		OCR0A = 255;
	}
	tickReady = false;
	TIMSK0 = 2; //Lastly, enable OCIE2A
*/
}

ISR(TIMER0_COMPA_vect) {
	tickReady = true;
    AgingTimer++;
}

#endif
