/*
 * EEPROM.h
 *
 *  Author: Andrew
 */ 


#ifndef EEPROM_H_
#define EEPROM_H_

#include "Includes.h"

/*
enum {
	Read = 0,
	Write = 1
}EEPROM_Mode;
*/
extern volatile bool EEPROM_Ready;
extern volatile bool EEPROM_Enabled;
extern volatile uint16_t EEPROM_Address;

extern int16_t EEPROM_Read(uint16_t Address); // int16_t as -1 indicates that the operation failed. Of course need 8 unsigned bits to return the read address.
extern int16_t EEPROM_Write(uint16_t Address, uint8_t Data);

#endif /* EEPROM_H_ */