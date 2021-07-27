/*****************************************************************/
/*                    DDS mit AD9850                            */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega8, 8 MHz                  */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*  Letzte Aenderung: 23.03.2019                                 */
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

int main(void);

/*******************/
//       SPI
/*******************/
//Belegung
//FQ_UD: green
//SDATA: white
//W_CLK: blue
//RESET  violet/pink

///////////////////////
//  SPI DDS AD9850  //
///////////////////////
#define DDS_PORT PORTC
#define DDS_DDR  DDRC
#define DDS_W_CLK 0  //blue PB3
#define DDS_FQ_UD 1 //green PB0
#define DDS_SDATA 2 //white PB1
#define DDS_RESETPORT PORTC 
#define DDS_RESETDDR DDRC
#define DDS_RESETPIN 3

void spi_send_bit(int);
void set_frequency_ad9850(unsigned long);

//************
//    SPI
//************
void spi_send_bit(int sbit)
{
    if(sbit)
	{
		DDS_PORT |= (1 << DDS_SDATA);  
	}
	else
	{
		DDS_PORT &= ~(1 << DDS_SDATA);
	}
	
	DDS_PORT |= (1 << DDS_W_CLK);  
    DDS_PORT &= ~(1 << DDS_W_CLK);
    
}


//Set AD9850 to desired frequency
void set_frequency_ad9850(unsigned long fx)
{
    unsigned long f, clk = 125000000;
    unsigned long hibyte, lobyte;
	int t1;

    double fword0;
    unsigned long fword1;
    		
	//Define frequency word
    clk >>= 8;
    f = (fx << 8);
    fword0 = (double) f / clk;
    fword1 = (unsigned long) (fword0);
    fword1 <<= 16;
	hibyte = fword1 >> 16;
    lobyte = fword1 & 0xFFFF;

    //Send 32 frequency bits + 8 additional bits to DDS
	//Start sequence
	DDS_PORT &= ~(1 << DDS_FQ_UD); //FQ_UD lo => Bit PD0 = 0
       
	//W0...W15
	for(t1 = 0; t1 < 16; t1++)
    {
       spi_send_bit(lobyte & (1 << t1));
    }
	
	//W16...W31
	for(t1 = 0; t1 < 16; t1++)
    {
       spi_send_bit(hibyte & (1 << t1));
    }
	
	//W32...W39
	for(t1 = 0; t1 < 8; t1++)
	{
	    spi_send_bit(0);
	}	
	
	//Stop  sequence
    DDS_PORT |= (1 << DDS_FQ_UD); //FQ_UD hi => Bit PD0 = 1
	       
}

int main()
{

	DDS_DDR = 0x07; //SPI (PB0..PB2) 
    DDS_RESETDDR |= 0x0F;
	
	long f;
	
	DDS_RESETPORT |= (1 << DDS_RESETPIN);       //Bit set
    _delay_ms(1);       //wait for > 20ns i. e. 1ms minimum time with _delay_s()
	DDS_RESETPORT &= ~(1 << DDS_RESETPIN);  //Bit erase        
  
    set_frequency_ad9850(10000000);
    set_frequency_ad9850(10000000);
        
    for(;;) 
	{
		for(f= 1000000; f<40000000; f+=100)
		{
			set_frequency_ad9850(f);
		}	
	    
	}

	
	
    return 0;
}
