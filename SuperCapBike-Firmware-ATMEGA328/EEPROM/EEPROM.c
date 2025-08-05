/*
 * EEPROM.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/EEPROM.h"

volatile bool EEPROM_Ready = false; // Not used

ISR(EE_READY_vect){ // Not used
	
	EEPROM_Ready = true;
		
}