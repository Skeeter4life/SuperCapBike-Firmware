/*

Completely and utterly deprecated.

Just here to show how much better I have gotten...

(Build action set to: None)

*/


#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h> //To remove

#define F_CPU 16000000
#define SHIFT_AMOUNT 6 // I should experiment with this number
#define SHIFT_MASK ((1 << SHIFT_AMOUNT) - 1)

#define FALSE 0
#define TRUE 1 // To update later

// VOLATILE DEFN'S: Compiler assumes variables do not changed unless they are explicitly modified within the program; this is not the case here; Must always read from memory
volatile uint16_t ADC_DATA_0; // Throttle Data
volatile uint16_t ADC_DATA_1; // C1 Voltage
volatile uint16_t ADC_DATA_2; // VCC Voltage
volatile uint8_t ADC_Selection = 0; // 0 = ADC 0 1 = ADC 1 2 = ADC 2

// Note: I prefer returning '0' when a process has failed, and '1' when it succeeds.

struct Diagnostics {
  //--------- TWI
  uint8_t TWI_Register; // Cant pass struct in ISR
  uint8_t TWI_Data;
  uint8_t TWI_RW; // Read = 1 Write = 0

  volatile bool Register_Data_Sent = false;
  volatile bool Transmission_Complete = true; // No TWSR code for TWI STOP

  volatile uint8_t Read_Data = 0;
  
  // volatile bool Read_Register_Selected = false;
  
  uint8_t* End_of_Queue_Ptr;

  uint8_t* Queue_Ptr = NULL; // 16 bit address space

  volatile uint8_t Queue_Counter = 0;

  //--------- CAPACITOR_VOLTAGES
  uint8_t VC1; // Voltage of Capacitor 1
  uint8_t mVC1; // mV of Capacitor 1

  uint8_t VC2;  // Voltage of Capacitor 2
  uint8_t mVC2; // mV of Capacitor 2

  //--------- THROTTLE
  uint8_t Throttle_Position; // Range of {0 1, 2 ... 100} (% Applied)

  //--------- ERROR HANDLING
  char Fault_Code[16]; // n*sizeof(type)

  //--------- CAPACITOR_CONFIGURATIONS
  bool is_ShutDown;
  bool is_Series; // True if capacitors are in series False if in parallel
} Bike_Status;

void Compute_Voltages() { // Uses fixed point arithmetic

  uint32_t VCC_Equation;
  uint32_t mVCC_Equation;

  uint32_t VC1_Equation;
  uint32_t mVC1_Equation;

  VC1_Equation = (((ADC_DATA_1 << SHIFT_AMOUNT) << 4) / 1023);

  mVC1_Equation = (((VC1_Equation & SHIFT_MASK) * 100) / (1 << SHIFT_AMOUNT)); // When using this, this is STRICTLY the fractional part. (*100 -> Two decimal places)

  VC1_Equation = VC1_Equation >> SHIFT_AMOUNT; // Now, I can use VC1 Equation as I thought I was. OOPS.

  Bike_Status.VC1 = VC1_Equation;
  Bike_Status.mVC1 = mVC1_Equation;

  if (Bike_Status.is_Series) {

    VCC_Equation = (((ADC_DATA_2 << SHIFT_AMOUNT) * 391) / 12500);
    mVCC_Equation = ((VCC_Equation & SHIFT_MASK) * 100) / (1 << SHIFT_AMOUNT);

    VCC_Equation = VCC_Equation >> SHIFT_AMOUNT;

    if (mVCC_Equation > mVC1_Equation) { // Glad I caught this edge case!
      Bike_Status.VC2 = VCC_Equation - VC1_Equation;
      Bike_Status.mVC2 = mVCC_Equation - Bike_Status.mVC1;
    } else {
      Bike_Status.VC2 = VCC_Equation - VC1_Equation - 1;
      Bike_Status.mVC2 = mVCC_Equation + 100 - mVC1_Equation;
    }
  } else {
    VCC_Equation = (((ADC_DATA_2 << SHIFT_AMOUNT) << 4) / 1023);
    mVCC_Equation = ((VCC_Equation & SHIFT_MASK) * 100) / (1 << SHIFT_AMOUNT);

    Bike_Status.VC2 = VCC_Equation;
    Bike_Status.mVC2 = mVCC_Equation;
    
  }
}

void TWIFault() {
}

void Compute_Throttle() {
}

enum TWI_States {
  REPEATED_START,
  ADDRESSING,
  //  WAIT_ACK,
  MAIN_WRITE,
  MAIN_READ,
  STOP,
  FAILED,
} TWI_States;

/*
   The register address is always 7 bits long. The last bit can be R/W bit. Mask the last bit.
*/

void TWI_Queue_Handler(uint8_t Register_Address = 0xFF, uint8_t Data = 0, uint8_t RW = 0); // My queue works properly as a FIFO datastructure 


uint8_t TWI_MCP(uint8_t Register_Address, uint8_t Data, uint8_t RW) {

  Serial.println("TWI MCP Request Recieved");

  if (Bike_Status.Transmission_Complete) { // If there is currently a TWI transmission NOT in progress:

    Serial.println("Starting TWI");

    if (RW) { // If inbound is a READ request:

      Serial.println("Reading");

      Bike_Status.TWI_RW = 1;  //Read
      //Bike_Status.Read_Register_Selected = false; REDUNDENT -> Can tell if there has been a REPAETED START or not.

    } else { // Else, we are WRITING

      Serial.println("Writing");
      Bike_Status.TWI_RW = 0;  //Write
    
    }

    // Store inbound arguments in Bike Status, for debugging & scope reasons:
    
    Bike_Status.TWI_Register = Register_Address; 
    Bike_Status.TWI_Data = Data;
    
    Bike_Status.Transmission_Complete = false; // A transmission is currently IN progress:

    TWSR = 0x00; // TWI Clear the status register (A redundency)
    
    TWBR = 0; // Set TWBR to 12, TWI SCL frequency of 400kHz 0 = 1MHZ (Prescalar bits are 0 in the TWSR)
    
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA); // Clear the interrupt flag, enable TWI & TWI interrupts, Set start bit -> TWI START TRANSMITTED HERE.

    TWI_States = ADDRESSING; // Switch case to "ADDRESSING"

    Serial.print("TWI - Sending Start Bit\n");

    // free(Bike_Status.Queue_Ptr); // Should I really be calling free here??

  } else {
    //Serial.println(Register_Address);
    //Serial.println(Data);
    //Serial.println("HERE!");
    TWI_Queue_Handler(Register_Address, Data, RW); // TWI bus is currently BUSY. Add inbound to QUEUE.

    return;


  }
}

void TWI_Queue_Handler(uint8_t Register_Address = 0xFF, uint8_t Data = 0, uint8_t RW = 0) { //NOTE: This is a cheeky way to not have TEMP variables in Bike_Status. Register addressing never goes > 0x15 for the MCP23017. If there were actual other devices, I would not do this. Read = 1 Write = 0
  // Responsible for setting the relevant contents in Bike_Status
  /*
     NOTE: This works because the highest possible Register_Address is 0x1A
  */
  
  if (!Bike_Status.Transmission_Complete) { // This function will be called to queue something, AND to grab something FROM the queue. This case is we are ADDING to the queue. (This If statement is kind of redundent - need to double check)
    
     Bike_Status.Queue_Counter += 1; // Add one to the number of packets (each packet is 2 bytes, Register Address and Data) in the queue.
    
     Serial.print("Queue Counter: +1 ");
     Serial.println(Bike_Status.Queue_Counter);

     Serial.print("Shifting Register_Address ");
     
     Serial.println(Register_Address, BIN);

     if (Register_Address > (0x1A << 1)) { // 0x1A is the highest accessable register address of the MCP23017
      //Error state
      Serial.println("TWI Queue handler: Error state");
      return;
    }

     
     Register_Address <<= 1; // This is kind of a cheeky way of adding another peice of info without taking up another byte. 0x1A = 0b00011010, therefore we can always assume the first bit will never be a 1 (so it will never overflow).
     
     if(RW){ // I use that one bit of space I made to:
      Register_Address++; // Add ONE if we are READING.
     }
     
    Serial.println(Register_Address, BIN);

    Serial.println("Adding to Queue: ");

   
    // Serial.println("TWI request Queued");

    Bike_Status.Queue_Ptr = (uint8_t*)realloc(Bike_Status.Queue_Ptr, 2 * Bike_Status.Queue_Counter); // 2* because I am storing the Register_Adress and Data. Unknown final size, malloc() is not efficent here. This is a pointer to the start of the 1 byte packets. (2*Bike_Status.Queue_Counter works because I am working with 1 byte types anyways)

    if (Bike_Status.Queue_Ptr == NULL) { // This would typically happen if I run out of my 256 bytes of RAM. Unlikely, but an edge case nonetheless...
     // Some error routine, allocation failed
     Serial.println("Reallocation Failed!");
     //free(Bike_Status.Queue_Ptr);  // ERROR Routine
    }
    
    Serial.print("Address: 0x");

    uint16_t x = Bike_Status.Queue_Ptr; // Useful for debugging (16 bit address space)

    Serial.println(x, HEX); // Useful for debugging
    
    Bike_Status.End_of_Queue_Ptr = Bike_Status.Queue_Ptr + 2*(Bike_Status.Queue_Counter - 1) + 1; // I am storing packets of 2 bytes. This points to the last byte. NOTE: This pointer is NOT used in any other real logic down the line. I thought I might need it, but so far I don't. 

    uint16_t z = Bike_Status.End_of_Queue_Ptr; // Useful for debugging

    Serial.print("End Of Queue Ptr: 0x");
    Serial.println(z, HEX);

    // The allocated memory keeps growing, and it is contiguous.
    
    uint8_t Index_Equation = 2 * (Bike_Status.Queue_Counter - 1); // I store 2 bytes of data (My "packets") This simple equation is what defines how to access my packets

    Bike_Status.Queue_Ptr[Index_Equation] = Register_Address; 
    Bike_Status.Queue_Ptr[Index_Equation + 1]  = Data; 

    // A reminder to me: [] offsets from the pointer's base address by sizeof(T) byte(s).

    Serial.print("Queued Succesfully (Register_Addrsss, Data, RW): 0x");

    Serial.print(Bike_Status.Queue_Ptr[Index_Equation] >> 1, HEX);

    Serial.print(" 0b");

    Serial.print(Bike_Status.Queue_Ptr[Index_Equation + 1], BIN);

    Serial.print(" ");
    
    Serial.println(RW);

  } else if (Bike_Status.Transmission_Complete && Bike_Status.Queue_Counter > 0) { // If the latest TWI transmission is COMPLETE, and there are still packets in the Queue:

    //Serial.println("Loading and sending: ");
    
    uint8_t* Data_Address = (Bike_Status.Queue_Ptr + 1); // The orginization is as follows: Byte 1: Register_Address Byte 2: Data. Queue_Ptr points to the begining of the queue.

    uint16_t z = Data_Address; // Useful for debugging.

    uint16_t Deref_Data_Address = *Data_Address; // Dereferenced the pointer
    uint16_t Deref_Register_Address = *Bike_Status.Queue_Ptr; // Dereferenced the pointer (easier to read later code)

    Serial.print("Temp Register Address (2): 0x");

    Serial.println(z, HEX);
    
    Data = Deref_Data_Address;
     
    RW = Deref_Register_Address & 0b00000001; // Logic AND on the last bit. If there is a 1 on the LSB, we are READING

    Register_Address = (Deref_Register_Address >> 1); // Shift the register address back

    if (Bike_Status.Queue_Counter > 1) {
      memmove(Bike_Status.Queue_Ptr, Bike_Status.Queue_Ptr+2, (2*Bike_Status.Queue_Counter - 2) ); // Overwrite first two bytes and shrink allocated memory by two bytes (why did -4 work up until the last)?
    }

    Bike_Status.Queue_Counter--; // Succesfully grabbed items from the queue.

    Serial.print("Received from Queue (Register_Addrsss, Data, RW): 0x");

    Serial.print(Register_Address, HEX);

    Serial.print(" 0b");

    Serial.print(Data, BIN);

    Serial.print(" ");

    Serial.println(RW, BIN);
    
    // Serial.print("Register_Address: "); Serial.println(Register_Address, HEX);
    // Serial.print("Data: "); Serial.println(Data, HEX);

    TWI_MCP(Register_Address, Data, RW); // Start TWI with the new data fetched from the queue


    Serial.print("Queue Counter:");
    Serial.println(Bike_Status.Queue_Counter);

    

    /*
    for (uint8_t i = 0; i < (Bike_Status.Queue_Counter+2); i++) {
      Bike_Status.Queue_Ptr[i] = Bike_Status.Queue_Ptr[i + 2]; // DOUBLE CHECK THIS (I don't really like this re-shuffling)
    }
   */

    // Serial.println("Sent last!");

  }
  
  if(Bike_Status.Queue_Counter == 0){
     if(Bike_Status.Queue_Ptr != NULL){
       free(Bike_Status.Queue_Ptr);
       Serial.println("Queue Handler: Queue is empty. All Allocated memory released");
     }else{
       Serial.println("Queue was already empty");
     }
  }

  return;

}

/*For the MCP23017 to read data from a register:
   Transmit START, MCP_ADDRESS (WRITE), REGISTER_ADDRESS, REPEATED START, MCP_ADDRESS (READ), DATA_OUT, STOP
*/

ISR(INT0_vect){
    Toggle_LED(1);
}

ISR(TWI_vect) {

  const uint8_t MCP23017_Address = 0x20;

  uint8_t RW = Bike_Status.TWI_RW;
    
  Bike_Status.Transmission_Complete = false; // A redundency, but this just ensures everytime the ISR is called, a new TWI sequence is never started.

  //Serial.println(RW);

  switch (TWI_States) {

    case REPEATED_START: // This is only used for when reading. Once the TWI has written the register address, we will be in this case.

      TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTA); // Set start bit, reset interrupt flag, enable TWI (TWI START)

      Serial.println("Repeated Start");

      TWI_States = ADDRESSING;

      break;

    case ADDRESSING: // Used for sending the MCP23017 I2C address, the target register address, and the R/~W bit.

      if (TWSR == 0x08 || TWSR == 0x10) { // A START or REPEATED START condition has been transmitted

        Serial.println("ADDRESSING!");

        if (!RW || ((TWSR != 0x10) && RW)) { // Writing OR Read operation AND the register to read from has not been selected (No repeated start).

          TWI_States = MAIN_WRITE;

          TWDR = (MCP23017_Address << 1); // Load the TWDR with the 7 bit address of the MCP23017, with the LSB being 0 (Write).

          Serial.println("TWI: MCP23017 Address sent (Write)");

        } else { // Send the address and the read bit:

          TWI_States = MAIN_READ;

          TWDR = (MCP23017_Address << 1) + 1; // Load the TWDR with the 7 bit address of the MCP23017, with the LSB being 1 (Read). 
  
          Serial.println("TWI: MCP23017 Address sent (Read)");

        }

        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWEA); // Transmit address - At this point TWSR should = 0x18 SLA+W has been transmitted ACK received OR TWSR should = 0x40 SLA+R has been transmitted ACK received

        break;

      } else { // Error state

        Serial.println("Failed TWI Start");
        Serial.println(TWSR);
        break;
        
      }


    /*
       If SLA+W is transmitted, MT mode is entered,
       if SLA+R is transmitted, MR mode is entered
    */


    case MAIN_WRITE: // This case handles writing the target register address.

      //  Serial.println("Here!");
      //Serial.println(TWSR);


      if (TWSR == 0x18) { // 0x18 Start bit sent - ACK
          
        TWDR = (Bike_Status.TWI_Register); // Load the TWDR with the target register address
        Serial.println("Sent Register Address");

        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Transmit

        TWI_States = MAIN_WRITE; // Go back and check the state of the TWI bus. STOP if writing, REPEATED START if reading.

      } else if (TWSR == 0x28) { // 0x28 - Data byte has been transmitted ACK
         
         TWDR = (Bike_Status.TWI_Data);

         if (RW) { // If Reading:
          
           // Bike_Status.Read_Register_Selected = true; // 0x10...?
           
           TWI_States = REPEATED_START;
           
           break;
           
         }else{
          
           TWI_States = STOP;
           
         }

         TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Transmit
         
         Serial.println("Sent Data");
          
  
      } else { // Error state
        // TWSR == 0x20 -> NACK
        
        Serial.print(TWSR, HEX);
        
        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTO); // Free the TWI bus. Transmit STOP
        Serial.println("Failed");
        
      }

      break;

    case MAIN_READ: // This case handles 
  
      if(TWSR == 0x40) { // Read Address has been trasnmitted, and ACK recieved:
         
        Serial.println("SLA+R ACKed, waiting for data...");

        TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Clear flag
        
      } else if (TWSR == 0x58) { // Data recieved from MCP23017! (NACK - Last byte/single byte) (TWEA is bit is set to ZERO - (acknowladge bit set to 0))
         
         Bike_Status.Read_Data = TWDR;
         Serial.println(Bike_Status.Read_Data, HEX);
    
         TWI_States = STOP;
  
         TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE); // Clear flag LAST

      }else{
        
        Serial.print("Unexpected state: ");
        Serial.println(TWSR, HEX);
        // Error state
      }

      break;

    case STOP:


      TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWSTO) | (1 << TWIE); // Transmit STOP condition
      Serial.println("STOP SENT");
        
      Bike_Status.Transmission_Complete = true;

      TWI_Queue_Handler(Bike_Status.Read_Data); // Call the Queue handler to see if there is anything it
          
      break;
      

    case FAILED:
      Serial.print(TWSR, HEX); // Check for codes HERE...
      //TWDR = 0;
      TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWIE) | (1 << TWSTO); // case STOP
      Serial.println("Failed");
      break;

  }

}

void Init_328_GPIO() {
  DDRB |= (1 << DDB0); // Set B0 as an output pin
  DDRD = (1 << DDD0); // Set D0 as an output pin
  PORTB &= ~(1 << PORTB0); // Set B0 to LOW
}

void Toggle_LED(uint8_t State) {
  if (State) {
    PORTB |= (1 << PORTB0); // Set B0 HIGH
  } else {
    PORTB &= ~(1 << PORTB0); // Set B0 LOW
  }

}

ISR(ADC_vect) { // Send multiple bytes of data before end!!!!

  uint16_t Reg0_7 = ADCL; // Read the ADCL registers first
  uint16_t Reg8_9 = ADCH; // Read ADCH last, setting ADSC LOW, allowing writing to the ADC

  if (ADC_Selection == 0) { // ADC 0 - Handles THROTTLE information

    ADC_DATA_0 = Reg0_7 | (Reg8_9 << 8); // Combine the results from the ADC
    ADMUX |= (1 << MUX0); // Switch to ADC 1
    ADC_Selection = 1;
    Compute_Throttle();

  } else if (ADC_Selection == 1) { // ADC 1 Capacitor 1 Voltage

    ADC_DATA_1 = Reg0_7 | (Reg8_9 << 8);
    ADMUX = (ADMUX & ~(1 << MUX0)) | (1 << MUX1); // Set MUX0 to 0 and Set MUX1 to 1, Switch to ADC 2
    ADC_Selection = 2;

  } else if (ADC_Selection == 2) { // ADC 2 +VCC Voltage

    ADMUX &= ~(1 << MUX1); // Set MUX 1 to 0, Switch to ADC 0
    ADC_DATA_2 = Reg0_7 | (Reg8_9 << 8);
    ADC_Selection = 0;
    Compute_Voltages();

  } else {

    return;

  }

  if ((int)ADC_DATA_0 > 400) {
    Toggle_LED(1);
  } else {
    Toggle_LED(0);
  }

  ADCSRA |= (1 << ADSC); // Start the next conversion
}


void init_ADC() {
  PRR &= ~(1 << PRADC); // Set the Power Reduction ADC bit to logic LOW
  ADMUX = (1 << REFS0); // Set DAC AREF to AVCC, Right shifted, ADC0
  ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Enable ADC, Enable ADC interrupt, Division factor of 128 = 125kHz ADC
}

int main(void) { // MAKE IT SO RW goes FIRST so i dont need DATA

  // Read = 1 Write = 0
  Serial.begin(9600);

  EICRA |= (1<< ISC11) | (1<<ISC01); // Falling edge on INT0 and INT1 causes the ISR to run

  EIMSK |= (1<<INT1) | (1<<INT0); // Enable INT0, INT1

  DDRD &= ~(1 << DDD2);   // Set PD2 (INT0) as input
  //PORTD |= (1 << PORTD2); // Enable internal pull-up (optional, but good idea)
  
  //sei(); // Enable global interrupts -> MOVE TO AFTER REQUESTS TO DEBUG


  // 1) Set BANK=1 ? IOCON @ 0x0A (BANK=0) goes 0b1000?0000
    TWI_MCP(0x0A, 0b10000000, 0);

    // 2) IODIRB @ 0x10 ? 0b0011?1000 (GPB3-5 inputs)
    TWI_MCP(0x10, 0b00111000, 0);

    // 3) GPPUB @ 0x16 ? 0b0011?1000 (enable pull-ups on GPB3-5)
    //TWI_MCP(0x16, 0b00111000, 0);

    // 4) GPINTENB @ 0x12 ? 0b0011?1000 (enable IOC on GPB3-5)
    TWI_MCP(0x12, 0b00010000, 0);

    // 5) DEFVALB @ 0x13 ? 0xFF (idle=HIGH)
    TWI_MCP(0x13, 0xFF,       0);

    // 6) INTCONB @ 0x14 ? 0b0011?1000 (compare GPB3-5 ? DEFVALB)
    TWI_MCP(0x14, 0b00010000, 0);

    // 7) “Read GPIOB (0x19) to clear any pending interrupt.” ? RIGHT CALL,
    //    but nothing in main() waits for that read to actually finish.
    //TWI_MCP(0x19, 0, 1);

   
    SREG |= 0b10000000; // Enable global interrupts -> MOVE TO AFTER REQUESTS TO DEBUG


    
  //TWI_MCP(0x01, 0x00, 0); 


 // TWI_MCP(0x13, 0, 1);

  // TWI_MCP(0x13, 0x00, 1); // Address GPIOB, set all pins hi init_ADC(); 0x13 originally 

  //  TWI_MCP(0x13, 0b00000000, 0); // Address GPIOB, set all pins hi  init_ADC(); 0x13 originally

  //TWI_MCP(0x13, 0b00000001); // Address GPIOB, set all pins hi  init_ADC(); 0x13 originally



  ADCSRA |= (1 << ADSC); // Start the ADC

  Init_328_GPIO();

  Toggle_LED(0);

  while (1) {
    // Main loop can perform other tasks, or go to sleep
    if (Bike_Status.is_ShutDown) {
      break;
    }
  }

  return 0;
}
