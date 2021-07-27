///////////////////////////////////////////////////////////////////
//                      24C65 EEPROM                             //
///////////////////////////////////////////////////////////////////
//  Compiler:         GCC (GNU AVR C-Compiler)                   //
//  Author:           Peter Rachow (DK7IH)                       //
//  Last change:      FEB 2019 2017                              //
//Fuses: -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m                     //
///////////////////////////////////////////////////////////////////

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

#undef F_CPU 
#define F_CPU 80000000

  ///////////////////////////
 //   EEPROM 24C65        //
///////////////////////////
#define EEPROMSIZE 8192
void eeprom24c65_write(int, int, unsigned char);
unsigned char eeprom24c65_read(int, int);
void write_data(int, int);
unsigned char read_data(int);

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
void eeprom24c65_write(int device, int mem_address, unsigned char value)
{
   int hi_addr = mem_address >> 8;
   int lo_addr = mem_address & 0xFF;
   	
   twi_start();
   twi_write(device); //Device address
   twi_write(hi_addr);
   twi_write(lo_addr);
   twi_write(value);
   twi_stop();	
}	

unsigned char eeprom24c65_read(int device, int mem_address)
{
   int hi_addr = mem_address >> 8;
   int lo_addr = mem_address & 0xFF;
   int r;
   	
   twi_start();
   twi_write(device);     //Device address
   twi_write(hi_addr);
   twi_write(lo_addr);
   twi_start();
   twi_write(device + 1); //Device address + 1
   r = twi_read_nack();
   twi_stop();	
   return r;
}	

void write_data(int mem_addr, int value)
{
	if(mem_addr < EEPROMSIZE)
	{
		eeprom24c65_write(0xA0, mem_addr, value); //EPROM #1
	}
	else
	{
		eeprom24c65_write(0xA2, mem_addr - EEPROMSIZE, value); //EPROM #2
	}
}	

unsigned char read_data(int mem_addr)
{
	if(mem_addr < EEPROMSIZE)
	{
		return eeprom24c65_read(0xA0, mem_addr); //EPROM #1
	}
	else
	{
		return eeprom24c65_read(0xA2, mem_addr - EEPROMSIZE); //EPROM #2
	}
	
	return 0;
}	

int main(void)
{
    int t1, x = 0;
    	
	//TWI
	twi_init();
		
	//oled_putstring(0, 2, "Schreibe...          ", 0, 0);	
    for(t1 = 0; t1 < 16384; t1++)
    {
        write_data(t1, t1);
        //oled_putnumber(40, 5, t1, -1, 0, 0);
        if(x > 255)
        {
			x = 0;
		}
	}

	//oled_putstring(0, 2, "Lese...          ", 0, 0);
	
	for(t1 = 0; t1 < 16384; t1++)
    {
		//oled_putnumber(20, 6, t1, -1, 0, 0);
        //oled_putnumber(60, 6, read_data(t1), -1, 0, 0);
        _delay_ms(10);
	}
			
    
    for(;;)
    {
		
    }
	return 0;
}

