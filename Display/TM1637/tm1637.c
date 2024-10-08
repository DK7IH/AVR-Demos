/*****************************************************************/
/*                    Display TM1637                             */
/*  ************************************************************ */
/*  MUC:              ATMEL AVR ATmega328p, 8 MHz                */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Author:           Peter Baier (DK7IH)                        */
/*  Last change:      SEP 2022                                   */
/*****************************************************************/

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>


#ifdef F_CPU
#undef F_CPU
#endif

#define CMD1_WRITE_AUTO  0b01000000
#define CMD1_WRITE_FIX   0b01000100
#define CMD3_ADDRESS     0b11000000
#define CMD3_DISPLAY_ON  0b10001000
#define CMD3_DISPLAY_OFF 0b10000000

#define DELAYTIME 2

//Defining segments in use for each number
const uint8_t segdata[] = { 0b00111111,    // 0
                            0b00000110,    // 1
                            0b01011011,    // 2
                            0b01001111,    // 3
                            0b01100110,    // 4
                            0b01101101,    // 5
                            0b01111101,    // 6
                            0b00000111,    // 7
                            0b01111111,    // 8
                            0b01101111};   // 9

#define MAX_BRIGHTNESS 7

#define DISPLAYPDDR DDRB
#define DISPLAYPORT PORTB
#define CLK 0
#define DIO 1

int main(void);
void tm1637_start(void);
void tm1637_stop(void);
void tm1637_write(int);
void tm1637_clear(void);
void set_brightness(int);
void set_digit(int, int);
void show_number(long);


void tm1637_start(void)
{
	DISPLAYPORT &= ~(1 << DIO); 
	_delay_us(DELAYTIME);
	DISPLAYPORT &= ~(1 << CLK);
	_delay_us(DELAYTIME);
}

void tm1637_stop(void)
{
	DISPLAYPORT &= ~(1 << DIO); 
	_delay_us(DELAYTIME);
	DISPLAYPORT |= (1 << CLK); 
	_delay_us(DELAYTIME);
	DISPLAYPORT |= (1 << DIO); 
	_delay_us(DELAYTIME);
}
	
void tm1637_write(int value)
{
    int t1;
        
    for(t1 = 0; t1 < 8; t1++)
    {
		_delay_us(DELAYTIME);
		DISPLAYPORT &= ~(1 << CLK);
		_delay_us(DELAYTIME);
		
		if((1 << t1) & value)
		{
			DISPLAYPORT |= (1 << DIO);
		}
		else	
		{
			DISPLAYPORT &= ~(1 << DIO);
		}
		_delay_us(DELAYTIME);
		
		DISPLAYPORT |= (1 << CLK);
		_delay_us(DELAYTIME);
	}	
	DISPLAYPORT &= ~(1 << CLK);
	DISPLAYPORT |= (1 << DIO);
	_delay_us(DELAYTIME);
	
	DISPLAYPORT |= (1 << CLK);
	_delay_us(DELAYTIME);
			
	DISPLAYPORT &= ~(1 << CLK);		
	_delay_us(DELAYTIME);
}	

void tm1637_clear(void)
{
	int t1 = 0;
	
	for(t1 = 0; t1 < 6; t1++)
	{
	    tm1637_start();
        tm1637_write(CMD1_WRITE_FIX); //Write and address mode
        tm1637_stop();
    
        tm1637_start();
        tm1637_write(CMD3_ADDRESS | t1); //Define digit position
        tm1637_write(0x00);              //Write data
        tm1637_stop();
     }   
}     
	
void set_brightness(int brightness)
{
    tm1637_start();
    tm1637_write(CMD3_DISPLAY_ON | brightness);
    tm1637_stop();
}

void set_digit(int pos0, int num)
{
	int pos1 = pos0;
	
	
	if(pos0 >= 3)
	{
		pos1 = 5 - (pos0 - 3);
	}	
	
	if(pos0 <= 2)
	{
		pos1 = 5 - (pos0 + 3);
	}
		
	tm1637_start();
    tm1637_write(CMD1_WRITE_FIX); //Write and address mode
    tm1637_stop();
    
    tm1637_start();
    tm1637_write(CMD3_ADDRESS | pos1); //Define digit position
    if(pos1 == 4)
    {
        tm1637_write(segdata[num] | 128);            //Write data + decimal
    }
    else
    {
        tm1637_write(segdata[num]);            //Write data
    }    
    tm1637_stop();
}

void show_number(long n)
{
	int t1, dig;
	long d, n1 = n;
			
	//Get valid digits	
	for (t1 = 0; n1 / 10; t1++)
	{
	    n1 = n1 / 10;
	}
	dig = t1 + 1;
	
	//Display valid digits
	n1 = n;	
	for (t1 = 0; t1 < dig; t1++)
	{
		d  = n - 10 * (n / 10);
		n = n / 10;
		set_digit(5 - t1, d);
	}	
}	

	
int main(void)
{
	int t1 = 100;			
	
	//OUPUT lines for TM1637
    DISPLAYPDDR = (1 << CLK) | (1 << DIO);
            
    //Init Display
    DISPLAYPORT |= (1 << DIO); 
	DISPLAYPORT |= (1 << CLK); 
		
	set_brightness(7);
	tm1637_clear();
	_delay_ms(10);
	
    for(;;)
    {
    
        show_number(t1++);
        //wait_ms(1);
        if(t1 > 999999)
        {
			t1 = 0;
		}	

    }
	return 0;
}
