/*
 * I2C.h
 *
 *  Author: Andrew
 */ 


#ifndef I2C_H_
#define I2C_H_

#include "../Headers/Includes.h"

#define MCP23017_Address 0x20

enum TWI_State{
	
	TWI_STATE_START,
	TWI_STATE_REPEATED_START,
	SEND_ADDRESS_READ,
	SEND_ADDRESS_WRITE,
	ADDRESSING_REGISTER,
	WRITING,
	READING,
	STOP,
	
}I2C_State;

typedef enum TWI_Status{
	
	TWI_OK,
	DATA_RECEIVED,
	TWI_FAULT
		
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
	
	
	uint8_t Device_Address;
	uint8_t Register_Address;
	
	TWI_Modes Mode;
	uint8_t Data;

	uint8_t Received_Data;
	
}TWI_Data;

extern bool TWI_Ready;
extern TWI_Status Init_TWI(TWI_Data* I2C_Data, uint8_t Device_Address, uint8_t Register_Address, TWI_Modes Mode, uint8_t Data);
extern TWI_Status TWI_Handler(TWI_Data* I2C_Data);

#endif /* I2C_H_ */