/*
 * SPI.h
 * 
 *  Author: Andrew
 */ 


#ifndef SPI_H_
#define SPI_H_

enum{
	Main,
	Passive
}SPI_Config;

typedef enum{
	LSB_First,
	MSB_First
}Data_Order;

typedef enum{
	Device1,
	Device2
}Devices;

typedef enum SPI_Status{
	SPI_FAULT = 0,
	SPI_OK
}SPI_Status;

extern SPI_Status SPI_Main_Init(uint8_t SPI_Prescaler, Data_Order Order);
extern SPI_Status SPI_Passive_Init();
extern SPI_Status SPI_Transmit(uint8_t Data, Devices Device);


#endif /* SPI_H_ */