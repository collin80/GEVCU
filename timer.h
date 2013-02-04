/*
 * timer.h
 *
 * Created: 1/6/2013 8:26:58 PM
 *  Author: Collin
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

void setupTimer(long microSeconds);

extern volatile int8_t tickReady;

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */