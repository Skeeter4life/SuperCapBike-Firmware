/*
 * Dynamic_Ring_Buffer.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Includes.h"
#include "../Headers/Dynamic_Ring_Buffer.h"

Ring_Buffer_Status Init_Buffer(Ring_Buffer* Ring_Buffer, uint8_t Size, uint8_t Increment){
	
	if(Size == 0 
	|| Increment == 0){
		return BUFFER_FAULT;
	}
	
	uint8_t* p = (uint8_t*)malloc(Size);
	
	if(p == NULL){
		return BUFFER_FAULT;
	}
	
	Ring_Buffer->Buffer = p;
		
	Ring_Buffer->Read_Index = 0;
	Ring_Buffer->Write_Index = 0;
	
	Ring_Buffer->Size = Size;
	Ring_Buffer->Increment = Increment;
	
	Ring_Buffer->Adjusted_Size = 0;
	Ring_Buffer->Wrap_Index = 0;
	Ring_Buffer->Overflow_Index = 0;
	
	Ring_Buffer->Indexing_State = NONE;

	return BUFFER_OK;
	
}

Ring_Buffer_Status Increase_Buffer(Ring_Buffer* Ring_Buffer){
	
	if(Ring_Buffer == NULL){
		return BUFFER_FAULT;
	}
	
	uint8_t RB_Size = Ring_Buffer->Size;
	uint8_t RB_Increment = Ring_Buffer->Increment;
	
	uint8_t* p = (uint8_t*)realloc(Ring_Buffer->Buffer, RB_Size + RB_Increment); // Opted for safe re-allocation
	
	if(p == NULL){
		return BUFFER_FAULT;
	}
	
	Ring_Buffer->Buffer = p;
	Ring_Buffer->Size = RB_Size + RB_Increment;
	
	return BUFFER_OK;
	
}

Ring_Buffer_Status Write_to_Buffer(Ring_Buffer* Ring_Buffer, uint8_t Data){
	
	uint8_t RB_Write_Index = Ring_Buffer->Write_Index;
	uint8_t RB_Read_Index = Ring_Buffer->Read_Index;
	uint8_t RB_Size = Ring_Buffer->Size;
	
	if(RB_Write_Index == RB_Size){ 
		
		Ring_Buffer->Write_Index = 0;
		Ring_Buffer->Indexing_State = WRITE_LEADS_READ;
		
	}
	
	if( Ring_Buffer->Indexing_State == WRITE_LEADS_READ && 
	RB_Read_Index == RB_Write_Index){
		
		if(Increase_Buffer(Ring_Buffer) == 1){
			
			Ring_Buffer->Indexing_State = OVERFLOW;
			
			Ring_Buffer->Overflow_Index = RB_Write_Index;
			
			Ring_Buffer->Adjusted_Size = RB_Size;
			
			Ring_Buffer->Wrap_Index = RB_Size;
	
			Ring_Buffer->Write_Index = Ring_Buffer->Size - Ring_Buffer->Increment;
			
		}else{
			
			return BUFFER_FAULT;
			
		}
		
	}
	
	Ring_Buffer->Buffer[Ring_Buffer->Write_Index] = Data;
	Ring_Buffer->Write_Index++;
	
	return BUFFER_OK;
	
}

Ring_Buffer_Status Free_Buffer(Ring_Buffer* Ring_Buffer){
	
	if(Ring_Buffer == NULL){
		return BUFFER_FAULT;
	}
	
	free(Ring_Buffer->Buffer);
	
	Ring_Buffer->Buffer = NULL;
	
	return BUFFER_OK;
	
}


Ring_Buffer_Status Read_from_Buffer(Ring_Buffer* Ring_Buffer, uint8_t* Outgoing_Data){
	
	uint8_t RB_Read_Index = Ring_Buffer->Read_Index;
	
	if (Ring_Buffer->Indexing_State == NONE
	 && Ring_Buffer->Read_Index == Ring_Buffer->Write_Index) {
		
		return BUFFER_EMPTY;
		
	}
	
	*Outgoing_Data = Ring_Buffer->Buffer[Ring_Buffer->Read_Index];
	
	switch(Ring_Buffer->Indexing_State){
		
		case NONE:
			
			Ring_Buffer->Read_Index++;
			break;
		
		case WRITE_LEADS_READ:
		
			if(RB_Read_Index == Ring_Buffer->Size - 1){
					
				Ring_Buffer->Read_Index = 0;	
				
			}else{
				
				Ring_Buffer->Read_Index++;
				
			}
		
			break;
		
		case OVERFLOW:
		
			if(RB_Read_Index == (Ring_Buffer->Adjusted_Size - 1)){
				
				Ring_Buffer->Read_Index = 0;
								
			}else if(RB_Read_Index == Ring_Buffer->Overflow_Index - 1){
				
				Ring_Buffer->Read_Index = Ring_Buffer->Wrap_Index;
				Ring_Buffer->Indexing_State = WRITE_LEADS_READ;
				
			}else{
				
				Ring_Buffer->Read_Index++;
				
			}
		
			break;
		
		default:
		
			return BUFFER_FAULT;
		
	}
	
	if(Ring_Buffer->Read_Index == Ring_Buffer->Write_Index){
		Ring_Buffer->Indexing_State = NONE;
	}
	
	return BUFFER_OK;
	
}

// var[x] = *(var + x) 


/* DYNAMIC RING BUFFER

	NOTES:
	
	1) I believe this approach is the most optimized for AVR. By keeping track of indexes instead of pointers, I reduce 16 bit operations to 8 bit.
	
	2) I previously attempted a "true" dynamic ring buffer, where even in the overflow case, if the read pointer increments, that new section is marked as writable.
	However, it takes at least 2 new dynamically allocated pointers to keep track of each new section. As such, I deemed this method infeasible for this use case.  
	
	EDGE CASES:

		
	POTENTIAL FIXES:
	
	
*/