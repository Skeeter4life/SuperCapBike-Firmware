/*
 * Timer_Counter.h

 *  Author: Andrew
 */ 


#ifndef TIMER_COUNTER_H_
#define TIMER_COUNTER_H_

#include "Includes.h"

extern const uint8_t u_Seconds; // Not in an enum to save memory
extern const uint16_t u_MiliSeconds;
extern const uint32_t u_MicroSeconds;

extern const uint32_t F_CLK;
extern const uint32_t TC_CLK;

typedef enum Timers{ // Has no tag name, so can't use enum Timers var (non-explicit version)
	_16_bit = 0,
	_8_bit1 = 1,
	_8_bit2 = 2
}Timers; // Creates a variable "Timers" of that anonymous enum type

typedef enum Timer_Modes{
	TIMER_PWM,
	TIMER_CTC,
	TIMER_NONE
}Timer_Modes;

typedef enum Timer_Status{
	TIMER_FAULT = 0,
	TIMER_OK,
}Timer_Status;

typedef enum Pins{
	// Timer1 (_16_bit)
	PB1_OC1A,
	PB2_OC1B,
	// Timer0 (_8_bit1)
	PD5_OC0B,
	PD6_OC0A,
	// Timer2 (_8_bit2)
	PB3_OC2A,
	PD3_OC2B
}Pins;


typedef struct PWM_Setup{
	
	Timers Timer;
	Pins Pin;
	
	uint16_t ICR; // Only for 16_bit timer
	
}PWM_Setup;

extern Timer_Status Configure_Timer(uint16_t Time, uint32_t Unit, Timers Selected_Timer);
extern Timer_Status Init_PWM(PWM_Setup* PWM);


#endif /* TIMER_COUNTER_H_ */