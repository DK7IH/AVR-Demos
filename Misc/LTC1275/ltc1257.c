//////////////////////////////////////////////////
//  DAC with ATmega32 and LTC1257  DAC          //         
//////////////////////////////////////////////////
//  CVontroller: ATMEL AVR ATmega168, Clock 8M  //                
//  Compiler:    GCC (GNU AVR C-Compiler)       //            
//  Author:      Peter Baier                    //            
//////////////////////////////////////////////////

//PORT and PINs
#define CLK PD0
#define DIN PD1
#define LOAD PD2

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>

#define PI 3.14159265

void ltc1257_spi_send_data(long);
int main(void);

    //////////////////////
   //                  //
  // SPI for LTC1257  //
 //                  //
//////////////////////
void ltc1257_spi_send_data(long xbyte)
{
    int t1, x = 2048;
    
    PORTD |= (1 << LOAD); //LOAD hi
    
    for(t1 = 0; t1 < 13; t1++)
    {
		if(xbyte & x)
        {
            PORTD |= (1 << DIN); //DIN hi
        }
        else
        {
            PORTD &= ~(1 << DIN);//DIN lo
        }
        PORTD |= (1 << CLK);     //CLK hi
        PORTD &= ~(1 << CLK);    //CLK lo
                
        x >>= 1;
    }
    
    PORTD &= ~(1 << LOAD);   // LOAD lo
}

int main(void)
{
	int t1;
	double ret;
	
	//OUTPORT
	DDRD = 0x07; //PD0, PD1, PD2: LTC1257 SPI control lines
		    
	for(;;)
    {
		
        for(t1= 0; t1 < 358; t1++)
        {
           ret = sin((double)t1 * 2 * PI / 360) * 1023 + 1023;
           ltc1257_spi_send_data((long) ret);
        } 
	}
	return 0;
}
