/*****************************************************************/
/*                  TEST w. ATMega88 and Si5351                 */
/*  ************************************************************ */
/*  MUC:              ATMEL AVR ATmega88,  8 MHz                 */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Author:           Peter Rachow (DK7IH)                       */
/*  Last change:      OCT 2017                                   */
/*****************************************************************/


//TWI
//PC4=SDA, PC5=SCL: I�C-Bus lines: 

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <util/twi.h>

/////////////////////
//Defines for Si5351
/////////////////////
#define SI5351_ADDRESS 0xC0 // 0b11000000 for my module. Others may vary! The 0x60 did NOT work with my module!
#define PLLRATIO 36
#define CFACTOR 1048575

//Set of Si5351A register addresses
#define CLK_ENABLE_CONTROL       3
#define PLLX_SRC				15
#define CLK0_CONTROL            16 
#define CLK1_CONTROL            17
#define CLK2_CONTROL            18
#define SYNTH_PLL_A             26
#define SYNTH_PLL_B             34
#define SYNTH_MS_0              42
#define SYNTH_MS_1              50
#define SYNTH_MS_2              58
#define PLL_RESET              177
#define XTAL_LOAD_CAP          183

#undef F_CPU
#define F_CPU 8000000

//SI5351 Declarations & frequency
void si5351_write(int, int);
void si5351_start(void);
void si5351_set_freq(int, unsigned long);


///////////////////////////
//
//         TWI
//
///////////////////////////

void twi_init(void)
{
	
    //set SCL to 400kHz
    TWSR = 0x00;
    TWBR = 0x28;
    
    //enable TWI
    TWCR = (1<<TWEN);
    
}

//Send start signal
void twi_start(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

//send stop signal
void twi_stop(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void twi_write(uint8_t u8data)
{
    TWDR = u8data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

uint8_t TWIGetStatus(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}

////////////////////////////////
//
// Si5351A commands
//
///////////////////////////////
void si5351_write(int reg_addr, int reg_value)
{
   	 
   twi_start();
   twi_write(SI5351_ADDRESS);
   twi_write(reg_addr);
   twi_write(reg_value);
   twi_stop();
} 

// Set PLLs (VCOs) to internal clock rate of 900 MHz
// Equation fVCO = fXTAL * (a+b/c) (=> AN619 p. 3
void si5351_start(void)
{
  unsigned long a, b, c;
  unsigned long p1, p2;//, p3;
    
  // Init clock chip
  si5351_write(XTAL_LOAD_CAP, 0xD2);      // Set crystal load capacitor to 10pF (default), 
                                          // for bits 5:0 see also AN619 p. 60
  si5351_write(CLK_ENABLE_CONTROL, 0x00); // Enable all outputs
  si5351_write(CLK0_CONTROL, 0x0F);       // Set PLLA to CLK0, 8 mA output
  si5351_write(CLK1_CONTROL, 0x2F);       // Set PLLB to CLK1, 8 mA output
  si5351_write(CLK2_CONTROL, 0x2F);       // Set PLLB to CLK2, 8 mA output
  si5351_write(PLL_RESET, 0xA0);          // Reset PLLA and PLLB

  // Set VCOs of PLLA and PLLB to 650 MHz
  a = PLLRATIO;     // Division factor 650/25 MHz !!!!
  b = 0;            // Numerator, sets b/c=0
  c = CFACTOR;      //Max. resolution, but irrelevant in this case (b=0)

  //Formula for splitting up the numbers to register data, see AN619
  p1 = 128 * a + (unsigned long) (128 * b / c) - 512;
  p2 = 128 * b - c * (unsigned long) (128 * b / c);
  //p3  = c;
  
  //Write data to registers PLLA and PLLB so that both VCOs are set to 900MHz intermal freq
  si5351_write(SYNTH_PLL_A, 0xFF);
  si5351_write(SYNTH_PLL_A + 1, 0xFF);
  si5351_write(SYNTH_PLL_A + 2, (p1 & 0x00030000) >> 16);
  si5351_write(SYNTH_PLL_A + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_A + 4, (p1 & 0x000000FF));
  si5351_write(SYNTH_PLL_A + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(SYNTH_PLL_A + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_A + 7, (p2 & 0x000000FF));

  si5351_write(SYNTH_PLL_B, 0xFF);
  si5351_write(SYNTH_PLL_B + 1, 0xFF);
  si5351_write(SYNTH_PLL_B + 2, (p1 & 0x00030000) >> 16);
  si5351_write(SYNTH_PLL_B + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_B + 4, (p1 & 0x000000FF));
  si5351_write(SYNTH_PLL_B + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(SYNTH_PLL_B + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_B + 7, (p2 & 0x000000FF));

}

void si5351_set_freq(int synth, unsigned long freq)
{
  unsigned long  a, b, c = CFACTOR; 
  unsigned long f_xtal = 25000000;
  double fdiv = (double) (f_xtal * PLLRATIO) / freq; //division factor fvco/freq (will be integer part of a+b/c)
  double rm; //remainder
  unsigned long p1, p2;
  
  a = (unsigned long) fdiv;
  rm = fdiv - a;  //(equiv. to fractional part b/c)
  b = rm * c;
  p1  = 128 * a + (unsigned long) (128 * b / c) - 512;
  p2 = 128 * b - c * (unsigned long) (128 * b / c);
      
  //Write data to multisynth registers of synth n
  si5351_write(synth, 0xFF);      //1048575 MSB
  si5351_write(synth + 1, 0xFF);  //1048575 LSB
  si5351_write(synth + 2, (p1 & 0x00030000) >> 16);
  si5351_write(synth + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(synth + 4, (p1 & 0x000000FF));
  si5351_write(synth + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(synth + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(synth + 7, (p2 & 0x000000FF));
}

int main(void)
{
	unsigned long f = 5000000;
		
    PORTC = 0xFF; //I�C-Bus lines: PC4=SDA, PC5=SCL
    
	//TWI	
	_delay_ms(100);
	twi_init();
	_delay_ms(100);
	
	si5351_start();
	si5351_set_freq(SYNTH_MS_0, f);
	
	for(;;)
    {
		
        
    }
	
	return 0;
}

