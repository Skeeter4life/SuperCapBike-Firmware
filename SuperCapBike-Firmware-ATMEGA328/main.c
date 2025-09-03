/*
 * main.c
 *
 *  Author: Andrew
 */ 

//------- Includes:
#include "Headers/Includes.h"
#include "Headers/EEPROM.h"
#include "Headers/Timer_Counter.h"
#include "Headers/Dynamic_Ring_Buffer.h"
#include "Headers/I2C.h"
#include "Headers/Motor_Driver.h"

const uint32_t F_CLK = 8000000; // Currently using internal RC oscillator
const uint32_t TC_CLK = 8000000; // TC_CLK can be asynchronous to F_CLK.

const Timers Global_Timer = _8_bit2;

Ring_Buffer* p_TWI_Buffer;

int main(void)
{
	sei();
	
	Motor_Status Motor_Setup = Init_Motor();
	
	if(Motor_Setup == Motor_FAULT) return 1;
	
	MCUCR |= (1 << PUD); // Disable pull up resistors
	
	//DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2) | (1 << DDB3);
	//DDRD |= (1 << DDD6)| (1 << DDD7) | (1 << DDD5) | (1 << DDD3);
	
	Timer_Status Timer1_Set = Configure_Timer(1, u_MiliSeconds, Global_Timer); // 8 bit2 is free in my case, not needed for PWM
	
	if(!Timer1_Set){
		
		Error_Log Timer_Error = {
			.Message = "TIMER1",
			.Time = 0	
		};
		
		Log_Error(&Timer_Error);
		
	}
	//
	//Timers Timer2 = _8_bit1;
	//
	//Timer_Status Timer2_Set = Configure_Timer(5000, u_MicroSeconds, Timer2);
	//
	//Timers Timer3 = _16_bit;
	//
	//Timer_Status Timer3_Set = Configure_Timer(3, u_Seconds, Timer3);
	//
	//PWM_Setup Phase1;
	//
	//Phase1.Pin = PD5_OC0B;
	//
	//Timer1_Set = Init_PWM(&Phase1);
	//
	//Configure_PWM(&Phase1, 1, 50);
	//
	//if(!Timer1_Set || !Timer2_Set || !Timer3_Set){
		//PORTB = (1 << PORTB1);
	//}
		//
	///*
	//int16_t W1 = EEPROM_Write(0x0000, 0x1C);
	//
	//int16_t R1 = EEPROM_Read(0x0000);
	//
	//int16_t W2 = EEPROM_Write(0x0001, 0x1D);
		//
	//if(!W1 || !W2 || !R1){
		//PORTB = (1 << PORTB1);
	//}*/
		//
	//uint8_t Received_Data = 0;
	//
	Ring_Buffer TWI_Buffer;
	Ring_Buffer* p_TWI_Buffer = &TWI_Buffer; 
	
	TWI_Buffer_Enabled = true;
	
	Init_Buffer(p_TWI_Buffer, 25, 25);
	
	//TWI_Add_W_To_Queue(p_TWI_Buffer, MCP23017_Address, 0x01, 0b11111111);// Non imperative TWI operations. Only allowable under a certain speed.
	//TWI_Add_R_To_Queue(p_TWI_Buffer, MCP23017_Address, 0x13, &Received_Data);
	
	//TWI_Read(MCP23017_Address, 0x00, &Received_Data, NULL);
	
	while(1){
		
		if(TWI_Buffer_Enabled && Current_Speed <= 20 && !IsEmpty(p_TWI_Buffer) && Next_I2C_State == TWI_IDLE){
			Fetch_TWI(p_TWI_Buffer);
		}

		
	}
	
	//Free_Buffer(p_TWI_Buffer);
	
	//while (1 == true){
		//
		//if(TWI_Ready == true){
			//
			//TWI_Handler(&MCP23017);
			//
		//}
	//
	//} // Part of TWI_Old
	
	return 0;
}

/*
avrdude -c usbtiny -p m328 -U ?:?:"C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328\Debug\SuperCapBike-Firmware-ATMEGA328.hex":?
*/

/*
cd C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328
*/