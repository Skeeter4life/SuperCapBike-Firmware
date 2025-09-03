
/*
 * EEPROM_Driver.S
 *
 *  Author: Andrew
 */ 


 .global EEPROM_Read
 .global EEPROM_Write

 .extern EEPROM_enabled

 #define Address_H r25
 #define Address_L r24
 #define EEPROM_Data r22

 #define EECR 0x1F
 #define EEDR 0x20
 #define EEARL 0x21
 #define EEARH 0x22

 #define SPMCSR 0x37

 #define ATMEGA328_Address_Mask 0b11111100

 ; The arguments passed as put into those registers per the AVR-GCC definitions

 ;EEPROM Address Registers, EEPROM Data Register, EEPROM control register
 ;Addressable in I/O Space

 EEPROM_Ready_ERROR:
	
	; Pop EEPROM_READ / EEPROM_WRITE return address

	POP r18
	POP r19 

	; Note: R18-R27, R30, R31 are call clobbered (no need to preserve their contents).

	LDI R24, 0xFF 	
	LDI R25, 0xFF ; Returns -1

	RET


Check_Address:

	LDI r18, ATMEGA328_Address_Mask ; Requested address exceeds EEPROM address space

	AND r18, Address_H

	CPI r18, 0

	BRNE EEPROM_Ready_ERROR

	RET

EEPROM_Wait:

	IN r18, EECR
	SBRC r18, 1
	RJMP EEPROM_Wait
	RET 

Check_EEPROM_State:

	LDS r19, EEPROM_Enabled

	CPI r19, 0

	BREQ EEPROM_Ready_ERROR

	SBIC EECR, 1 ; Ensure EEPROM is not being written to
	
	RJMP EEPROM_Ready_ERROR

	IN r18, SPMCSR

	SBRC r18, 0 ; Ensure flash is not being written to by the bootloader (not relevent but mind as well for the future)

	RJMP EEPROM_Ready_ERROR

	RJMP Check_Address

EEPROM_Read:
	
	RCALL Check_EEPROM_State

	; valid EEPROM read request

	OUT EEARH, Address_H

	OUT EEARL, Address_L

	; SBI EECR, 3 ; Enable EEPROM interrupts

	CLI ; Disable global interrupt bit

	SBI EECR, 0 ; Set read bit

	; Reads the EEPROM...

	IN EEPROM_Data, EEDR

	SEI ; Enable interrupts

	MOV r24, EEPROM_Data 
	
	ldi r25, 0 ; return EEDR, EEPROM read successful

	RET
			
EEPROM_Write:

	RCALL Check_EEPROM_State

	; valid EEPROM write request

	OUT EEARH, Address_H

	OUT EEARL, Address_L

	OUT EEDR, EEPROM_Data

	; SBI EECR, 3 ; Enable EEPROM interrupts
	
	CLI 

	SBI EECR, 2; Set EEPROM master write enable

	SBI EECR, 1 ; Write to EEPROM
	
	; Writes to the EEPROM...

	SEI 

	RCALL EEPROM_Wait

	ldi r25, 1 ; return 1, EEPROM write successful

	RET

	;"https://gcc.gnu.org/wiki/avr-gcc#Frame_Layout"

	

