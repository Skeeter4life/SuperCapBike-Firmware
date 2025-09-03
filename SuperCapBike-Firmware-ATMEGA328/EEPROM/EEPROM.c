/*
 * EEPROM.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/EEPROM.h"

volatile bool EEPROM_Ready = false; // Not used
volatile uint16_t EEPROM_Address = 0;
volatile bool EEPROM_Enabled = false;

ISR(EE_READY_vect){ // Not used
	
	EEPROM_Ready = true;
		
}