/*
 * Motor_Driver.h
 *
 *  Author: Andrew
 */ 


#ifndef MOTOR_DRIVER_H_
#define MOTOR_DRIVER_H_

typedef enum Motor_Phases{
	Phase_A = 1,
	Phase_B = 2,
	Phase_C = 3,
	Phase_Invalid = 0
}Motor_Phases;

typedef struct Phase_Logic{
	Motor_Phases High;
	Motor_Phases Low;
	Motor_Phases High_Z;
}Phase_Logic;

typedef enum Motor_Status{
	
	Motor_OK,
	Motor_FAULT
	
}Motor_Status;

typedef enum {
	Parallel = 0,
	Series = 1
} Accumulator_State;

extern volatile uint8_t Current_Speed;
extern Motor_Status Init_Motor();
extern volatile bool Motor_Enabled;


#endif /* MOTOR_DRIVER_H_ */