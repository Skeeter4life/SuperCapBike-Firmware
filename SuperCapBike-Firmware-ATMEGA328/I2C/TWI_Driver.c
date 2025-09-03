/*
 * TWI_Driver.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"
#include "../Headers/Timer_Counter.h"

volatile TWI_Data I2C_Data = {0}; 

volatile TWI_States Next_I2C_State = TWI_IDLE;

volatile TWI_Status I2C_Status = TWI_OK; // Useful for debugging and checking I suppose

ISR(TWI_vect){
	
	uint8_t Transmit_Code = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Clear the interrupt flag, enable TWI and TWI interrupts
	
	switch(Next_I2C_State){
		
		case TWI_REPEATED_START:
		
			switch(TWSR_Status){
				
				case WRITE_DATA_ACK:
					
					Next_I2C_State = TWI_ADDRESS_READ;
				
					break;
				
				default:
				
					Next_I2C_State = TWI_STOP;
					
					I2C_Status = TWI_FAULT;
					return;
				
			}

			TWCR = Transmit_Code | (1 << TWSTA);
			
			I2C_Status = TWI_OK;
			return;
		
		case TWI_ADDRESS_READ:
		
			switch(TWSR_Status){
				
				case REPEATED_START:
				
					Next_I2C_State = TWI_READING;
					break;
				
				
				default:
				
					Next_I2C_State = TWI_STOP;
					
					I2C_Status = TWI_FAULT;
					return;
					
			}
			
			
			TWDR = (I2C_Data.Device_Address << 1) + 1;
			TWCR = Transmit_Code;
			
			I2C_Status = TWI_OK;
			return;
		
		case TWI_ADDRESS_WRITE:
		
			switch(TWSR_Status){
				
				case START:
				
					Next_I2C_State = TWI_ADDRESS_REGISTER;
					break;
				
				default:
				
					Next_I2C_State = TWI_STOP;
				
					I2C_Status = TWI_FAULT;
					return;
			}

			TWDR = (I2C_Data.Device_Address << 1);
			TWCR = Transmit_Code;

			
			I2C_Status = TWI_OK;
			return;
		
		case TWI_ADDRESS_REGISTER:
		
			switch(TWSR_Status){

				case WRITE_ADDRESS_ACK: // Same code for sending device address and register address

					if (I2C_Data.Mode == READING_MODE){
				
						Next_I2C_State = TWI_REPEATED_START;
				
					}else{
				
						Next_I2C_State = TWI_WRITING;
				
					}
			
					break;

				default:
			
					Next_I2C_State = TWI_STOP;
			
					I2C_Status = TWI_FAULT;
					return;
				
			}
		
			TWDR = I2C_Data.Register_Address;
			TWCR = Transmit_Code;
		
			I2C_Status = TWI_OK;
			return;
		
		case TWI_WRITING:
		
			switch(TWSR_Status){
			
				case WRITE_DATA_ACK: // Same code for sending device address and register address
			
					Next_I2C_State = TWI_STOP;
					break;
			
				default:
			
					Next_I2C_State = TWI_STOP;
			
					I2C_Status = TWI_FAULT;
					return;
			
			}
		
			TWDR = I2C_Data.Data;
			TWCR = Transmit_Code;
		
			I2C_Status = TWI_OK;
			return;
		
		case TWI_READING:
		
			switch(TWSR_Status){
			
				case READ_ADDRESS_ACK:
			
					TWCR = Transmit_Code; // Ready to receive the 1 byte
			
					I2C_Status = TWI_OK;
					return;
			
				case BYTE_RECEIVED:
			
					*(I2C_Data.Data_Out) = TWDR;
			
					//Next_I2C_State = TWI_STOP;
					
					I2C_Status = TWI_OK;
					
					if(I2C_Data.Callback != NULL){
						I2C_Data.Callback();
					}	
					
					TWCR = Transmit_Code | (1 << TWSTO);
					Next_I2C_State = TWI_IDLE;
					
					return;
					
				default:
			
					Next_I2C_State = TWI_STOP;

					I2C_Status = TWI_FAULT;
					return;
			
			}
		
			break; // Just for correctness
		
		case TWI_STOP:
		
			TWCR = Transmit_Code | (1 << TWSTO); 
			Next_I2C_State = TWI_IDLE;
			
			break;
		
		case TWI_IDLE:
			
			I2C_Status = TWI_OK;
			return;
		
		case TWI_TIMEOUT:
		
			I2C_Status = TWI_FAULT;
			return;

	}

	
}

void Start_TWI(void){
	
	TWSR = 0x00;
	TWBR = 39; // 12 = 400 kHz SCL frequency at F_CLK = 16MHZ, 17 at F_CLK = 20MHz 0 = 1.25Mhz @ F_CLK = 20MHz
		
	Next_I2C_State = TWI_ADDRESS_WRITE;
	TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA); // Send start
	
}


TWI_Status TWI_Write(uint8_t Device_Address, uint8_t Register_Address, uint8_t Data){
	
	//while(Next_I2C_State != TWI_IDLE);
	
	I2C_Data.Device_Address = Device_Address;
	I2C_Data.Register_Address = Register_Address;
	I2C_Data.Data = Data;
	
	I2C_Data.Mode = WRITING_MODE;
	
	Start_TWI();
	
	return TWI_OK;

}

TWI_Status TWI_Read(uint8_t Device_Address, uint8_t Register_Address, volatile uint8_t* Data_Out, void (*Callback)(void)){
	
	//while(Next_I2C_State != TWI_IDLE);
	
	I2C_Data.Device_Address = Device_Address;
	I2C_Data.Register_Address = Register_Address;
	I2C_Data.Data_Out = Data_Out;
	I2C_Data.Callback = Callback;
	
	I2C_Data.Mode = READING_MODE;
	
	
	Start_TWI();
	
	return TWI_OK;

}