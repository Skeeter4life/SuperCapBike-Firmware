/*
 * I2C.h
 *
 *  Author: Andrew
 */ 


#ifndef I2C_H_
#define I2C_H_

#include "../Headers/Includes.h"

#define MCP23017_Address 0x20
#define TWSR_Status (TWSR & 0xF8)

typedef enum TWI_States{
	
	TWI_REPEATED_START,
	TWI_ADDRESS_READ,
	TWI_ADDRESS_WRITE,
	TWI_ADDRESS_REGISTER,
	TWI_WRITING,
	TWI_READING,
	TWI_STOP,
	TWI_TIMEOUT,
	TWI_IDLE
	
}TWI_States;

typedef enum TWI_Status{
	
	TWI_FAULT = 0,
	TWI_OK,
	DATA_RECEIVED,
	TWI_BUSY
		
}TWI_Status;

typedef enum TWI_Modes{
	
	READING_MODE,
	WRITING_MODE
	
}TWI_Modes;

enum TWI_Codes{
	
	START = 0x08,
	REPEATED_START = 0x10,
	
	WRITE_ADDRESS_ACK = 0x18,
	WRITE_DATA_ACK = 0x28,
	
	READ_ADDRESS_ACK = 0x40,
	BYTE_RECEIVED = 0x58
	
		
}TWI_Codes;

typedef struct TWI_Data{
	
	
	volatile uint8_t Device_Address;
	volatile uint8_t Register_Address;
	
	volatile TWI_Modes Mode;
	volatile uint8_t Data;

	volatile uint8_t* Data_Out;
	void (*Callback)(void);
	
}TWI_Data;

extern TWI_Status TWI_Write(uint8_t Device_Address, uint8_t Register_Address, uint8_t Data); // I don't want to initialize a struct going into it
extern TWI_Status TWI_Read(uint8_t Device_Address, uint8_t Register_Address, volatile uint8_t* Data_Out, void (*Callback)(void));

extern volatile TWI_States Next_I2C_State;
extern volatile TWI_Status I2C_Status;

#endif /* I2C_H_ */