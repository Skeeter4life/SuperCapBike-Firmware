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

const uint32_t F_CLK = 16000000;
const uint32_t TC_CLK = 16000000; // TC_CLK can be asynchronous to F_CLK.

int main(void)
{
	sei();
	
	DDRB |= (1 << DDB0);
	DDRB |= (1 << DDB1);
	DDRD |= (1 << DDD7);
	DDRD |= (1 << DDD6);
	
	//bool Precisie_Mode = true;
	
	Timers Timer1 = _8_bit2;
	
	bool Timer1_Set = Configure_Timer_Tick(100, u_MiliSeconds, Timer1);
	
	Timers Timer2 = _8_bit1;
	
	bool Timer2_Set = Configure_Timer_Tick(1000, u_MicroSeconds, Timer2);
	
	Timers Timer3 = _16_bit;
	
	bool Timer3_Set = Configure_Timer_Tick(3, u_Seconds, Timer3);
	
	if(!Timer1_Set || !Timer2_Set || !Timer3_Set){
		PORTB = (1 << PORTB1);
	}
	
	int16_t W1 = EEPROM_Write(0x0000, 0x1C);
	
	int16_t R1 = EEPROM_Read(0x0000);
	
	int16_t W2 = EEPROM_Write(0x0001, 0x1D);
		
	if(!W1 || !W2 || !R1){
		PORTB = (1 << PORTB1);
	}
	
	Ring_Buffer Buffer1;
	uint8_t Received_Data = 0;
	
	Init_Buffer(&Buffer1, 10, 2);
	
	for(uint8_t i = 0; i <= 30; i++){
		Write_to_Buffer(&Buffer1, i);
		Read_from_Buffer(&Buffer1, &Received_Data);
	}
	
	Free_Buffer(&Buffer1); // Buffer functionality Verified with debugger. More rigorous testing soon.
	
	TWI_Data MCP23017;
	
	Init_TWI(&MCP23017, MCP23017_Address, 0x00, WRITING_MODE, 0b10101010);
	
	while (1 == true){
		
		if(TWI_Ready){
			
			TWI_Handler(&MCP23017);
			
		}

	}
	
	return 0;
}


/*
avrdude -c usbtiny -p m328 -U ?:?:"C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328\Debug\SuperCapBike-Firmware-ATMEGA328.hex":?
*/

/*
cd C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328
*/