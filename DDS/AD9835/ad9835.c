/*****************************************************************/
/*                     DDS mit  AD9835                           */
/*  ************************************************************ */
/*  MUC:              ATMEL AVR ATmega328, 8 MHz                 */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Author:           Peter Rachow (DK7IH)                       */
/*  Last change:      16 AUG 2015                                */
/*****************************************************************/
//
//
// SPI to AD9835
// FSYNC    = PB0
// SDATA:   = PB1
// SCLK:    = PB2

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/twi.h>

#undef F_CPU 
#define F_CPU 8000000 


////////////////
// SPI for DDS
////////////////
#define DDSPORT PORTD
#define FSNYC 8 //green PB3
#define SDATA 2 //white PB1
#define SCLK 4  //blue PB2


//************
//    SPI
//************
void spi_send_word(unsigned int sbyte)
{
    unsigned int t1, x = 32768;
    for(t1 = 0; t1 < 16; t1++)
    {
        if(sbyte & x)
        {
            DDSPORT |= SDATA;  //SDATA Bit PB1 set
        }
        else
        {
            DDSPORT &= ~(SDATA);  //SDATA Bit PB1 erase
        }
        
        DDSPORT |= SCLK;  //SCLK hi
        DDSPORT &= ~(SCLK); //SCLK lo
        
        x = x >> 1;
    }
    DDSPORT |= SDATA; //PB1 SDATA hi
}

void set_frequency(unsigned long f, int initad9835)
{
    unsigned long fxtal = 50000000;  //fCrystal in MHz
    double fword0;
    unsigned long fword1;
    int t1;

    unsigned char xbyte[] = {0x33, 0x22, 0x31, 0x20};    
    fword0 = (double) f / fxtal;
    fword1 = (unsigned long) (fword0 * 0xFFFFFFFF);

    if(initad9835)
    {
        //init, set AD9835 to sleepmode
        DDSPORT &= ~(FSNYC); //FSYNC lo
        spi_send_word(0xF800);
        DDSPORT |= FSNYC; // FSYNC hi
    }

    //Send 4 * 16 Bit to DDS
    for(t1 = 0; t1 < 4; t1++)
    {
        DDSPORT &= ~(FSNYC); //FSYNC lo => Bit PB0 = 0
        spi_send_word((xbyte[t1] << 8) + (((fword1 >> ((3 - t1) * 8))) & 0xFF));
        DDSPORT |= FSNYC; // FSYNC hi => Bit PB0 = 1
    }

    //End of sequence
    DDSPORT &= ~(FSNYC); //FSYNC lo => Bit PB0 = 0
    if(initad9835)
    {
        //AD9835 wake up from sleep
        spi_send_word(0xC000);
    }
    else
    {
        //AD9835 freq data update, no full init
        spi_send_word(0x8000);
    }
    DDSPORT &= ~(FSNYC); //FSYNC lo => Bit PB0 = 0	
}


int main(void)
{
    unsigned long freq = 5000000;
    DDRD = 0x07;//SPI for DDS on PB0..PB2

    _delay_ms(100);
    
    set_frequency(freq, 1);
    set_frequency(freq, 0);
    	
    for(;;)
    {
	     for(freq = 2000000; freq < 2200000; freq += 1)
	     {
			 set_frequency(freq, 0);
			 _delay_ms(1);
		 }
    }
    return 0;
}
