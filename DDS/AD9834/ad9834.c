/*****************************************************************/
/*                 DDS with ATMega und AD9834                   */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega32, 16 MHz                 */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*  Letzte Aenderung: 17.11.2018                                 */
/*****************************************************************/
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>


int main(void);

///////////////////////
//  SPI DDS1  AD9834 //
///////////////////////
#define DDSPORT PORTB
#define FSYNC 0 //white
#define SCLK  1  //blue
#define SDATA 2 //green

#define CPUCLK 16

void spi1_start(void);
void spi1_send_bit(int);
void spi1_stop(void);
void set_frequency1(unsigned long);
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

/////////////////
//  SPI DDS1   //
/////////////////
void spi1_start(void)
{
	DDSPORT |= (1 << SCLK);      //(1 << SCLK) hi
    DDSPORT &= ~(1 << FSYNC);  //(1 << FSYNC) lo
}

void spi1_stop(void)
{
	DDSPORT |= (1 << FSYNC); //(1 << FSYNC) hi
}

void spi1_send_bit(int sbit)
{
    if(sbit)
	{
		DDSPORT |= (1 << SDATA);  //(1 << SDATA) hi
	}
	else
	{
		DDSPORT &= ~(1 << SDATA);  //(1 << SDATA) lo
	}
	
	DDSPORT |= (1 << SCLK);     //(1 << SCLK) hi
    DDSPORT &= ~(1 << SCLK);  //(1 << SCLK) lo
}

void set_frequency1(unsigned long f)
{

    double fword0;
    long fword1, x;
    int l[] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int m[] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, t1;
    
    //2.440322327 = 268435456 / 110000000
    
    //fword0 = (double) 2.440322327  * f; //fClk = 110 MHZ
    
    fword0 = (double) 3.579139413 * f; // 75MHz
    //fword0 = (double) 5.36870912  * f; //fClk = 50MHz
    
    fword1 = (long) fword0;

    //Transfer frequency word to byte array
    x = (1 << 13);      //2^13
    for(t1 = 2; t1 < 16; t1++)
    {
		if(fword1 & x)
	    {
			l[t1] = 1;
	    }
	    x >>= 1;
    }
    
    x = (1L << 27);  //2^27
    for(t1 = 2; t1 < 16; t1++)
    {
	    if(fword1 & x)
	    {
	        m[t1] = 1;
	    }
	    x >>= 1;
    }
    ////////////////////////////////////////
    
    //Transfer to DDS
    //Send start command
    spi1_start();
    for(t1 = 15; t1 >= 0; t1--)
    {
       spi1_send_bit(0x2000 & (1 << t1));
    }
    spi1_stop();
        
    //Transfer frequency word	
    //L-WORD
    spi1_start();
    for(t1 = 0; t1 < 16; t1++)
    {
       spi1_send_bit(l[t1]);
    }
    spi1_stop();
	
	//M-WORD
	spi1_start();
    for(t1 = 0; t1 < 16; t1++)
    {
       spi1_send_bit(m[t1]);
    }
    spi1_stop();
}


int main()
{
	DDRB = 0x03; //SPI (Bit0...Bit2 of respective port) 
    
    wait_ms(10);       
   
	set_frequency1(10000000);
	set_frequency1(10000000);
		
	for(;;) 
	{
	}    
	
    return 0; 
}
