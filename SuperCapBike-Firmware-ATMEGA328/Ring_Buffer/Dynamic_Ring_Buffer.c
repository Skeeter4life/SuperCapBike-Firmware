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
		return FAULT;
	}
	
	uint8_t* p = (uint8_t*)malloc(Size);
	
	if(p == NULL){
		return FAULT;
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

	return OK;
	
}

Ring_Buffer_Status Increase_Buffer(Ring_Buffer* Ring_Buffer){
	
	if(Ring_Buffer == NULL){
		return FAULT;
	}
	
	uint8_t RB_Size = Ring_Buffer->Size;
	uint8_t RB_Increment = Ring_Buffer->Increment;
	
	uint8_t* p = (uint8_t*)realloc(Ring_Buffer->Buffer, RB_Size + RB_Increment);
	
	if(p == NULL){
		return FAULT;
	}
	
	Ring_Buffer->Buffer = p;
	Ring_Buffer->Size = RB_Size + RB_Increment;
	
	return OK;
	
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
			
			return FAULT;
			
		}
		
	}
	
	Ring_Buffer->Buffer[Ring_Buffer->Write_Index] = Data;
	Ring_Buffer->Write_Index++;
	
	return OK;
	
}

Ring_Buffer_Status Free_Buffer(Ring_Buffer* Ring_Buffer){
	
	if(Ring_Buffer == NULL){
		return FAULT;
	}
	
	free(Ring_Buffer->Buffer);
	
	Ring_Buffer->Buffer = NULL;
	
	return OK;
	
}


Ring_Buffer_Status Read_from_Buffer(Ring_Buffer* Ring_Buffer, uint8_t* Outgoing_Data){
	
	uint8_t RB_Read_Index = Ring_Buffer->Read_Index;
	
	if (Ring_Buffer->Indexing_State == NONE
	 && Ring_Buffer->Read_Index == Ring_Buffer->Write_Index) {
		
		return EMPTY;
		
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
		
			return FAULT;
		
	}
	
	if(Ring_Buffer->Read_Index == Ring_Buffer->Write_Index){
		Ring_Buffer->Indexing_State = NONE;
	}
	
	return OK;
	
}

// var[x] = *(var + x) 