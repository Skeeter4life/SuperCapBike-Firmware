/*
 * Dynamic_Ring_Buffer.h
 *
 *  Author: Andrew
 */ 


#ifndef DYNAMIC_RING_BUFFER_H_
#define DYNAMIC_RING_BUFFER_H_

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"

typedef enum Buffer_Item_Type{
	
	BUFFER_TYPE_PTR,
	BUFFER_TYPE_BYTE
	
}Buffer_Item_Type;

typedef struct Buffer_Item{
	
	Buffer_Item_Type Item_Type;
	
	union{
		uint8_t* Ptr;
		uint8_t Byte;
	}Item;
		
}Buffer_Item;

typedef enum Ring_Buffer_Status{
	
	BUFFER_FAULT = 0,
	BUFFER_OK,
	BUFFER_EMPTY
	
}Ring_Buffer_Status;

typedef enum Indexing_States{
	NONE,
	WRITE_LEADS_READ,
	OVERFLOW
}Indexing_States;


typedef struct Ring_Buffer{

	
	Buffer_Item* Buffer;
	
	uint16_t Read_Index;
	uint16_t Write_Index;
	
	uint16_t Size;
	uint8_t Increment;
	
	uint16_t Adjusted_Size;
	
	uint16_t Wrap_Index; // Index that the read pointer must wrap to (up to Adjusted_Size) before moving onto the newly allocated space.
	uint16_t Overflow_Index; // Index that marks where the newly allocated space is by realloc()
	
	Indexing_States Indexing_State;
	
}Ring_Buffer;

extern Ring_Buffer_Status Init_Buffer(Ring_Buffer* Ring_Buffer, uint16_t Size, uint8_t Increment);

extern Ring_Buffer_Status Write_to_Buffer(Ring_Buffer* Ring_Buffer, Buffer_Item* Data);

extern Ring_Buffer_Status Read_from_Buffer(Ring_Buffer* Ring_Buffer, Buffer_Item* Outgoing_Data);

extern Ring_Buffer_Status Free_Buffer(Ring_Buffer* Ring_Buffer);

extern Ring_Buffer_Status TWI_Add_W_To_Queue(Ring_Buffer* Buffer, uint8_t Device_Address, uint8_t Register_Address, uint8_t Data);

extern Ring_Buffer_Status TWI_Add_R_To_Queue(Ring_Buffer* Buffer, uint8_t Device_Address, uint8_t Register_Address, uint8_t* Data_Out);

extern Ring_Buffer_Status Fetch_TWI(Ring_Buffer* Buffer);

extern Ring_Buffer_Status IsEmpty(Ring_Buffer* Buffer);

extern bool TWI_Buffer_Enabled;

extern Ring_Buffer* p_TWI_Buffer;

#endif /* DYNAMIC_RING_BUFFER_H_ */