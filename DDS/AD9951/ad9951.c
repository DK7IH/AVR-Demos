/*****************************************************************/
/*                   DDS with  ATMega168and 9951                 */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega8, 8 MHz                   */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow  DK7IH                        */
/*  Letzte Aenderung: 09.01.2016                                 */
/*****************************************************************/


#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#define DDSPORT PORTC
#define DDSDDR DDRC

#define DDSRES 1 //gray
#define SDIO 2
#define SCLK 4
#define IO_UD 8  


int main(void);

/*******************/
//       SPI
/*******************/
//Port usage
//IO_UPDATE:   PB0 (1) (green)  
//SDIO (DATA): PB1 (2) (white)
//SCLK         PB2 (4) (blue)
//RESET        PD7     (violet/ white-blue)

void spi_send_byte(unsigned int);
void set_frequency(unsigned long);

//************
//    SPI
//************

void spi_send_byte(unsigned int sbyte)
{
    int t1, x = (1 << 7);
	
	//spi_start();
	for(t1 = 0; t1 < 8; t1++)
	{
	    //spi_send_bit(sbyte & x);	
	    DDSPORT &= ~(SCLK);  //SCLK lo
    	
        //Bit setzen oder löschen
	    if(sbyte & x)
	    {
		    DDSPORT|= SDIO;  //SDATA  set
	    }
	    else
	    {
		    DDSPORT &= ~(SDIO);  //SDATA  erase
	    }
	
        DDSPORT |= SCLK; //SCLK hi
		x >>= 1;
	}	
}


//SET F AD9951 DDS
void set_frequency(unsigned long frequency)
{
    //unsigned long interfreq = 10E06; //Interfrequency of radio in Hz
    unsigned long f;
    unsigned long fword;
    int t1, shiftbyte = 24, resultbyte;
    unsigned long comparebyte = 0xFF000000;
	
	f = frequency; //Offset because of inaccuracy of crystal oscillator
	
    //Calculate frequency word
    //2³² / fClk = ----
    
    //Clock rate =  75MHz
    //fword = (unsigned long) f * 57.266230613;
    
    //Clock rate =  100MHz
    //fword = (unsigned long) f * 42.94967296;
        
    //Clock rate =  110MHz
    //fword = (unsigned long) f * 39.045157236;
    
    //Clock rate =  125MHz
    fword = (unsigned long) f * 34.358675; 
        	
	//Clock rate =  200MHz
    //fword = (unsigned long) f * 21.47478;  //..36448
    
    //Clock rate =  300MHz
    //fword = (unsigned long) f * 14.316557653;
    
    //Clock rate =  400MHz
    //fword = (unsigned long) f * 10.73741824;
		
	
    //Start transfer to DDS
    DDSPORT &= ~(IO_UD); //IO_UD lo
    
	//Send instruction bit to set fequency by frequency tuning word
	spi_send_byte(0x04);	
	
    //Calculate and transfer the 4 bytes of the tuning word to DDS
    //Start with msb
    for(t1 = 0; t1 < 4; t1++)
    {
        resultbyte = (fword & comparebyte) >> shiftbyte;
        comparebyte >>= 8;
        shiftbyte -= 8;       
        spi_send_byte(resultbyte);	
    }    
	
	//End transfer sequence
    DDSPORT|= (IO_UD); //IO_UD hi 
}

void set_clock_multiplier(void)
{
    //Start transfer to DDS
    DDSPORT &= ~(IO_UD); //IO_UD lo
    
	//Send CFR2
	spi_send_byte(0x01);
	
	//Multiply by 4
	spi_send_byte(0x00);
	spi_send_byte(0x00);
	spi_send_byte(0x24); //0x04 << 3
	
	
	//End transfer sequence
    DDSPORT |= (IO_UD); //IO_UD hi 
}	


int main()
{
         
    //Set DDRs of DDSPort and DDSResetport  
	DDSDDR = 0x0F; //SPI-Lines PC0:PC2, RES PC3
	
	_delay_ms(100);
	//Reset DDS (AD9951)
	DDSPORT |= DDSRES;  
	_delay_ms(100);
	DDSPORT &= ~DDSRES;          
    _delay_ms(100);
	DDSPORT |= DDSRES;  
	
    //set_clock_multiplier();
    set_frequency(5000000);
    set_frequency(5000000);
    for(;;) 
	{
		
	}
	
    return 0;
}
