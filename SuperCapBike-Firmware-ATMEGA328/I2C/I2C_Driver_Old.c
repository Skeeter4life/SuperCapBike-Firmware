/*
 * I2C_Driver.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"
#include "../Headers/Timer_Counter.h"

volatile bool TWI_Ready = false;
volatile uint8_t I2C_Status = 0xF8;

// This is the OLD TWI Driver! This took me a fair bit to understand why this did not work properly, but, the reasoning is very interesting! (Configured to not compile)
// See if you can figure out why yourselves...

/*
When an event requiring the attention of the application occurs on the TWI bus, the TWI
Interrupt Flag (TWINT) is asserted. In the next clock cycle, the TWI Status Register (TWSR) is updated with a
status code identifying the event.
*/

/*
The TWSR only contains relevant status information when the TWI Interrupt
Flag is asserted.
*/

/*
At all other times, the TWSR contains a special status code indicating that no relevant status
information is available. (0xF8)
*/

/*
As long as the TWINT Flag is set, the SCL line is held low. This allows the application
software to complete its tasks before allowing the TWI transmission to continue.
*/

ISR(TWI_vect){
	
	TWI_Ready = true;
	I2C_Status = TWSR & 0xF8;
	TWCR = TWCR & ~(1 << TWINT);
	
}

TWI_Status Init_TWI(TWI_Data* I2C_Data, uint8_t Device_Address, uint8_t Register_Address, TWI_Modes Mode, uint8_t Data){
	
	if(I2C_Data == NULL){
		return TWI_FAULT;
	}
	
	I2C_Data->Device_Address = Device_Address;
	I2C_Data->Register_Address = Register_Address;
	I2C_Data->Mode = Mode;
	I2C_Data->Data = Data;
	
	I2C_State = TWI_STATE_START;
		
	TWSR = 0x00; 
	TWBR = 255; // 72 = 100 kHz SCL frequency
	
	TWI_Ready = true;
	
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
				
				case WRITE_DATA_ACK:
				
					I2C_State = SEND_ADDRESS_READ;
					
					break;
				
				default:
				
					I2C_State = TWI_STOP;
				
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
				
					I2C_State = TWI_STOP;
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
				
					I2C_State = TWI_STOP;
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
	
					I2C_State = TWI_STOP;
					
					return TWI_FAULT;
			}
			
			TWDR = I2C_Data->Register_Address;
			TWCR = Transmit_Code;
			
			return TWI_OK;
		
		case WRITING:
			
			switch(I2C_Status){
				
				case WRITE_DATA_ACK: // Same code for sending device address and register address
				
					I2C_State = TWI_STOP;
					break;
					
				default:
				
					I2C_State = TWI_STOP;
					
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
				
					I2C_Data->Received_Data = TWDR;
										
					I2C_State = TWI_STOP;
					
					return TWI_OK;

				default:
				
					I2C_State = TWI_STOP;

					return TWI_FAULT;
					
			}
			
			break; // Just for correctness
			
		case TWI_STOP:
		
			TWCR =  (1 << TWEN) | (1 << TWIE) | (1 << TWSTO);
			I2C_State = TWI_IDLE;
			TWI_Ready = false;
			
			break;
			
		case TWI_IDLE:
		
			return TWI_OK;
			
		case TWI_TIMEOUT:
			
			return TWI_FAULT;

	}
	
	return TWI_OK; 
	
}

/* I2C DRIVER:

	NOTES:
	
	1) Can add functionality to send multiple bytes at a time, however, the general theme across these drivers is its been optimized for my hardware. 
	
	EDGE CASES:
	
	POTENTIAL FIXES:
	
*/
