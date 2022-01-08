///////////////////////////////////////////////////////////////////
//            Module 1 Defining output port                      //
//            LED connected from VDD to PB0                      //                     
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
void wait_ms(int);

// Cheap & dirty delay
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

int main()
{
	DDRB = (1 << 0); //Pin PB0 as Output
            
    for(;;) 
	{
		PORTB &= ~(1 << 0); //Pin LO
		wait_ms(500);
		PORTB |= (1 << 0);  //Pin HI
		wait_ms(500);
	}

	return 0;
}
