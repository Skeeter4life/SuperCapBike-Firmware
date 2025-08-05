/*
 * SPI_Queue.c
 *
 * Created: 08/01/2025 10:59:47 AM
 *  Author: andrewfi
 */ 

#include "../Headers/Includes.h"

uint8_t* p1;
uint16_t  Queue_Size = 0;

void Init_Queue(){
	 p1 = NULL;
}

int Add_To_Queue(uint8_t Data){
	
	Queue_Size++;
	
	uint8_t* temp = (uint8_t*)realloc(p1, Queue_Size); // Number of elements = number of byte
	
	if(temp == NULL){ // If re-allocation fails, we do not want to lose p1
		return -1;
	}
	
	p1 = temp;
	
	if(p1 == NULL){
		return -1;
	}
	
	if(Queue_Size == 1){
		return Data; // No need to swap numbers
	}
	
	uint8_t* p3 = (p1 + Queue_Size - 1);
	uint8_t* p2 = p3--;
	
	uint8_t Temp_Data;
	
	for(uint8_t i = 1; i < Queue_Size; i++){ // n-1 swaps
		
		Temp_Data = *p2;
		*p2 = *p3;
		*p3 = Temp_Data;
		
		if(p2 != p1){
			p2--;
			p3--; 
		}
	
	}
	
	return Data;

}

int Remove_From_Queue(){
	
	uint8_t Data;
	
	if(Queue_Size == 0){
		return -1;
	}
	
	Data = *p1;
	
	Queue_Size--;
	
	uint8_t* temp = (uint8_t*)realloc(p1, Queue_Size); // Don't need to call free of realloc(p1, 0)
	
	if(temp == NULL && Queue_Size != 0){
		return -1;
	}
	
	p1 = temp;
	
	return Data;
}

/*

[0]

[0, 1] -> [1, 0]

[1, 0, 2] -> [1, 2, 0] -> [2, 1. 0]

[2, 1, 0, 3] -> [2, 1, 3, 0] -> [2, 3, 1, 0] -> [3, 2, 1, 0]

If Queue_Size == 1
	do nothing
	
else
	Use two pointers, Swap The pointer at the end (say p2) with a pointer at end-1 (say p3) until p3 is at the start of the array (p3 == p1)



*/

// This is O(n) time and O(n) space. O(n^2) time depending on what realloc does I suppose. Not reallly great. Time to figure out ring buffers...
