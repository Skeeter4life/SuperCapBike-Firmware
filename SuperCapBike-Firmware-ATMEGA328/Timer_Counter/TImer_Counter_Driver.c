/*
 * Timer_Counter_Driver.c
 *
 * Author : Andrew
 */ 

// Note/Reminder: The constexpr keyword was not added until the C23 standard
// Some may find my commenting superfluous, but I just want to make sure I am being clear... :)

#include "../Headers/Includes.h"
#include "../Headers/Timer_Counter.h"

// Interesting note here: TC_CLK = F_CLK will not compile.

//------- Units (u_):

const uint32_t u_MicroSeconds = 1000000; // u_ defines a unit
const uint16_t u_MiliSeconds = 1000;
const uint8_t u_Seconds = 1;

volatile uint64_t System_Ticks[3] = {0, 0, 0}; // Each tick is defined by Configure_Timer
volatile uint32_t Calculated_Ticks[3] = {0, 0, 0};
volatile uint32_t Remaining_Ticks[3] = {0, 0, 0};
	
Timer_Modes Modes[3] = {TIMER_NONE, TIMER_NONE, TIMER_NONE};
	
Timers Selected_Timer = TIMER_NONE;

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
			
			Remaining_Ticks[_8_bit2] = Calculated_Ticks[_8_bit2]; 
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

static Timer_Status Set_Prescaler(Timers Timer, uint16_t Prescaler){
	
	switch(Timer){
		
		case _16_bit:
		
			switch(Prescaler){ // Don't need to clear TCCRxB, as it will be assigned:
				
				case 1:
				
				TCCR1B |= (1 << CS10);
				break;
				
				case 8:
				
				TCCR1B |= (1 << CS11);
				break;
				
				case 64:
				
				TCCR1B |= (1 << CS11) | (1 << CS10);
				break;
				
				case 256:
				
				TCCR1B |= (1 << CS12);
				break;
				
				case 1024:
				
				TCCR1B |= (1 << CS12) | (1 << CS10);
				break;
				
			}
			
			break;
		
		case _8_bit1:
		
			switch(Prescaler){
				
				case 1:
				
				TCCR0B |= (1 << CS00);
				break;
				
				case 8:
				
				TCCR0B |= (1 << CS01);
				break;
				
				case 64:
				
				TCCR0B |= (1 << CS01) | (1 << CS00);
				break;
				
				case 256:
				
				TCCR0B |= (1 << CS02);
				break;
				
				case 1024:
				
				TCCR0B |= (1 << CS02) | (1 << CS00);
				break;
				
			}
			
			break;
		
		case _8_bit2:
		
			switch(Prescaler){
				
				case 1:
				
				TCCR2B |= (1 << CS20);
				break;
				
				case 8:
				
				TCCR2B |= (1 << CS21);
				break;
				
				case 64:
				
				TCCR2B |= (1 << CS22);
				break;
				
				case 256:
				
				TCCR2B |= (1 << CS22) |  (1<<CS21);
				break;
				
				case 1024:
				
				TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);
				break;
				
			}
			
			break;
		
		default:
		
			return TIMER_FAULT;

	}
	
	return TIMER_OK;
	
}


Timer_Status Configure_Timer(uint16_t Time, uint32_t Unit, Timers Selected_Timer){ // All relevent types were optimized by calculating the largest possible values to Configure_Timer_Step()
	
	if(TC_CLK == 0){ 
		return TIMER_FAULT;
	}
	
	if(Unit == 0){
		return TIMER_FAULT;
	}
	
	if((uint64_t)TC_CLK * Time/Unit <= Max_ISR_Cycles){ // If the requested tick is shorter than or equal to the max time it takes to increment System_Ticks, return error state
		return TIMER_FAULT;
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
	uint64_t Scaled_Ticks = Numerator / Unit; // How many times we have to count for the requested time to have passed at the current clock frequency
	
	uint16_t Prescaler = 0;
	uint32_t Calculated_Prescaler = 0; // Ensures that OCRxA is <= (2^n - 1)
	
	switch(Selected_Timer){
		
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
		
			return TIMER_FAULT;
		
	}


	if(Calculated_Prescaler > 1024){  // The required count will overflow the selected timer, even with the largest available prescaler

		Prescaler = 1024;
		
		//Adjusted_Cycles = (((TC_CLK + (F_CLK*Prescaler/2))/F_CLK*Prescaler) * Avg_ISR_Cycles); // I may revisit this. Quite challenging to get right, and even harder to get consistently right
		
		Calculated_Ticks[Selected_Timer] = (Scaled_Ticks >> 10);
		
		Remaining_Ticks[Selected_Timer] = Calculated_Ticks[Selected_Timer];
		
		switch(Selected_Timer){
			
			case _16_bit:
				
				TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12); // Set prescaler to 1024, CTC mode (TCCR = Timer counter control register)
								
				OCR1AH = 0xFF;
				OCR1AL = 0xFF; // Timer begins
				
				TIMSK1 = (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enabled interrupt for progrm at TIMER1_COMPA_vect to be executed on compare match
				break;
			
			case _8_bit1:
				
				TCCR0A = (1 << WGM01);
				TCCR0B = (1 << CS02) | (1 << CS00);
				
				TIMSK0 = (1 << OCIE0A);
				
				OCR0A = 0xFF;
				break;
			
			case _8_bit2:

				TCCR2A = (1 << WGM21);
				TCCR2B = (1 << CS22) | (1 << CS21) | (1 << CS20);

				TIMSK2 = (1 << OCIE2A);
	
				OCR2A = 0xFF;
				break;
			
		}
		
		return TIMER_OK;
		
	}else{
		
		Calculated_Ticks[Selected_Timer] = 0;
		
		uint16_t Clock_Dividers[5] = {1, 8, 64, 256, 1024}; 
		
		for(uint8_t i = 0; i <= 4; i++){  // Logic to ensure that the Raw_Count <= uint16_t
			
			if(Clock_Dividers[i] >= Calculated_Prescaler){
				Prescaler = Clock_Dividers[i];
				break;
			}
			
		}
	}
	
	uint32_t Denominator =  Prescaler * Unit;

	if(Denominator == 0) return TIMER_FAULT;
	
	//Adjusted_Cycles = (((TC_CLK + (F_CLK*Prescaler/2))/F_CLK*Prescaler) * Avg_ISR_Cycles); 
	
	// Rounding integer division (A new trick I learned) reduces error of Timer_Top ideally to +- 0.5:
	
	uint32_t Timer_Top = ((Numerator + (Denominator/2)) / Denominator); 
	
	if (Timer_Top == 0) return TIMER_FAULT;

	if( (Selected_Timer == _8_bit1 || Selected_Timer == _8_bit2) && Timer_Top > 255 ){
		
		return TIMER_FAULT; 
		
	}else if(Selected_Timer == _16_bit && Timer_Top > 65535){
		
		return TIMER_FAULT;
		
	}
	
	Timer_Status Status = Set_Prescaler(Selected_Timer, Prescaler);
	
	if(Status == TIMER_FAULT){
		return TIMER_FAULT;
	}
	
	switch(Selected_Timer){
		
		case _16_bit:
				
			Modes[_16_bit] = TIMER_CTC;
				
			TCCR1B |= (1 << WGM12); 
			
			OCR1AH = (Timer_Top >> 8) & 0xFF;
			TIMSK1 = (1 << OCIE1A); // Timer/Counter1 Interrupt Mask Register -> Enabled interrupt for progrm at TIMER1_COMPA_vect to be executed on compare match
			OCR1AL = (Timer_Top & 0xFF); // Timer begins
			
				
			break;
			
		case _8_bit1:
			
			Modes[_8_bit1] = TIMER_CTC;

			TCCR0A |= (1 << WGM01); 	
			
			TIMSK0 = (1 << OCIE0A);
			OCR0A = Timer_Top;		
		
			break;
			
		case _8_bit2:
			
			Modes[_8_bit2] = TIMER_CTC;

			TCCR2A |= (1 << WGM21);		
			
			TIMSK2 = (1 << OCIE2A);
			OCR2A = Timer_Top; 
		
			break;
				
	}
	
	return TIMER_OK;

}

// f_PWM = f_clk/N*256

static void Reset_Timer_If_CTC() {
	
	switch(Selected_Timer) {
		
		case _16_bit:
		
			if(Modes[_16_bit] == TIMER_CTC){
				
				TIMSK1 = 0; // Disable all timer interrupts
				TCCR1A = 0;
				TCCR1B = 0;
				
			}
			
			break;
			
		case _8_bit1:
		
			if(Modes[_8_bit1] == TIMER_CTC){
				
				TIMSK0 = 0;
				TCCR0A = 0;
				TCCR0B = 0;
				
			}
			
			break;
			
		case _8_bit2:
		
			if(Modes[_8_bit2] == TIMER_CTC){
				
				TIMSK2 = 0;
				TCCR2A = 0;
				TCCR2B = 0;
				
			}
			
			break;
	}
	
}

Timer_Status Init_PWM(PWM_Setup* PWM) { // Hardware is incapable of variable freq. variable duty %, except for 16 bit timer.
	
	switch(PWM->Pin){ // I repeat myself seemingly a fair bit here, but the only way the timer is known, without having to pass the timer, is by knowing which pin it is.
		
		case PB1_OC1A:
			
			Selected_Timer = _16_bit;
			
			Reset_Timer_If_CTC();
			
			TCCR1A |= (1 << COM1A1); // Non inverting phase correct PWM
		
			break;
		
		case PB2_OC1B:
			
			Selected_Timer = _16_bit;
			
			Reset_Timer_If_CTC();
			
			TCCR1A |= (1 << COM1B1);
			
			break;
		
		case PD5_OC0B:
			
			Selected_Timer = _8_bit1;
			
			Reset_Timer_If_CTC();
		
			TCCR0A |= (1 << COM0B1);
			
			break;
		
		case PD6_OC0A:
		
			Selected_Timer = _8_bit1;
			
			Reset_Timer_If_CTC();
				
			TCCR0A |= (1 << COM0A1);
			
			break;
		
		case PB3_OC2A:
			
			Selected_Timer = _8_bit2;
			
			Reset_Timer_If_CTC();
		
			TCCR2A |= (1 << COM2A1);
			
			break;
		
		case PD3_OC2B:
		
			Selected_Timer = _8_bit2;
			
			Reset_Timer_If_CTC();
					
			TCCR2A |= (1 << COM2B1);
			
			break;
		
		default:
		
			return TIMER_FAULT;
		
	}

	switch(Selected_Timer) {
		
		case _16_bit:	
			
			// PWM Mode 10: Phase correct, variable freq & duty cycle
			TCCR1A |= (1 << WGM11);
			//TCCR1B |= (1 << WGM13);
			
			ICR1H = (uint8_t)(PWM->ICR >> 8); // A new style of doing this. Is this more clear? I think so.
			ICR1L = (uint8_t)(PWM->ICR);
			
			Modes[_16_bit] = TIMER_PWM;
			
			break;
		
		case  _8_bit1:
				
			TCCR0A |= (1 << WGM00); // Phase correct PWM
			//TCCR0B |= (1 << WGM02); 
			
			Modes[_8_bit1] = TIMER_PWM;
		
			break;
		
		case _8_bit2:
			
			TCCR2A |= (1 << WGM20); // Phase correct PWM
			//TCCR2B |= (1 << WGM22);
	
			Modes[_8_bit2] = TIMER_PWM;
			
			break;
		
		default:
		
			return TIMER_FAULT;
	
	}
	
	return TIMER_OK;
	
}

Timer_Status Configure_PWM(PWM_Setup* PWM, uint16_t Prescaler, uint8_t Duty_Cycle){
	
	if(Duty_Cycle > 100) {
		return TIMER_FAULT;
	}
	
	switch(PWM->Pin){
		
		case PB1_OC1A:
		
			OCR1A = (Duty_Cycle * 65535 + 50) / 100;
			Selected_Timer = _16_bit;
		
			break;
		
		case PB2_OC1B:
		
			OCR1B = (Duty_Cycle * 65535 + 50) / 100;
			Selected_Timer = _16_bit;
		
			break;
		
		case PD5_OC0B:
		
			OCR0B = (Duty_Cycle * 255 + 50) / 100;
			Selected_Timer = _8_bit1;
		
			break;
		
		case PD6_OC0A:
		
			OCR0A = (Duty_Cycle * 255 + 50) / 100;
			Selected_Timer = _8_bit1;
		
			break;
		
		case PB3_OC2A:
		
			OCR2A = (Duty_Cycle * 255 + 50) / 100;
			Selected_Timer = _8_bit2;
		
			break;
		
		case PD3_OC2B:
		
			OCR2B = (Duty_Cycle * 255 + 50) / 100;
			Selected_Timer = _8_bit2;
		
			break;
		
		default:
		
			return TIMER_FAULT;
		
	}
	
	Timer_Status Status = Set_Prescaler(Selected_Timer, Prescaler);
	
	if(Status == TIMER_FAULT){
		return TIMER_FAULT;
	}
	
	return TIMER_OK;
	
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

/* CONFIGURE & INIT PWM FUNCTIONS:

	NOTES:
	
	1) When switching between a timer for CTC or PWM, the timer mode will automatically be updated
	2) Only with the 16 bit timer can PWM frequency and duty cycle be modified (Due the the hardware). Only duty cycle can be changed with the 8 bit timer.
	3) The PWM is restricted to phase correct mode. I may add functionality for both, to make it a true driver, but for my case (driving a motor) phase correct is optimal.
	
	EDGE CASES:
	
	POTENTIAL FIXES:
	
*/

/* MEASUREMENTS:
	~5us @ 16MHZ, no conditionals for scaled ticks > 1024 -O0
	~4us @ 16MHZ, no conditionals for scaled ticks > 1024 -03
*/
