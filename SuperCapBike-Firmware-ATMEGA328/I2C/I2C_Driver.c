/*
 * I2C_Driver.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"

volatile bool TWI_Ready = true;

#define I2C_Status (TWSR & 0xF8)

ISR(TWI_vect){
	TWI_Ready = true;
}

TWI_Status Init_TWI(TWI_Data* I2C_Data, uint8_t Device_Address, uint8_t Register_Address, TWI_Modes Mode, uint8_t Data){
	
	if(I2C_Data == NULL){
		return TWI_FAULT;
	}
	
	I2C_Data->Device_Address = Device_Address;
	I2C_Data->Register_Address = Register_Address;
	I2C_Data->Mode = Mode;
	I2C_Data->Data = Data;
		
	TWSR = 0x00;
	TWBR = 72;
	
	return TWI_OK;
	
}

TWI_Status TWI_Handler(TWI_Data* I2C_Data){
	
	if(!TWI_Ready){
		return TWI_OK;
	}
	
	TWI_Ready = false;
	
	uint8_t Transmit_Code = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Clear the interrupt flag, enable TWI and TWI interrupts
	
	switch(I2C_State){
		
		case TWI_STATE_START:
		
			I2C_State = SEND_ADDRESS_WRITE;
			
			TWCR = Transmit_Code | (1 << TWSTA); 
			
			break;
			
			
		case TWI_STATE_REPEATED_START:	
		
			switch(I2C_Status){
				
				case ADDRESSING_REGISTER:
				
					I2C_State = SEND_ADDRESS_READ;
					
					break;
				
				default:
				
					I2C_State = STOP;
				
					return TWI_FAULT;
				
			}
		
			TWCR = Transmit_Code | (1 << TWSTA);
		
			return TWI_OK;
				
		case SEND_ADDRESS_READ:
		
			switch(I2C_Status){
				
				case REPEATED_START:
				
					I2C_State = READING;
					break;
			
				
				default:
				
					I2C_State = STOP;
					return TWI_FAULT;
				
			}
			
			TWDR = (I2C_Data->Device_Address << 1) + 1; 
			TWCR = Transmit_Code;
			return TWI_OK;
		
		case SEND_ADDRESS_WRITE:
			
			switch(I2C_Status){
				
				case START:
				
					I2C_State = ADDRESSING_REGISTER;
					break;
					
				default:
				
					I2C_State = STOP;
					return TWI_FAULT;
				
			}
						
			TWDR = (I2C_Data->Device_Address << 1);
			TWCR = Transmit_Code;
			return TWI_OK;
			  
				
		case ADDRESSING_REGISTER:

			switch(I2C_Status){

				case WRITE_ADDRESS_ACK: // Same code for sending device address and register address

					if (I2C_Data->Mode == READING_MODE) {
					
						I2C_State = REPEATED_START;
					
					}else{
					
						I2C_State = WRITING;
					
					}
					
					break;

				default:
	
					I2C_State = STOP;
					
					return TWI_FAULT;
			}
		
			TWDR = I2C_Data->Register_Address;
			TWCR = Transmit_Code;
			
			return TWI_OK;

		
		case WRITING:
			
			switch(I2C_Status){
				
				case WRITE_DATA_ACK: // Same code for sending device address and register address
				
					I2C_State = STOP;
					break;
					
				default:
				
					I2C_State = STOP;
					
					return TWI_FAULT;
				
			}
			
			TWDR = I2C_Data->Data;
			TWCR = Transmit_Code;
			
			return TWI_OK;
		
		case READING:
						
			switch(I2C_Status){
				
				case READ_ADDRESS_ACK:
				
					TWCR = Transmit_Code; // Ready to receive the 1 byte
					
					return TWI_OK;
					
				case BYTE_RECEIVED:
				
					I2C_Data->Data = TWDR;
					
					return TWI_OK;

				default:
				
					I2C_State = STOP;

					return TWI_FAULT;
					
			}
			
			break; // Just for correctness
			
		case STOP:
		
			TWCR = Transmit_Code | (1 << TWSTO);

	}
	
	return TWI_OK; 
	
}
