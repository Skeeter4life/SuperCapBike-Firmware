/*
 * Timer_Counter.h

 *  Author: Andrew
 */ 


#ifndef TIMER_COUNTER_H_
#define TIMER_COUNTER_H_

#include "Includes.h"

extern const uint8_t u_Seconds;
extern const uint16_t u_MiliSeconds;
extern const uint32_t u_MicroSeconds;

extern const uint32_t F_CLK;
extern const uint32_t TC_CLK;

typedef enum Timers{ // Has no tag name, so can't use enum Timers var (non-explicit version)
	_16_bit = 0,
	_8_bit1 = 1,
	_8_bit2 = 2
}Timers; // Creates a variable "Timers" of that anonymous enum type


extern bool Configure_Timer_Tick(uint16_t Time, uint32_t Unit, Timers Timer);


#endif /* TIMER_COUNTER_H_ */