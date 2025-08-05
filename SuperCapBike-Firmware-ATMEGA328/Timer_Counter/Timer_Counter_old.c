/*
 * 16Bit_Timer_Counter.c
 *
 * Author : Andrew
 */ 

// Note/Reminder: The constexpr keyword was not added until the C23 standard
// Some may find my commenting superfluous, but I just want to make sure I am being clear... :)

//------- Includes:
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

//------- Bool Definition:

#define bool uint8_t
#define true 1
#define false 0

//------- Timer/Counter Clock:

const uint32_t F_CLK = 16000000;
const uint32_t TC_CLK = 16000000; // TC_CLK can be asynchronous to F_CLK. 

// Interesting note here: TC_CLK = F_CLK will not compile.

//------- Units (u_):

const uint32_t u_MicroSeconds = 1000000; // u_ defines a unit 
const uint16_t u_MiliSeconds = 1000;
const uint8_t u_Seconds = 1;

//------- Timer Globals:

enum Timers{
	_16_bit = 0,
	_8_bit1 = 1,
	_8_bit2 = 2
};

volatile uint64_t System_Ticks[3] = {0, 0, 0}; // Each tick is defined by Configure_Timer
volatile uint32_t Calculated_Ticks[3] = {0, 0, 0};
volatile uint32_t Remaining_Ticks[3] = {0, 0, 0};

//------- Timer Definitions:

const uint8_t Max_ISR_Cycles = 150; // Max time the ISR will take to increment System_Ticks.

ISR(TIMER0_COMPA_vect){
		
	if(Remaining_Ticks[_8_bit1] == 0){
		
		PORTD ^= (1 << PORTD6);
				
		if(Calculated_Ticks[_8_bit1] > 0){
			
			Remaining_Ticks[_8_bit1] = Calculated_Ticks[_8_bit1]; // Reset the counter
			OCR0A = 0xFF;
			
		}
		
	}else{

		uint8_t NextOCR = (Remaining_Ticks[_8_bit1] > 0xFF) ? 0xFF : (uint8_t)Remaining_Ticks[_8_bit1];

		OCR0A = NextOCR;
		Remaining_Ticks[_8_bit1] -= NextOCR;
		
	}
	
}

ISR(TIMER2_COMPA_vect){
	
	if(Remaining_Ticks[_8_bit2] == 0){
		
		PORTD ^= (1 << PORTD7);
		
		System_Ticks[_8_bit2]++; 

		if(Calculated_Ticks[_8_bit2] > 0){
			
			Remaining_Ticks[_8_bit2] = Calculated_Ticks[_8_bit2]; // Reset the counter
			OCR2A = 0xFF;
			
		}
		
	}else{

		uint8_t NextOCR = (Remaining_Ticks[_8_bit2] > 0xFF) ? 0xFF : (uint8_t)Remaining_Ticks[_8_bit2];

		OCR2A = NextOCR;
		Remaining_Ticks[_8_bit2] -= NextOCR;
		
	}

}


ISR(TIMER1_COMPA_vect){
	
	if(Remaining_Ticks[_16_bit] == 0){
		
		PORTB ^= (1 << PORTB0);
		
		System_Ticks[_16_bit]++;
		
		if(Calculated_Ticks[_16_bit] > 0){
			
			Remaining_Ticks[_16_bit] = Calculated_Ticks[_16_bit];
			OCR1AH = 0xFF;
			OCR1AL = 0xFF;
			
		}
		
	}else{
		
		uint16_t NextOCR = (Remaining_Ticks[_16_bit] > 0xFFFF) ? 0xFFFF : (uint16_t)Remaining_Ticks[_16_bit];
		
		OCR1A = NextOCR;
		Remaining_Ticks[_16_bit] -= NextOCR;
		
	}
	
}


bool Configure_Timer_Tick(uint16_t Time, uint32_t Unit, uint8_t Timer){ // All relevent types were optimized by calculating the largest possible values to Configure_Timer_Step()
	
	if(TC_CLK == 0){ 
		return false;
	}
	
	if(Unit == 0){
		return false;
	}
	
	if((uint64_t)TC_CLK * Time/Unit <= Max_ISR_Cycles){ // If the requested tick is shorter than or equal to the max time it takes to increment System_Ticks, return error state
		return false;
	}
	
	//uint32_t Adjusted_Cycles = 0; // Won't overflow even with a 1Hz clock

	/*
	if(F_CLK >= TC_CLK){
		// Having this opens the door to the question: Consistency or precision?
		Adjusted_Cycles = ((F_CLK + (TC_CLK/2))/TC_CLK) * Avg_ISR_Cycles;
	}else{
		Adjusted_Cycles = ((TC_CLK + (F_CLK/2))/F_CLK) * Avg_ISR_Cycles;
	} */

	// Disable the timer interrupts as the timer is being re-configured:

	uint64_t Numerator = Time * TC_CLK;
	uint64_t Scaled_Ticks = Numerator / (Unit); // How many times we have to count for the requested time to have passed at the current clock frequency
	
	uint16_t Prescaler = 0;
	uint32_t Calculated_Prescaler = 0; // Ensures that OCRxA is <= (2^n - 1)
	
	switch(Timer){
		
		case _16_bit:
			
			TIMSK1 = TIMSK1 & ~(1 << OCIE1A); // Disable the timer interrupt as it is being reconfigured.

			Calculated_Prescaler = (Scaled_Ticks+65534)/65535; // Ceiling function

			break;
		
		case _8_bit1:
			
			TIMSK0 = TIMSK0 & ~(1 << OCIE0A);
			
			Calculated_Prescaler = (Scaled_Ticks+254)/255;
			
			break;
			
		case _8_bit2:
			
			TIMSK2 = TIMSK2 & ~(1 << OCIE2A);
			
			Calculated_Prescaler = (Scaled_Ticks+254)/255;
		
			break;
			
		default:
		
			return false;
		
	}


	if(Calculated_Prescaler > 1024){  // The required count will overflow the selected timer, even with the largest available prescaler

		Prescaler = 1024;
		//Adjusted_Cycles = (((TC_CLK + (F_CLK*Prescaler/2))/F_CLK*Prescaler) * Avg_ISR_Cycles); // I may revisit this. Quite challenging to get right, and even harder to get consistently right

		Calculated_Ticks[Timer] = (Scaled_Ticks >> 10);
		Remaining_Ticks[Timer] = Calculated_Ticks[Timer];
		
		switch(Timer){
			
			case _16_bit:
				
				TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12); // Set prescaler to 1024, CTC mode (TCCR = Timer counter control register)
								
				OCR1AH = 0xFF;
				OCR1AL = 0xFF; // Timer begins
				
				TIMSK1 |= (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enabled interrupt for progrm at TIMER1_COMPA_vect to be executed on compare match
				break;
			
			case _8_bit1:
				
				TCCR0A = (1 << WGM01);
				TCCR0B = (1 << CS02) | (1 << CS00);
				
				TIMSK0 |= (1 << OCIE0A);
				OCR0A = 0xFF;
				break;
			
			case _8_bit2:

				TCCR2A = (1 << WGM21);
				TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);

				TIMSK2 |= (1 << OCIE2A);
				
				OCR2A = 0xFF;
				break;
			
		}
		

		return true;
		
	}else{
		
		Calculated_Ticks[Timer] = 0;
		
		uint16_t Clock_Dividers[5] = {1, 8, 64, 256, 1024}; 
		
		for(uint8_t i = 0; i <= 4; i++){  // Logic to ensure that the Raw_Count <= uint16_t
			
			if(Clock_Dividers[i] >= Calculated_Prescaler){
				Prescaler = Clock_Dividers[i];
				break;
			}
			
		}
	}
	
	uint32_t Denominator =  Prescaler * Unit;

	if(Denominator == 0) return false; // Unexpected error
	
	//Adjusted_Cycles = (((TC_CLK + (F_CLK*Prescaler/2))/F_CLK*Prescaler) * Avg_ISR_Cycles); 
	
	// Rounding integer division (A new trick I learned) reduces error of Timer_Top ideally to +- 0.5:
	uint32_t Timer_Top = ((Numerator + (Denominator/2)) / Denominator); 
	
	if (Timer_Top == 0) return false; //Unexpected error

	if( (Timer == _8_bit1 || Timer == _8_bit2) && Timer_Top > 255 ){
		return false; // Unexpected error
	}else if(Timer == _16_bit && Timer_Top > 65535){
		return false;
	}
	
	switch(Timer){
		
		case _16_bit:
			
			switch(Prescaler){ // Don't need to clear TCCRxB, as it will be assigned:
					
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
				
			TCCR1B |= (1 << WGM12); 
			
			OCR1AH = (Timer_Top >> 8) & 0xFF;
			TIMSK1 |= (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enabled interrupt for progrm at TIMER1_COMPA_vect to be executed on compare match
			OCR1AL = (Timer_Top & 0xFF); // Timer begins
				
			break;
			
		case _8_bit1:
		
			switch(Prescaler){ 
				
				case 1:
				
					TCCR0B = (1 << CS00);
					break;
				
				case 8:
				
					TCCR0B = (1 << CS01);
					break;
				
				case 64:
				
					TCCR0B = (1 << CS01) | (1 << CS00);
					break;
				
				case 256:
					
					TCCR0B = (1 << CS02);
					break;
				
				case 1024:
				
					TCCR0B = (1 << CS02) | (1 << CS00);
					break;
				
			}
			

			TCCR0A |= (1 << WGM01); 	
						
			TIMSK0 |= (1 << OCIE0A);
			OCR0A = Timer_Top;		
		
			break;
			
		case _8_bit2:

			switch(Prescaler){
			
				case 1:
			
				TCCR2B = (1 << CS20);
				break;
			
				case 8:
			
				TCCR2B = (1 << CS21);
				break;
			
				case 64:
			
				TCCR2B = (1 << CS22);
				break;
			
				case 256:
			
				TCCR2B = (1 << CS22) |  (1<<CS21);
				break;
			
				case 1024:
			
				TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);
				break;
			
			}
			
			TCCR2A = (1 << WGM21);		
				
			TIMSK2 |= (1 << OCIE2A);
			OCR2A = Timer_Top; 
		
			break;
				
	}
	
	return true;

}

/* CONFIGURE TIMER FUNCTION

	NOTES:
	
	1) Works best when TC_CLK is 1MHZ or a multiple of 2. 
	The entire point of this function is to be able to have an accurate timer with a variable TC_CLK.
	
	Min TC_CLK = 1Hz
	Max TC_CLK = F_CLK
	
	2) Can modify the function to use timer overflow flags to emulate them as 17 or 6 bit timers. The hardware allows for it.
	
	3) Time domain t: t E Z, {0 <= t <= 65535}
			
	4) The internal crystal oscillator has some pretty intense clock jitter. Its frequency also has an error of +10% Oscillator must be calibrated. 
	
	5) Is this the most optimized, efficient method of setting up timers? No. Is it versatile and easy to use? I would say so! Is it accurate? Yes. 
	This was an excellent exercise to test my ability to write efficient code. 
	It has required me to understand exactly what each line is doing, and more importantly, how each part interacts with each other. 
	It deepened my understanding of the strengths and weaknesses of the AVR architecture.
	
	EDGE CASES:
	
	1) Timer ISR overhead: Tested with TC_CLK = 16MHz: 
	   When compiled with -O0 (No optimization): the ISR overhead is 8us. 
	   When compiled with Compiled with -O2 (Optimize More): the ISR overhead is 5us. 
	   When compiled with -03 (Optimize most): the ISR overhead is 5us.
	   
	2) ISR overlap. If multiple timers or being used, or some other ISR is being executed, the timer will be unpredictably inaccurate.
	
	3) The execution time of the ISR can be longer than the requested delay.
	
	4) If a timers delay is configured to the absolute minimum, it will effectively act as a blocking function.
	
	5) Timers would always be slightly out of sync of each other for multiple calls to the Configure_Timer_Tick() function.
		
	POTENTIAL FIXES:
	
	1) Try to further optimize the ISR.
	
	2) Be mindful of the issue.
	
	3) Added a check to see if it is possible to execute the ISR in the max number of clock cycles before the requested time has elapsed (~10us, as it depends where the ISR gets placed in memory). There will of course always be overhead and some jitter. 
	
	4) As stated in 3, I added a check to enforce the minimum, but I won't enforce any sort of recommended delay. It is something I (and others using it?) will have to be mindful of.
	
	5) Be mindful of the issue. I can completely refactor the function to accept multiple timer configuration requests to make it a bit faster, but I don't see it as a substantial issue.
	
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
	DDRD |= (1 << DDD7);
	DDRD |= (1 << DDD6);
	
	//bool Precisie_Mode = true;
	
	uint8_t Timer1 = _8_bit1;
	
	bool Timer1_Set = Configure_Timer_Tick(100, u_MiliSeconds, Timer1);
	
	uint8_t Timer2 = _8_bit2;
	
	bool Timer2_Set = Configure_Timer_Tick(10, u_MicroSeconds, Timer2);
	
	uint8_t Timer3 = _16_bit;
	
	bool Timer3_Set = Configure_Timer_Tick(3, u_Seconds, Timer3);
	
	if(!Timer1_Set || !Timer2_Set || !Timer3_Set){ 
		PORTB = (1 << PORTB1); 
	}

	while (1){
		
	}
	
	return 0;
}

/*
avrdude -c usbtiny -p m328 -U ?:?:"C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328\Debug\SuperCapBike-Firmware-ATMEGA328.hex":?
*/

// ^^ I use an ATMEL ICE, but I also have a usbtiny programmer.

/*
cd C:\Users\Andrew\Documents\SuperCap_Bike\SuperCapBike-Software\SuperCapBike-Firmware-ATMEGA328
*/