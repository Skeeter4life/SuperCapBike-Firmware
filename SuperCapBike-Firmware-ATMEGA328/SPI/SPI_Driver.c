/*
 * SPI_Driver.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/SPI.h"

// For my use case, I don't ever see the need of reading data from a secondary device. So, none of that logic is implemented here.

SPI_Status SPI_Main_Init(uint8_t SPI_Prescaler, Data_Order Order){
	
	// I am not too worried about clock phase or polarity
	
	DDRB |= (1 << PORTB5) | (1 << PORTB3); // SCK and MOSI set as outputs
	
	switch(SPI_Prescaler){
		
		switch(Order){
			
			case(LSB_First):
				
				SPCR = (1 << SPE) | (1 << MSTR) | (1 << DORD); // SPI enable, MSTR mode, LSB first
				break;
				
			case(MSB_First):
			
				SPCR = (1 << SPE) | (1 << MSTR);
				break;
		}		
		
		case 4:
		
			break;
		
		case 16:
		
			SPCR |= (1 << SPR0);
			
			break;
		
		case 64:
			
			SPCR |= (1 <<SPR1);
			
			break;
		
		case 128:
			
			SPCR |= (1 << SPR1) | (1 << SPR0);
			break;
		
		case 2:
			
			SPSR |= (1 << SPI2X);
			
			break;
		
		case 8:
			
			SPSR |= (1 << SPI2X);
			SPCR |= (1<<SPR0);

			break;
		
		case 32:
		
			SPSR |= (1 << SPI2X);
			SPCR |= (1<<SPR1);
		
			break;
			
		default:
			
			// Same as case 4, of course.
			
			return SPI_FAULT;
	}
	
	SPI_Config = Main;
	return SPI_OK;
			
}

SPI_Status SPI_Passive_Init(){ // Not receiving. Useful for re-programming over SPI
	
	DDRB &= ~((1 << PORTB5) | (1 << PORTB4) | (1 << PORTB3));
	SPI_Config = Passive;
	return SPI_OK; // Keep it consistent 
	
}

SPI_Status SPI_Transmit(uint8_t Data, Devices Device){
	
	if(SPI_Config != Main){
		return SPI_FAULT;
	}
	
	switch(Device){ // Handle SS
		
		case Device1:
			
			break;
			
		case Device2:
		
			break;
		
	}
	
	SPDR = Data;
	return SPI_OK;
	
}
