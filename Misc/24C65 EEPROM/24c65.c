///////////////////////////////////////////////////////////////////
//           24C65 EEPROM with ATmega32  16MHz                   //
///////////////////////////////////////////////////////////////////
//  Compiler:         GCC (GNU AVR C-Compiler)                   //
//  Author:           Peter Baier (DK7IH)                        //
//  Last change:      FEB 2017                                   //
///////////////////////////////////////////////////////////////////
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/twi.h>

#define CPUCLK 16

  ///////////////////////////
 //   EEPROM 24C65        //
///////////////////////////
#define EEPROMSIZE 8192
#define DEVICE_ADDR 0xA0

void eeprom24c65_write( int, unsigned char);
unsigned char eeprom24c65_read(int);

  ///////////////////////////
 //        T  W  I        //
///////////////////////////
void twi_init(void);
void twi_start(void);
void twi_stop(void);
void twi_write(unsigned char u8data);
unsigned char twi_read_nack(void);
unsigned char twi_read_ack(void);
unsigned char twi_get_status(void);

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

  ///////////////////////////
 //        T  W  I        //
///////////////////////////
void twi_init(void)
{
    //set SCL to 400kHz
    TWSR = 0x00;
    TWBR = 0x0C;
	
    //enable TWI
    TWCR = (1 << TWEN);
}

//Send start signal
void twi_start(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
    while ((TWCR & (1 << TWINT)) == 0);
}

//send stop signal
void twi_stop(void)
{
    TWCR = (1 << TWINT) | (1 << TWSTO)|(1 << TWEN);
}

void twi_write(uint8_t u8data)
{
    TWDR = u8data;
    TWCR = (1 << TWINT) | (1 << TWEN);
    while ((TWCR & (1 << TWINT)) == 0);
}

unsigned char twi_get_status(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}

unsigned char twi_read_nack(void)
{
    TWCR = (1 << TWINT)|(1 << TWEN);
    while ((TWCR & (1 << TWINT)) == 0);
    return TWDR;
}

unsigned char twi_read_ack(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while ((TWCR & (1<<TWINT)) == 0);
	return TWDR;
}

  ///////////////////////////
 //   EEPROM 24C65        //
///////////////////////////
void eeprom24c65_write(int mem_address, unsigned char value)
{
   int hi_addr = mem_address >> 8;
   int lo_addr = mem_address & 0xFF;
   	
   twi_start();
   twi_write(DEVICE_ADDR); //Device address
   twi_write(hi_addr);
   twi_write(lo_addr);
   twi_write(value);
   twi_stop();	
}	

unsigned char eeprom24c65_read(int mem_address)
{
   int hi_addr = mem_address >> 8;
   int lo_addr = mem_address & 0xFF;
   int r;
   	
   twi_start();
   twi_write(DEVICE_ADDR);     //Device address
   twi_write(hi_addr);
   twi_write(lo_addr);
   twi_start();
   twi_write(DEVICE_ADDR + 1); //Device address + 1
   r = twi_read_nack();
   twi_stop();	
   return r;
}	

int main(void)
{
    int t1, x1 = 0, x2 = 0;
    
    DDRD = 0x03; //Set PB0, PB1 for output (control LEDs)
    	
	//TWI
	twi_init();
	
    for(t1 = 0; t1 < EEPROMSIZE; t1++)
    {
		//Write EEPROM
        eeprom24c65_write(t1, t1);
        if(x1 > 255)
        {
			x1 = 0;
		}
		
		wait_ms(200);
		
		//Read EEPROM
		x2 = eeprom24c65_read(t1);
		wait_ms(200);
		
		//Compare
		if(x1 == x2)
		{
			PORTD &= ~(1 << PD0);
			PORTD |= (1 << PD1);
		}
		else	
		{
			PORTD |= (1 << PD0);
			PORTD &= ~(1 << PD1);
		}
		wait_ms(200);
		PORTD |= (1 << PD0);
		PORTD |= (1 << PD1);
	}
	
	return 0;
}

