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
	
	char Message[8]; // 7 char max
	uint64_t Time; // Realistically, will never overflow.
	
}Error_Log;

extern void Log_Error(Error_Log* Error);

#endif /* INCLUDES_H_ */