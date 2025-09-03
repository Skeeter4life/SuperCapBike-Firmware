/*
 * Motor_Driver.c
 *
 *  Author: Andrew
 */ 


// Phase angle: 120 degrees

#include "../Headers/Includes.h"
#include "../Headers/I2C.h"
#include "../Headers/Motor_Driver.h"
#include "../Headers/Timer_Counter.h"

volatile uint8_t Current_Speed = 0;
volatile Accumulator_State Switching_State = Parallel;

volatile uint8_t Hall_State = 0;
volatile TWI_Status Status;

volatile bool Motor_Enabled = true; // Some things are volatile because I may use them in an ISR and would rather not forget to make it volatile. Ill optimize once everything works.

PWM_Setup IN_1;
PWM_Setup IN_2;
PWM_Setup IN_3; // Could just use the pins locally for each function, but this makes things a bit more cohesive 

void Update_Commutation(void);

const Phase_Logic Commutation_LUT[8] = {
	
	[0b000] = {Phase_Invalid, Phase_Invalid, Phase_Invalid},
	[0b001] = {Phase_A, Phase_B, Phase_C}, // Hall effect sensors on GPB3,4,5
	[0b101] = {Phase_A, Phase_C, Phase_B},
	[0b100] = {Phase_B, Phase_C, Phase_A},
	[0b110] = {Phase_B, Phase_A, Phase_C},
	[0b010] = {Phase_C, Phase_A, Phase_B},
	[0b011] = {Phase_C, Phase_B, Phase_A},
	[0b111] = {Phase_Invalid, Phase_Invalid, Phase_Invalid}
	
};

ISR(INT0_vect){
	
	if(Next_I2C_State == TWI_IDLE){
		Status = TWI_Read(MCP23017_Address, 0x11, &Hall_State, &Update_Commutation); // Clear interrupt flag (read INTCAPB register)
	}else{
		Status = TWI_FAULT;
	}	
}

void Update_Commutation(void){
	
	if(!Motor_Enabled) return;
	
	if(Status == TWI_FAULT){
		
		Error_Log Motor_Error = {
			
			.Message = "MOTOR",
			.Time = Global_Timer
			
		};
		
		Log_Error(&Motor_Error);
		
		EICRA &= ~(1 << ISC01); // Disable INT0
		Motor_Enabled = false;
		
	}
		
	; // Error state -> Will add error flag and error handling
		
	// ^ Yeah this really slows things down. Design mistake.
	
	uint8_t Phase_Index = (Hall_State>>3) & 0b111;
	
	if(Phase_Index == 7 || Phase_Index == 0){
		return; // Error state
	}
			
	const Phase_Logic* Current_Phase = &Commutation_LUT[Phase_Index]; // Not checking for out of bounds. Mask GPB5-3.
	
	PORTD &= ~((1 << PORTD0) | (1 << PORTD1) | (1 << PORTD4)); // Ensure all IR2014's are in shutdown state
	
	switch(Current_Phase->High){
		
		case Phase_A:
		
			DDRD |= (1 << DDD6); // IN_1 as output
			Toggle_PWM(&IN_1, ON);
			
			break;
		
		case Phase_B:
			
			DDRD |= (1 << DDD5); // IN2
			Toggle_PWM(&IN_2, ON);
			
			break;
			
		case Phase_C:
			
			DDRB |= (1 << DDB1); // IN3
			Toggle_PWM(&IN_3, ON);
			
			break;
			
		default:
		
			return; // Error state
			
	}
	
	switch(Current_Phase->Low){
		
		case Phase_A:
		
			Toggle_PWM(&IN_1, OFF); // Note: if the COMnX bit(s) are set, the functionality of the PORTx register is overridden
			
			DDRD |= (1 << DDD6); // Set output, digital LOW:
			
			PORTD &= ~(1 << PORTD6); // IN_1
		
			break;
		
		case Phase_B:
		
			Toggle_PWM(&IN_2, OFF);
			
			DDRD |= (1 << DDD5);
			
			PORTD &= ~(1 << PORTD5); // IN_2
		
			break;
		
		case Phase_C:
		
			Toggle_PWM(&IN_3, OFF);
			
			DDRB |= (1 << DDB1);
			
			PORTB &= ~(1 << PORTB1); // IN_3
		
			break;
		
		default:
		
			return; // Error state
		
	}
	
	switch(Current_Phase->High_Z){ 
		
		case Phase_A: // Shutdown U1 IR2104
			
			PORTD &= ~(1 << PORTD0); //D0: U1
			
			PORTD |= (1 << PORTD1) | (1 << PORTD4); // D1: U3, D4: U2
			
			break;
			
		case Phase_B: // Shutdown U2 
			
			PORTD &= ~(1 << PORTD4); 
			
			PORTD |= (1 << PORTD1) | (1 << PORTD0);
			
			break;
				
		case Phase_C: // Shutdown U3
			
			PORTD &= ~(1 << PORTD1); 
			
			PORTD |= (1 << PORTD0) | (1 << PORTD4);
			
			break;
			
		default:
		
			return; // Error state
			
	}
	
	
	return;
	
}

Motor_Status Init_Motor(void){
	
	DDRB |= (1 << DDB4) | (1 << DDB1); // Set MOS_DRIVER, IN_3 as a outputs	
	DDRD |= (1 << DDD7) | (1 <<DDD5) | (1 << DDD6) | (1 << DDD0) | (1 << DDD1) | (1 << DDD4); // Set RELAYS, IN_2, IN_1 , SD_U1, SD_U3, SD_U2 as outputs
	
	PORTD &= ~((1 << PORTD0) | (1 << PORTD1) | (1 << PORTD4)); // Ensure all IR2014's are in shutdown state
	
	DDRD &= ~((1 << DDD5) | (1 << DDD6));
	DDRB &= ~(1<<DDB1); // Set IN_1, IN_2 and IN_3 as High-Z
	
	TWI_Write(MCP23017_Address, 0x01, 0xFF);         // IODIRB: Set all Port B to inputs
	while(Next_I2C_State != TWI_IDLE);

	TWI_Write(MCP23017_Address, 0x0B, 0b00000010);   // IOCON: INTB is active-high, push-pull
	while(Next_I2C_State != TWI_IDLE);
	
	TWI_Write(MCP23017_Address, 0x0B, 0b00000100); // Active-low, open-drain
	while(Next_I2C_State != TWI_IDLE);

	TWI_Write(MCP23017_Address, 0x07, 0b00000010);   // DEFVALB: Set compare value for GPIOB1 to HIGH
	while(Next_I2C_State != TWI_IDLE);

	TWI_Write(MCP23017_Address, 0x09, 0b00000010);   // INTCONB: Compare GPIOB1 against DEFVALB
	while(Next_I2C_State != TWI_IDLE);

	uint8_t dummy_read;
	TWI_Read(MCP23017_Address, 0x11, &dummy_read, NULL);  // Read INTCAPB to clear interrupt flag
	while(Next_I2C_State != TWI_IDLE);
	
	TWI_Write(MCP23017_Address, 0x05, 0b00111000); 
	while(Next_I2C_State != TWI_IDLE);

	IN_1.Pin = PD6_OC0A;
	
	Timer_Status Timer1_Set = Toggle_PWM(&IN_1, OFF); // Just to make sure the pin can be toggled, and to have the pin initalized in a safe state
	
	Timer_Status Timer1_Config = Configure_PWM(&IN_1, 1, 10); // Testing with 10% duty cycle
	
	if(Timer1_Set == TIMER_FAULT || Timer1_Config == TIMER_FAULT) return Motor_FAULT;
	
	IN_2.Pin = PD5_OC0B;
	
	Timer_Status Timer2_Set = Toggle_PWM(&IN_2, OFF);
	
	Timer_Status Timer2_Config = Configure_PWM(&IN_2, 1, 10);

	if(Timer2_Set == TIMER_FAULT || Timer2_Config == TIMER_FAULT) return Motor_FAULT;
	
	IN_3.Pin = PB1_OC1A;
	
	Timer_Status Timer3_Set = Toggle_PWM(&IN_3, OFF);
	
	Timer_Status Timer3_Config = Configure_PWM(&IN_3, 1, 10);
	
	if(Timer3_Set == TIMER_FAULT || Timer3_Config == TIMER_FAULT) return Motor_FAULT;
	
	sei(); // Ensure interrupts are enabled
		
	EICRA |= (1 << ISC01); // The falling edge of INT0 generates an interrupt
	EIMSK |= (1 << INT0);
	
	return Motor_OK;
	
}

Motor_Status Switch_Accumulator_State(void){
	
	Switching_State = !Switching_State;
	
	PORTD &= ~((1 << PORTD7));
	PORTB &= ~((1 << PORTB4));
	
	uint64_t Current_Tick = System_Ticks[Global_Timer];
	
	if(Timer_Unit[Global_Timer] != u_MiliSeconds || Timer_Step[Global_Timer] != 1 || Timer_Mode[Global_Timer] != TIMER_CTC) return Motor_FAULT;
	while(System_Ticks[Global_Timer] < (Current_Tick + 12)); // 10 ms relay operating time (allow for 30% error)
	
	// Accumulator disconnected
	
	switch(Switching_State){
		
		case Parallel:
		
			PORTD |= (1 << PORTD7);  // Set RELAY signal high
			
			break;
		
		case Series:
		
			PORTB |= (1 << PORTB4); // Set MOS_DRIVER signal high
			
			break;
		
		default:
		
			return Motor_FAULT;
		
	}
	
	return Motor_OK;
	
}