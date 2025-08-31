/*
 * Timer_Counter.h

 *  Author: Andrew
 */ 


#ifndef TIMER_COUNTER_H_
#define TIMER_COUNTER_H_

#include "Includes.h"

extern volatile uint64_t System_Ticks[3];

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

typedef enum Timer_Units{
	
	u_MicroSeconds = 1000000,
	u_MiliSeconds = 1000,
	u_Seconds = 1,
	
	Invalid = 0
		
}Timer_Units;


typedef struct PWM_Setup{
	
	Pins Pin;
	
	uint16_t ICR; // Only for 16_bit timer.
	
}PWM_Setup;

extern Timer_Status Configure_Timer(uint16_t Step, Timer_Units Unit, Timers Selected_Timer);
extern Timer_Status Init_PWM(PWM_Setup* PWM);
extern Timer_Status Configure_PWM(PWM_Setup* PWM, uint16_t Prescaler, uint8_t Duty_Cycle);

extern Timer_Modes Timer_Mode[3];
extern Timer_Units Timer_Unit[3];
extern uint16_t Timer_Step[3];


#endif /* TIMER_COUNTER_H_ */