/*
 * Includes.h
 *
 *  Author: Andrew
 */ 


#ifndef INCLUDES_H_
#define INCLUDES_H_

// Global includes:

#include <stdint.h>
#include <stdlib.h>

#include <avr/io.h>
#include <avr/interrupt.h>

//------- Bool Definition:

#define bool uint8_t
#define true 1
#define false 0

typedef struct Error_Log{
	
	char* Message[8];
	uint64_t Time; // Realistically, will never overflow.
	
}Error_Log;

#include "Timer_Counter.h"
extern const Timers Global_Timer;

#endif /* INCLUDES_H_ */