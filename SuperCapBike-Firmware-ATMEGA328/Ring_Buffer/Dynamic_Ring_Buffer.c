/*
 * Dynamic_Ring_Buffer.c
 *
 *  Author: Andrew
 */ 

#include "../Headers/Dynamic_Ring_Buffer.h"

bool TWI_Buffer_Enabled = false;

Ring_Buffer_Status Init_Buffer(Ring_Buffer* Ring_Buffer, uint16_t Size, uint8_t Increment){
	
	if(Size == 0 
	|| Increment == 0){
		return BUFFER_FAULT;
	}
	
	if(Size > 65535/sizeof(Ring_Buffer)){
		return BUFFER_FAULT; // Or call Increase_Buffer on startup? That seems wasteful.
	}
	
	Buffer_Item* p = (Buffer_Item*)malloc(Size * sizeof(Buffer_Item));
	
	if(p == NULL){
		return BUFFER_FAULT;
	}
	
	Ring_Buffer->Buffer = p;
		
	Ring_Buffer->Read_Index = 0;
	Ring_Buffer->Write_Index = 0;
	
	Ring_Buffer->Size = Size * sizeof(Buffer_Item);
	Ring_Buffer->Increment = Increment * sizeof(Buffer_Item); 
	
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
	
	uint16_t RB_Size = Ring_Buffer->Size;
	uint8_t RB_Increment = Ring_Buffer->Increment;
	
	if((uint32_t)RB_Size + RB_Increment > 65535 || RB_Size >= 2048){ // Overflow protection
		return BUFFER_FAULT;
	}
	
	Buffer_Item* p = (Buffer_Item*)realloc( Ring_Buffer->Buffer, (RB_Size + RB_Increment) ); // Opted for safe re-allocation
	
	if(p == NULL){
		return BUFFER_FAULT;
	}
	
	Ring_Buffer->Buffer = p;
	Ring_Buffer->Size = RB_Size + RB_Increment;
	
	return BUFFER_OK;
	
}

bool IsEmpty(Ring_Buffer* Ring_Buffer){
	
	if(Ring_Buffer->Indexing_State == NONE
	 && Ring_Buffer->Read_Index == Ring_Buffer->Write_Index){
		 return true;
	}
	
	return false;
	
}

Ring_Buffer_Status Write_to_Buffer(Ring_Buffer* Ring_Buffer, Buffer_Item* Data){
	
	uint16_t RB_Write_Index = Ring_Buffer->Write_Index;
	uint16_t RB_Read_Index = Ring_Buffer->Read_Index;
	uint16_t RB_Size = Ring_Buffer->Size;
	
	if(RB_Write_Index == RB_Size){ 
		
		Ring_Buffer->Write_Index = 0;
		Ring_Buffer->Indexing_State = WRITE_LEADS_READ;
		
	}
	
	if( Ring_Buffer->Indexing_State == WRITE_LEADS_READ && 
	RB_Read_Index == RB_Write_Index){
		
		if(Increase_Buffer(Ring_Buffer) == BUFFER_OK){
			
			Ring_Buffer->Indexing_State = OVERFLOW;
			
			Ring_Buffer->Overflow_Index = RB_Write_Index;
			
			Ring_Buffer->Adjusted_Size = RB_Size;
			
			Ring_Buffer->Wrap_Index = RB_Size;
	
			Ring_Buffer->Write_Index = Ring_Buffer->Size - Ring_Buffer->Increment;
			
		}else{
			
			return BUFFER_FAULT;
			
		}
		
	}
	
	Ring_Buffer->Buffer[Ring_Buffer->Write_Index] = *Data;
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


Ring_Buffer_Status Read_from_Buffer(Ring_Buffer* Ring_Buffer, Buffer_Item* Outgoing_Data){
	
	uint16_t RB_Read_Index = Ring_Buffer->Read_Index;
	
	if(IsEmpty(Ring_Buffer) == true){
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

Ring_Buffer_Status TWI_Add_W_To_Queue(Ring_Buffer* Buffer, uint8_t Device_Address, uint8_t Register_Address, uint8_t Data){
	
	if(IsEmpty(Buffer) && Next_I2C_State == TWI_IDLE){ // Nothing in queue, and TWI ready
		TWI_Write(Device_Address, Register_Address, Data);
		return BUFFER_OK;
	}
		
	Buffer_Item Temp_Item = {
		.Item_Type = BUFFER_TYPE_BYTE,
		.Item.Byte = WRITING_MODE
	};
	
	Ring_Buffer_Status a = Write_to_Buffer(Buffer, &Temp_Item); 
	
	if(a == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item.Byte = Device_Address;
	
	Ring_Buffer_Status b = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(b == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item.Byte = Register_Address;
	
	Ring_Buffer_Status c = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(c == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item.Byte = Data;
	
	Ring_Buffer_Status d = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(d == BUFFER_FAULT) return BUFFER_FAULT;
	
	return BUFFER_OK;
	
}

Ring_Buffer_Status TWI_Add_R_To_Queue(Ring_Buffer* Buffer, uint8_t Device_Address, uint8_t Register_Address, uint8_t* Data_Out){
	
	if(IsEmpty(Buffer) && Next_I2C_State == TWI_IDLE){
		TWI_Read(Device_Address, Register_Address, Data_Out, NULL); // Function callback is not implemented for queued items! (Maybe later)?
		return BUFFER_OK;
	}
	
	Buffer_Item Temp_Item = {
		.Item_Type = BUFFER_TYPE_BYTE,
		.Item.Byte = READING_MODE
	};
	
	Ring_Buffer_Status Status_Check = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(Status_Check == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item.Byte = Device_Address;
	
	Status_Check = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(Status_Check == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item.Byte = Register_Address;
	
	Status_Check = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(Status_Check == BUFFER_FAULT) return BUFFER_FAULT;
	
	Temp_Item.Item_Type = BUFFER_TYPE_PTR;
	Temp_Item.Item.Ptr = Data_Out;
	
	Status_Check = Write_to_Buffer(Buffer, &Temp_Item);
	
	if(Status_Check == BUFFER_FAULT) return BUFFER_FAULT;
	
	return BUFFER_OK;
	
	
}


Ring_Buffer_Status Fetch_TWI(Ring_Buffer* Buffer){
	
	if(IsEmpty(Buffer) == true){ 
		return BUFFER_EMPTY;
	}
	
	Buffer_Item Buffer_Out;
	
	if(Buffer == NULL){
		return BUFFER_FAULT;
	}
	
	uint8_t Device_Address;
	uint8_t Register_Address;
	
	TWI_Modes TWI_Mode;
	
	Ring_Buffer_Status Status_Check = Read_from_Buffer(Buffer, &Buffer_Out);
	if(Status_Check == BUFFER_FAULT || Status_Check == BUFFER_EMPTY) return BUFFER_FAULT;
	
	if(Buffer_Out.Item_Type == BUFFER_TYPE_PTR) return BUFFER_FAULT; // Maybe I check this too much, but better safe then sorry.
	
	if(Buffer_Out.Item.Byte != WRITING_MODE && Buffer_Out.Item.Byte != READING_MODE) return BUFFER_FAULT;
	
	TWI_Mode = Buffer_Out.Item.Byte;
	
	Status_Check = Read_from_Buffer(Buffer, &Buffer_Out); // Probably should check if this fails... SO MANY TO CHECK.
	if(Status_Check == BUFFER_FAULT || Status_Check == BUFFER_EMPTY) return BUFFER_FAULT; // I can probably make this cleaner somehow. Low priority.
	
	if(Buffer_Out.Item_Type == BUFFER_TYPE_PTR) return BUFFER_FAULT;
	
	Device_Address = Buffer_Out.Item.Byte;
	
	Status_Check = Read_from_Buffer(Buffer, &Buffer_Out);
	if(Status_Check == BUFFER_FAULT || Status_Check == BUFFER_EMPTY) return BUFFER_FAULT;
	
	if(Buffer_Out.Item_Type == BUFFER_TYPE_PTR) return BUFFER_FAULT;
		
	Register_Address = Buffer_Out.Item.Byte;
	
	
	if(TWI_Mode == READING_MODE){
		
		Status_Check = Read_from_Buffer(Buffer, &Buffer_Out);
		if(Status_Check == BUFFER_FAULT || Status_Check == BUFFER_EMPTY) return BUFFER_FAULT;
		
		if(Buffer_Out.Item_Type == BUFFER_TYPE_BYTE) return BUFFER_FAULT;
		
		TWI_Read(Device_Address, Register_Address, Buffer_Out.Item.Ptr, NULL);
		
		return BUFFER_OK;

	}
	
	Status_Check = Read_from_Buffer(Buffer, &Buffer_Out);
	if(Status_Check == BUFFER_FAULT || Status_Check == BUFFER_EMPTY) return BUFFER_FAULT;

	if(Buffer_Out.Item_Type == BUFFER_TYPE_PTR) return BUFFER_FAULT;
	
	TWI_Write(Device_Address, Register_Address, Buffer_Out.Item.Byte);
		
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