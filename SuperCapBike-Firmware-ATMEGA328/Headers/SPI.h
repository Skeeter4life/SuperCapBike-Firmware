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

extern int8_t SPI_Main_Init(uint8_t SPI_Prescaler, Data_Order Order);
extern int8_t SPI_Passive_Init();
extern int SPI_Transmit(uint8_t Data, Devices Device);


#endif /* SPI_H_ */