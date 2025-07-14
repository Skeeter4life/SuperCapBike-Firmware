/*
 * 16Bit_Timer_Counter.c
 *
 * Created: 2025-06-25 8:21:05 PM
 * Author : Andrew
 */ 

// Note/Reminder: The constexpr keyword was not added until the C23 standard
// Some may find my commenting superfluous, but I just want to make sure I am being clear... :)

//------- Includes:
#include <avr/io.h>
#include <avr/interrupt.h>

//------- Bool Definition:
#define bool uint8_t
#define true 1 // I believe this defaults to an int, which is 1 byte in AVR C?
#define false 0

//------- ATMEGA328 Clock:
const uint32_t F_CLK = 16000000;

//------- Units (u_):

const uint32_t u_MicroSeconds = 1000000; // u_ defines a unit 
const uint16_t u_MiliSeconds = 1000;
const uint8_t u_Seconds = 1;


//------- Timer Globals:

volatile uint64_t System_Ticks = 0; // Each tick is 1 defined by Configure_Timer

volatile int32_t Calculated_Ticks = 0;
volatile int32_t Remaining_Ticks = 0;

ISR(TIMER1_COMPA_vect){ 
	
	PORTB |= (1 << PORTB0);
		
	if(Remaining_Ticks == 0){
				
		System_Ticks++; // This operation is hilariously slow
		
		PORTB &= ~(1 << PORTB0);
		Remaining_Ticks = Calculated_Ticks; // Reset the counter
		OCR1A = 0xFFFF; 
		
	}else{
					
		if(Remaining_Ticks <= 65535){

			OCR1AH = (Remaining_Ticks >> 8) & 0xFF;
			OCR1AL = Remaining_Ticks & 0xFF;
			Remaining_Ticks = 0;

				
		}else{
			
			Remaining_Ticks -= 65536; // Tricky! 65536 Ticks as it rolls over back to 0 :).
			
		}
		
		
	}
	
}

bool Configure_Timer_Tick(uint16_t Time, uint32_t Units){ // All relevent types were optimized by calculating the largest possible values to Configure_Timer_Step()
	
	System_Ticks = 0; // Reset the system ticks
	
	uint64_t Numerator = Time * F_CLK;
	
	uint64_t Scaled_Ticks = Numerator / (Units << 1); // How many times we have to count for the requested time to have passed at the current clock frequency
	
	TIMSK1 |= (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enabled interrupt for progrm at TIMER1_COMPA_vect to be executed on compare match
		
	uint16_t Prescaler = 1;
	uint32_t Calculated_Prescaler = Scaled_Ticks/65535; // Calculates the prescaler required to ensure that OCR1A is <= 65535
		
	if(Calculated_Prescaler > 1024){
		
		 // The required count will overflow 16bits even with the largest available prescaler 
		
		Calculated_Ticks = (Scaled_Ticks >> 10); // Divide by 1024
		Remaining_Ticks = Calculated_Ticks; 
		
		TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12); // Set prescaler to 1024
		OCR1AH = 0xFF;
		OCR1AL = 0xFF; // Timer begins

		return true;
		
	}else{
		
		uint16_t Clock_Dividers[5] = {1, 8, 64, 256, 1024}; // Prescalers available, 2^i (1, 8, 64, 256, 1024)
		
		for(uint8_t i = 0; i <= 4; i++){  // Logic to ensure that the Raw_Count <= uint16_t
			
			if(Clock_Dividers[i] >= Calculated_Prescaler){
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
	
	// Rounding integer division (A new trick I learned) reduces error of Timer_Top ideally to +- 0.5:
	
	uint32_t Denomenator = Prescaler * Units;
	uint16_t Timer_Top = (Numerator + (Denomenator/2)) / Denomenator;
	
	OCR1AH = (Timer_Top >> 8) & 0xFF;
	OCR1AL = (Timer_Top & 0xFF); // Timer begins

	return true;

}

/* CONFIGURE TIMER FUNCTION

	NOTES:
	
	1) Works best when F_CLK is 1MHZ or a multiple of 2. 
	Min F_CLK = 1MHZ (TO BE TESTED WITH 128kHz)
	Max F_CLK = 20MHZ
	
	2) Can use timer overflow flag to emulate it as a 17 bit timer
	
	3) Time domain t: t E Z, {0 <= t <= 65535}
		
	4) Possible for the timer to be inaccurate if other non-interruptible interrupts are being used.
	
	5) The internal crystal oscillator has some pretty intense clock jitter. Its frequency also has an error of +10% Oscillator must be calibrated. 
	
	EDGE CASES:
	
	1) Timer ISR overhead: Tested with F_CLK = 8MHz: The logic in the timer ISR takes 20us to be executed, Compiled with -O2. 
	   When compiled with -O0, the ISR overhead is 30us.
		
	POTENTIAL FIXES:
	
	1) Try to further optimize the ISR	
	
*/

/* MEASUREMENTS:
	~5us @ 16MHZ, no conditionals for scaled ticks > 1024 -O0
	~4us @ 16MHZ, no conditionals for scaled ticks > 1024 -03
*/

int main(void)
{
	
	sei();
	
	DDRB |= (1 << DDB0);
	DDRB |= (1 << DDB1);
	
	bool Timer_Config_Success = Configure_Timer_Tick(1, u_MicroSeconds);
	
	if(Timer_Config_Success == false){ // Error state (Redundant as of now, as Timer_Config_Success will only return true. Will be changed.)
		//PORTB |= (1<<PORTB0);
	}

	while (1){
	}
	
	return 0;
}

/*
avrdude -c usbtiny -p m328 -U ?:?:"C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328\Debug\SuperCapBike-Firmware-ATMEGA328.hex":?
*/

/*
cd C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328
*/