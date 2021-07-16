/*****************************************************************/
/*     BMP180 Temp & Pressure Sensor Demo-Demo with ATMega328    */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega328p 8 MHz                 */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*  Letzte Aenderung: 07-2021                                    */
/*****************************************************************/

#define F_CPU 8000000

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

  ///////////////////
 //  LCD-Display  //
///////////////////
#define LCD_DDR  DDRD
#define LCD_PIN PIND
#define LCD_PORT PORTD  //Port for LCD D0:D3
#define RS_PORT  PORTD  //Port for LCD RS line
#define RW_PORT  PORTD   //Port for LCD RW line
#define E_PORT   PORTD   //Port for LCD E line
#define LCD_D0 0
#define LCD_D1 0
#define LCD_D2 2
#define LCD_D3 3
#define LCD_RS 5
#define LCD_RW 6
#define LCD_E  7

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

//LCD
void lcd_write(char, unsigned char);
void lcd_write(char, unsigned char);
void lcd_init(void);
void lcd_cls(void);
void lcd_line_cls(int);
void lcd_putchar(int, int, unsigned char);
void lcd_putstring(int, int, char*);
int lcd_putnumber(int, int, long, int, int, char, char);
void lcd_display_test(void);
int lcd_check_busy(void);

//BMP180 variables
//calibration data
int ac1, ac2, ac3, b1, b2, mb, mc, md;
unsigned int ac4, ac5, ac6;
long b5;


// Write CMD or DATA to LCD
void lcd_write(char lcdmode, unsigned char value)
{
    int t1;
    
    while(lcd_check_busy()); //Check busy flag
        
	LCD_DDR = 0xFF;         //Set DDR as output
	RW_PORT &= ~(1 << LCD_RW);   //Set RW to write operation, i. e. =0
	
    E_PORT &= ~(1 << LCD_E);     //E=0
    if(!lcdmode)
	{
        RS_PORT &= ~(1 << LCD_RS); //CMD
	}	
    else
	{
        RS_PORT |= (1 << LCD_RS);   //DATA
	}	
    
    //HI NIBBLE    
    E_PORT |= (1 << LCD_E); //E = 1
    for(t1 = 0; t1 < 4; t1++)
	{
	    if(((value & 0xF0) >> 4) & (1 << t1))
	    {
	       LCD_PORT |= (1 << t1);      
	    }
        else	
	    {
           LCD_PORT &= ~(1 << t1);     
	    }  
	}	
	E_PORT &= ~(1 << LCD_E);
	
	//LO NIBBLE
	E_PORT |= (1 << LCD_E);
	for(t1 = 0; t1 < 4; t1++)
	{
	    if(value  & (1 << t1))
	    {
	       LCD_PORT |= (1 << t1);      
	    }
        else	
	    {
           LCD_PORT &= ~(1 << t1);     
	    }  
	}
    E_PORT &= ~(1 << LCD_E);
    _delay_ms(1);
}

int lcd_check_busy(void)
{
	unsigned char value;
	
	LCD_DDR &= ~(1 << LCD_D0); //LCD_PORT bits 0:3 on rx mode
	LCD_DDR &= ~(1 << LCD_D1);
	LCD_DDR &= ~(1 << LCD_D2);
	LCD_DDR &= ~(1 << LCD_D3);
	
    //LCD_PORT &= ~(0x01);
    RW_PORT |= (1 << LCD_RW);     //Read operation => RW=1
	
	RS_PORT &= ~(1 << LCD_RS); //CMD => RS=0: for busy flag
	
	//Read data
	//Hi nibble
	E_PORT |= (1 << LCD_E);          //E=1
    _delay_us(1);       
	value = (LCD_PIN & 0x0F) << 4;
    E_PORT &= ~(1 << LCD_E);       //E=0	
		
	//Lo nibble
	E_PORT |= (1 << LCD_E);          //E=1
    _delay_us(1);       
	value += (LCD_PIN & 0x0F);
    E_PORT &= ~(1 << LCD_E);       //E=0	
		
	LCD_DDR |= (1 << LCD_D0); //LCD_PORT bits 0:3 on rx mode
	LCD_DDR |= (1 << LCD_D1);
	LCD_DDR |= (1 << LCD_D2);
	LCD_DDR |= (1 << LCD_D3);
		
	return (value >> 8) & 1;
}  


//Send one char to LCD
void lcd_putchar(int row, int col, unsigned char ch)
{
    lcd_write(0, col + 128 + row * 0x40);
    lcd_write(1, ch);
}


//Print out \0-terminated string on LCD
void lcd_putstring(int row, int col, char *s)
{
    unsigned char t1 = col;

    while(*(s))
	{
        lcd_putchar(row, t1++, *(s++));
	}	
}

//Clear LCD
void lcd_cls(void)
{
    lcd_write(0, 1);
}


//Init LCD
void lcd_init(void)
{
		   
    // Basic settings of LCD
    // 4-Bit mode, 2 lines, 5x7 matrix
    lcd_write(0, 0x28);
    _delay_ms(2);
    lcd_write(0, 0x28);
    _delay_ms(2);
    
    // Display on, Cursor off, Blink off 
    lcd_write(0, 0x0C);
    _delay_ms(2);

    // No display shift, no cursor move
    lcd_write(0, 0x04);
    _delay_ms(2);
}

//Write an n-digit number (int or long) to LCD
int lcd_putnumber(int row, int col, long num, int digits, int dec, char orientation, char showplussign)
{
    char cl = col, minusflag = 0;
    unsigned char cdigit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, digitcnt = 0;
    long t1, t2, n = num, r, x = 1;

    if(num < 0)
    {
        minusflag = 1;
        n *= -1;
    }

    /* Stellenzahl automatisch bestimmen */
    if(digits == -1)
    {
        for(t1 = 1; t1 < 10 && (n / x); t1++)
		{
            x *= 10;
		}	
        digits = t1 - 1;
    }

    if(!digits)
        digits = 1;

    for(t1 = digits - 1; t1 >= 0; t1--)
    {
        x = 1;
        for(t2 = 0; t2 < t1; t2++)
            x *= 10;
        r = n / x;
        cdigit[digitcnt++] = r + 48;

        if(t1 == dec) 
            cdigit[digitcnt++] = 46;
        n -= r * x;
    }

    digitcnt--;
    t1 = 0;

    /* Ausgabe */
    switch(orientation)
    {
        case 'l':   cl = col;
                    if(minusflag)
                    {
                        lcd_putchar(row, cl++, '-');
                        digitcnt++;
                    }	 
		            else
		            {
		                if(showplussign)
			            {
			                lcd_putchar(row, cl++, '+');
                            digitcnt++;
			            } 
                    }	
			
                    while(cl <= col + digitcnt)                       /* Linksbuendig */
		            {
                        lcd_putchar(row, cl++, cdigit[t1++]);
					}	
                    break;

        case 'r':   t1 = digitcnt;                              /* Rechtsbuendig */
                    for(cl = col; t1 >= 0; cl--)              
					{
                        lcd_putchar(row, cl, cdigit[t1--]);
                        if(minusflag)	
						{
                            lcd_putchar(row, --cl, '-');
                        }
					}	
    }
	
    if(dec == -1)
	{
        return digits;
	}	
    else
	{
        return digits + 1;	
	}	
}	

void lcd_line_cls(int ln)
{
    int t1; 
	
	for(t1 = 0; t1 < 15; t1++)
	{
	    lcd_putchar(1, t1, 32);
	}
}	

//Define chars
void defcustomcharacters(void)
{
    int i1;
    unsigned char adr = 0x40;

    unsigned char customchar[]={0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, //S-Meter chars
	                            0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	                            0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C,  
	                            0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
	                            0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, //End S-Mater chars
	                            0x00, 0x00, 0x0E, 0x0E, 0x0E, 0x0E, 0x00, 0x00,
	                            0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00,
	                            0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E};
    lcd_write(0, 0);
    lcd_write(1, 0);

    //Send data to CGRAM in LCD
    for (i1 = 0; i1 < 64; i1++)
    {
        lcd_write(0, adr++);
        lcd_write(1, customchar[i1]);
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
		
	_delay_ms(100);
	
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
    x1 = (x1 * x1 * 3038L) >> 16;
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
		
	_delay_ms(100);
	
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

int main()
{
		
	//Set port for LCD
    LCD_DDR = 0xFF; //LCD_PORT Bits 0..6
	    		
	//Display
	_delay_ms(20); //Datasheet: "Wait for more than 15ms after VDD rises to 4.5V"
    lcd_init();
    lcd_cls();		
    
    defcustomcharacters();		
    	
    lcd_putstring(0, 0, "BMP180 DEMO");
    lcd_putstring(1, 0, "by DK7IH");
    
    _delay_ms(1000);
    lcd_cls();
    
    //Init Tempsensor & communication
    TWIInit();
	BMP180_get_cvalues();     		
         
    for(;;) 
	{
		lcd_putstring(0, 1 + lcd_putnumber(0, 0, BMP180_get_pressure(), -1, 3, 'l', 0), "hPa");
		lcd_putstring(1, 1 + lcd_putnumber(1, 0, BMP180_get_temp(), -1, 1, 'l', 0), "degC");
		_delay_ms(500);
	}
	return 0;
}

