/*
 * Error_Logging.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"
#include "../Headers/Dynamic_Ring_Buffer.h"
#include "../Headers/EEPROM.h"

void Log_Error(Error_Log* Error){
	
	for(int8_t i = 7; i >= 0; i--){ // Big endian, MSB first.
		
		while(!EEPROM_Ready);
		
		if(Error->Message[i] == '\0') break;
		
		EEPROM_Write(EEPROM_Address, Error->Message[i]);
		EEPROM_Address++;
		
	}
	
	uint8_t* Time_Ptr = (uint8_t*)&(Error->Time);
	
	for(int8_t i = 7; i >= 0; i--){
		
		while(!EEPROM_Ready);
		EEPROM_Write(EEPROM_Address, Time_Ptr[i]);
		EEPROM_Address++;
		
	}
	
	TWI_Add_W_To_Queue(p_TWI_Buffer, MCP23017_Address, 0x00, 0b11101111);
	TWI_Add_W_To_Queue(p_TWI_Buffer, MCP23017_Address, 0x14, 0b00010000); // Turn on fault led
		
}