///////////////////////////////////////////////////////////////////
//            Module 7 Using rotary encoder    Version 1         //
///////////////////////////////////////////////////////////////////
//  Mikrocontroller:  ATMEL AVR ATmega328p,16 MHz                //
//                                                               //
//  Compiler:         GCC (GNU AVR C-Compiler)                   //
//  Autor:            Peter Baier                                //
//  Letzte Aenderung: 08.01.2021                                 //
///////////////////////////////////////////////////////////////////
//Connect encoder to PB0 and PB1
//2LEDs to VDD on PD0 and PD1 to control direction

#define CPUCLK 8

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

//Global variables encoder
int laststate = 0; //Last state of rotary encoder
int direction = 0;

int main(void);

//Simple delay
void wait_ms(int ms)
{
    int t1, t2;

    for(t1 = 0; t1 < ms; t1++)
    {
        for(t2 = 0; t2 < 137 * CPUCLK; t2++)
        {
            asm volatile ("nop" ::);
        }   
     }    
}

  /////////////////////////
 //  INTERRUPT HANDLER  //
/////////////////////////
//Rotary encoder
ISR(PCINT0_vect)
{ 
    int pins = (PINB & 0x03);               //Read PB0 and PB1
	
    int state = (pins >> 1) ^ pins;         //Convert from Gray code to binary

    if (state != laststate)                 //Compare states
    {        
        direction += ((laststate - state) & 3) - 2; // Results in -1 or +1
        laststate = state;
    } 
    PCIFR |=  (1 << PCIF0); //Clear pin change interrupt flag.
}

int main()
{
	PORTB = 0x03; //INPUT: Pullup resistors PD0 and PD1 rotary encoder
	DDRD = 0x03;  //Output LEDs on PD0, PD1
	
    //Interrupt definitions for rotary encoder  
	PCMSK0 |= ((1<<PCINT0) | (1<<PCINT1));  //Enable encoder pins as interrupt source
	PCICR |= (1<<PCIE0);                    //Enable pin change interupts for interrupt bank 0
	
	//Test LEDs
	PORTD &= ~(1 << 0);
	PORTD &= ~(1 << 1);
	wait_ms(1000);
	PORTD |= (1 << 0);
	PORTD |= (1 << 1);
	
    sei(); //Enable global interrupts
        
    for(;;) 
	{
		if(direction > 1)
		{
			PORTD &= ~(1 << 0);
	   	    PORTD |= (1 << 1);
	   	    direction = 0;
	   	}
	   	
	   	if(direction < -1)
	   	{
			PORTD &= ~(1 << 1);
		    PORTD |= (1 << 0);
		    direction = 0;
		}    
	
		
	}
	return 0;
}
