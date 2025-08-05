/*
 * Dynamic_Ring_Buffer.h
 *
 *  Author: Andrew
 */ 


#ifndef DYNAMIC_RING_BUFFER_H_
#define DYNAMIC_RING_BUFFER_H_

typedef enum Ring_Buffer_Status{
	
	OK = 1,
	EMPTY = 0,
	FAULT = -1
	
}Ring_Buffer_Status;

typedef enum Indexing_States{
	NONE,
	WRITE_LEADS_READ,
	OVERFLOW
}Indexing_States;

typedef struct Ring_Buffer{

	
	uint8_t* Buffer;
	
	uint8_t Read_Index;
	uint8_t Write_Index;
	
	uint16_t Size;
	uint8_t Increment;
	
	uint8_t Adjusted_Size;
	uint8_t Wrap_Index; // Index that the read pointer must wrap to (up to Adjusted_Size) before moving onto the newly allocated space.
	uint8_t Overflow_Index; // Index that marks where the newly allocated space is by realloc()
	
	Indexing_States Indexing_State;
	
}Ring_Buffer;

extern Ring_Buffer_Status Init_Buffer(Ring_Buffer* Ring_Buffer, uint8_t Size, uint8_t Increment);

extern Ring_Buffer_Status Write_to_Buffer(Ring_Buffer* Ring_Buffer, uint8_t Data);

extern Ring_Buffer_Status Free_Buffer(Ring_Buffer* Ring_Buffer);

extern Ring_Buffer_Status Read_from_Buffer(Ring_Buffer* Ring_Buffer, uint8_t* Outgoing_Data);




#endif /* DYNAMIC_RING_BUFFER_H_ */