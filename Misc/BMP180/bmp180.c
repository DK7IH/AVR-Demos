/*****************************************************************/
/*     BMP180 Temp & Pressure Sensor Demo  with ATMega32         */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega32 16MHz                   */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Baier                                */
/*  Letzte Aenderung: 07-2016                                    */
/*****************************************************************/
//This examples uses UART to output measured data.
//Parameters are 9600-8-N-1
//Data can be read on PD0

#define CPUCLK 16

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

int main(void);

//I²C
void TWIInit(void);
void TWIStart(void);
void TWIStop(void);
uint8_t TWIReadACK(void);
uint8_t TWIReadNACK(void);
uint8_t TWIGetStatus(void);

//BMP180 functions
long BMP180_get_temp(void);
void BMP180_get_cvalues(void);
long BMP180_get_pressure(void);

int int2asc(long, int, char*, int);

//BMP180 variables
//calibration data
int ac1, ac2, ac3, b1, b2, mb, mc, md;
unsigned int ac4, ac5, ac6;
long b5;

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

/************************/
//
//         TWI
//
/************************/
void TWIInit(void)
{
    //set SCL to 400kHz
    TWSR = 0x00;
    TWBR = 0x0C;
    //enable TWI
    TWCR = (1<<TWEN);
}

void TWIStart(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

//send stop signal
void TWIStop(void)
{
    TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void TWIWrite(uint8_t u8data)
{
    TWDR = u8data;
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
}

uint8_t TWIReadACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}

//read byte with NACK
uint8_t TWIReadNACK(void)
{
    TWCR = (1<<TWINT)|(1<<TWEN);
    while ((TWCR & (1<<TWINT)) == 0);
    return TWDR;
}

uint8_t TWIGetStatus(void)
{
    uint8_t status;
    //mask status
    status = TWSR & 0xF8;
    return status;
}

/************************/
//
//    TEMP AND PRESSURE
//
/************************/
//Calculate current air pressure
long BMP180_get_pressure(void)
{
    
	long up;
	long x1, x2, x3;
	long b3, b6, p;		
	unsigned long b4, b7;
	int oss = 0;
	//char *buf = malloc(16);
	
	//Start
	TWIStart();
	TWIWrite(0xEE); //Device adress and set WRITE mode
	TWIWrite(0xF4); //Register adress
	TWIWrite(0x34); //Register value = read pressure oss = 0!
	TWIStop();
		
	wait_ms(100);
	
	TWIStart();     //Restart
	TWIWrite(0xEE); //Device adress and WRITE mode
	TWIWrite(0xF6); //Specify register (0xF6(MSB), 0xF7 (LSB))
	
	//Get value 
	//Restart first
	TWIStart();
	TWIWrite(0xEF); //Send device adress and set READ mode
		
	//Copy data to buffer
	up  = (long) TWIReadACK() << 8;  //MSB
    up |= (long) TWIReadNACK();      //LSB
	
	TWIStop();
		
	//Calculate real pressure
	b6 = b5 - 4000L;
    x1 = (b2 * ((b6 * b6) >> 12)) >> 11;
    x2 = (ac2 * b6) >> 11;
    x3 = x1 + x2;
    b3 = (((ac1 * 4 + x3) << oss) + 2) >> 2;
    x1 = (ac3 * b6) >> 13;
    x2 = (b1 * ((b6 * b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (ac4 * ((unsigned long) x3 + 32768L)) >> 15;
    b7 = ((unsigned long)up - b3) * (50000L >> oss);
	p = (b7 < 0x80000000UL) ? ((b7 << 1) / b4) : ((b7 / b4) << 1);
    x1 = (p >> 8) * (p >> 8); //???
    x1 = (x1 * 3038L) >> 16;
    x2 = (-7357L * p) >> 16;
    p += ((x1 + x2 + 3791L) >> 4);
	
	return (p);
	
}

//Read current temperature from BMP180
long BMP180_get_temp(void)
{
    
	long tmp;
	long x1, x2;
			
	//Start
	TWIStart();
	TWIWrite(0xEE); //Device adress and set WRITE mode
	TWIWrite(0xF4); //Register adress
	TWIWrite(0x2E); //Register value = read temperature!
	TWIStop();
		
	wait_ms(100);
	
	TWIStart();     //Restart
	TWIWrite(0xEE); //Device adress and WRITE mode
	TWIWrite(0xF6); //Specify register (0xF6(MSB), 0xF7 (LSB))
	
	//Get value 
	//Restart first
	TWIStart();
	TWIWrite(0xEF); //Send device adress and set READ mode
		
	//Copy data to buffer
	tmp  = (long) TWIReadACK() << 8;  //MSB
    tmp |= (long) TWIReadNACK();      //LSB
	
	TWIStop();
	
	//Calculate temperature
	x1 = ((tmp - ac6) * ac5) >> 15;
	x2 = ((int32_t)mc << 11) / (x1 + md);
	b5 = x1 + x2;

	return (((x1 + x2) + 8) >> 4);
}


//Get the calibration values from BMP180 sensor
void BMP180_get_cvalues(void)
{
    int BMP180_regaddress[11] = {0xAA, 0xAC, 0xAE, 0xB0, 0xB2, 0xB4, 0xB6, 0xB8, 0xBA, 0xBC, 0xBE};
	int BMP180_coeff[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int t1 = 0;
		
	for(t1 = 0; t1 < 11; t1++)
	{
        TWIStart();
	    TWIWrite(0xEE); //Device adress and set WRITE mode
	    TWIWrite(BMP180_regaddress[t1]); //Specify register (0xF6(MSB), 0xF7 (LSB))
	    //Restart first
	    TWIStart();
	    TWIWrite(0xEF); //Send device adress and set READ mode
	    BMP180_coeff[t1] = TWIReadACK() << 8;  //MSB
	    BMP180_coeff[t1] |= TWIReadNACK();  //LSB
	    TWIStop();
    }
	
	ac1 = BMP180_coeff[0];
    ac2 = BMP180_coeff[1];
    ac3 = BMP180_coeff[2];
    ac4 = BMP180_coeff[3];
    ac5 = BMP180_coeff[4];
    ac6 = BMP180_coeff[5];
 
    b1  = BMP180_coeff[6];
    b2  = BMP180_coeff[7]; 
 
    mb  = BMP180_coeff[8];
    mc  = BMP180_coeff[9];
    md  = BMP180_coeff[10];
}

/////////////////////////////////
//
// STRING FUNCTIONS
//
////////////////////////////////
//INT 2 ASC
int int2asc(long num, int dec, char *buf, int buflen)
{
    int i, c, xp = 0, neg = 0;
    long n, dd = 1E09;

    if(!num)
	{
	    *buf++ = '0';
		*buf = 0;
		return 1;
	}	
		
    if(num < 0)
    {
     	neg = 1;
	    n = num * -1;
    }
    else
    {
	    n = num;
    }

    //Fill buffer with \0
    for(i = 0; i < 12; i++)
    {
	    *(buf + i) = 0;
    }

    c = 9; //Max. number of displayable digits
    while(dd)
    {
	    i = n / dd;
	    n = n - i * dd;
	
	    *(buf + 9 - c + xp) = i + 48;
	    dd /= 10;
	    if(c == dec && dec)
	    {
	        *(buf + 9 - c + ++xp) = '.';
	    }
	    c--;
    }

    //Search for 1st char different from '0'
    i = 0;
    while(*(buf + i) == 48)
    {
	    *(buf + i++) = 32;
    }

    //Add minus-sign if neccessary
    if(neg)
    {
	    *(buf + --i) = '-';
    }

    //Eleminate leading spaces
    c = 0;
    while(*(buf + i))
    {
	    *(buf + c++) = *(buf + i++);
    }
    *(buf + c) = 0;
	
	return c;
} 

void uart_send_string(char *s2)
{
	while(*s2)
	{
		while (!(UCSRA & (1<<UDRE)))  /* warten bis Senden moeglich                   */
        {
        }
		UDR = *s2++; 
	}	
}	

int main()
{
	char *s, *str_crlf;
	
	s = malloc(255);
	str_crlf = malloc(8);
	str_crlf[0] = 13;
	str_crlf[1] = 10;
	str_crlf[2] = 0;
		
	//Init UART	
	UCSRB |= (1<<TXEN);                           // UART TX enable
    UCSRC = (1<<URSEL)|(1 << UCSZ1)|(1 << UCSZ0); // Parameters: Asynchronous transmission, 8N1
    UBRRH = 103 >> 8;   //9600 Bd.
    UBRRL = 103 & 0xFF;
	
    wait_ms(1000);
        
    //Init Tempsensor & communication
    TWIInit();
	BMP180_get_cvalues();     		
         
    for(;;) 
	{
		int2asc(BMP180_get_pressure(), -1, s, 128);
		strcat(s, "hPa");
		uart_send_string("Air pressure: "); 
		uart_send_string(s); 
	    uart_send_string(str_crlf); 
		wait_ms(2000);
		int2asc(BMP180_get_temp(), 1, s, 128);
		strcat(s, "°C");
		uart_send_string("Temperature: "); 
		uart_send_string(s);
		uart_send_string(str_crlf);  
		wait_ms(2000);
	}
	return 0;
}
