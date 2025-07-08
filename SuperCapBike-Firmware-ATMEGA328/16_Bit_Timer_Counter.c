/*
 * 16Bit_Timer_Counter.c
 *
 * Created: 2025-06-25 8:21:05 PM
 * Author : Andrew
 */ 

// Note/Reminder: The constexpr keyword was not added until the C23 standard

//------- Includes:
#include <avr/io.h>
#include <avr/interrupt.h>

//------- Bool Definition:
#define bool uint8_t
#define true 1
#define false 0

//------- ATMEGA328 Clock:
const uint32_t F_CLK = 8000000;

//------- Units (u_):
const uint32_t u_MicroSeconds = 1000000; // u_ defines a unit 
const uint16_t u_MiliSeconds = 1000;
const uint8_t u_Seconds = 1;


//------- Globals:
volatile unsigned long System_clock = 0; // Each tick is 1 defined by Configure_Timer
unsigned long Current_clock = 0;

volatile int64_t Remaining_Ticks = 0;
volatile int64_t Raw_Ticks = 0;

ISR(TIMER1_COMPA_vect){ // Danger of an interrupt being missed
		
	if(Remaining_Ticks == 0){
		
		System_clock++;
		PORTB ^= (1 << PORTB0); // Visual debug
		
		Remaining_Ticks = Raw_Ticks; 
		
	}else{
		
		Remaining_Ticks -= 65536; // I feel like this is one of the rare cases It makes sense do stuff in an ISR
			
		if(Remaining_Ticks < 65536){

			OCR1AH = ((Remaining_Ticks - 1) >> 8) & 0xFF;
			OCR1AL = ((Remaining_Ticks - 1) & 0xFF);
			Remaining_Ticks = 0;

				
		}
		
	}
	
}

int Configure_Timer_Step(uint16_t Time, unsigned int Units){ // Step is in seconds (Cool concept, but has an error of .5 unit maximum)
	
	System_clock = 0;
	Current_clock = 0;

	uint8_t Shift_Amount = 4; // Everything is 16* as large
	uint8_t Shift_Mask = (1 << Shift_Amount) - 1;
	uint16_t Prescaler = 1;
	
	uint64_t Scaled_Ticks = Time * F_CLK / Units;

	TIMSK1 |= (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enable interrupts on Output Compare Match Port A
		
	uint64_t Raw_Prescaler = Scaled_Ticks/65535; // Calculates the prescaler required to ensure that OCR1A is <= 65535 (How many ticks per unit * time) divided by max 16 bit unsigned value
		
	if(Raw_Prescaler > 1024){ // The Time will overflow the 16bit timer
		
		Remaining_Ticks = Scaled_Ticks/1024;
		Raw_Ticks = Scaled_Ticks/1024;
		
		TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12); // Set prescaler to 1024
		OCR1AH = 0xFF;
		OCR1AL = 0xFF; // Timer begins

		return 1;
		
	}else{
		
		uint16_t Clock_Dividers[5] = {1, 8, 64, 256, 1024}; // Prescalers available, 2^i (1, 8, 64, 256, 1024)
		
		for(uint8_t i = 0; i <= 4; i++){  // Logic to ensure that the Raw_Count <= uint16_t
			
			if(Clock_Dividers[i] >= Raw_Prescaler){
				Prescaler = Clock_Dividers[i];
				break;
			}
			
		}
	}
		
	switch(Prescaler){ // Don't need to clear TCCR1B, as it will be assigned:
		
		case 1:
			TCCR1B = (1 << CS10);
			break;
		case 8:
			TCCR1B = (1 << CS11);
			break;
		case 64:
			TCCR1B = (1 << CS11) | (1 << CS10);
			break;
		case 256:
			TCCR1B = (1 << CS12);
			break;
		case 1024:
			TCCR1B = (1 << CS12) | (1 << CS10);
			break;
	}
	
	TCCR1B |= (1 << WGM12); // Normal port operation until here
	
	unsigned long F_CLK_Shifted = F_CLK << Shift_Amount; // Used for fixed point arithmetic
	
	uint32_t F_Tick = (F_CLK_Shifted)/Prescaler; // Frequency that the timer actually ticks at

	uint64_t TOP_Temp = Time * F_Tick / Units; // How many increments there will be in Time units (seconds, milliseconds, microseconds)

	uint64_t TOP = (TOP_Temp >> Shift_Amount);

	if((TOP_Temp & Shift_Mask) >= 8){ // Round up (Top_TEMP & First digit)
		
		if(TOP != 0xFFFF){ // This enforces the "error of 0.5 a unit" ********
			TOP++;
		}
		
	}

	if(TOP <= 0){
		return 0; // Failed, adjust TCCR1B
	}
	
	OCR1AH = (TOP >> 8) & 0xFF;
	OCR1AL = (TOP & 0xFF); // Timer begins

	return 1;

}

/* CONFIGURE TIMER FUNCTION

	NOTES:
	
	Works best when F_CPU is 1MHZ or a multiple of 2. 
	Min F_CLK = 1MHZ
	Max F_CLK = 20MHZ
	
	EDGE CASES:
	
	Time domain t: t ? Z, {0 <= t <= 65535}
	
	POTENTIAL FIXES:
	
	Can make 17 bit
	Add additional logic -> simple division? (fixed point arithmitic)

*/

int main(void)
{
	
	sei();
	
	DDRB |= (1 << DDB0);
	DDRB |= (1 << DDB1);

	int8_t x = Configure_Timer_Step(1, u_MiliSeconds);
	if(x < 0){ // Error state
		//PORTB |= (1<<PORTB0);
	}

	while (1){
	}
	
	return 0;
}

//avrdude -c usbtiny -p m328 -U ?:?:"C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328\SuperCapBike-Firmware-ATMEGA328\Debug\SuperCapBike-Firmware-ATMEGA328.hex":?
