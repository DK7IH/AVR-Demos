///////////////////////////////////////////////////////////////////
//            Module 2 Defining input port                       //
//            LED connected from VDD to PB0                      //                     
//            Switch from PC0 to GND                             // 
///////////////////////////////////////////////////////////////////
//  Mikrocontroller:  ATMEL AVR ATmega328p,16 MHz                //
//                                                               //
//  Compiler:         GCC (GNU AVR C-Compiler)                   //
//  Autor:            Peter Baier                                //
//  Letzte Aenderung: 07.01.2021                                 //
///////////////////////////////////////////////////////////////////
#include <avr/io.h>
#include <inttypes.h>
#include <stdio.h>

#define CPUCLK 16

int main(void);

int main()
{
    //Output port
	DDRB = (1 << 0); //Pin PB0 as Output for indicator LED
	
	//INPUT port
	DDRC &= ~(1 << 0); //Set PC0 as input pin (basically not neccessary)
	PORTC = (1 << 0);  //Set Pull-up resistor to VDD for PC0 if standrad switch should be used
            
    for(;;) 
	{
		if(!(PINC & (1 << 0))) //Switch to GND -> LED on
		{
		    PORTB &= ~(1 << 0); //Pin LO
		}    
		else
		{
			PORTB |= (1 << 0);  //Pin HI
		}	
	}

	return 0;
}
