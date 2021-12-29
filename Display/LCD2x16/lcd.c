///////////////////////////////////////////////////////////////////
/*  LCD-Demo 2x16 DOT Matrix with ATMega32                       */
///////////////////////////////////////////////////////////////////
/*  Mikrocontroller:  ATMEL AVR ATmega32 16 MHz                  */
///////////////////////////////////////////////////////////////////
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*  Letzte Aenderung:                                            */
///////////////////////////////////////////////////////////////////
#define CPUCLK 16

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>

int main(void);

  ///////////////////
 //  LCD-Display  //
///////////////////
#define LCD_DDR  DDRD
#define LCD_PIN PIND
#define LCD_PORT PORTD   //Port for LCD D0:D3
#define RS_PORT  PORTD   //Port for LCD RS line
#define RW_PORT  PORTD   //Port for LCD RW line
#define E_PORT   PORTD   //Port for LCD E line
#define LCD_D0 0
#define LCD_D1 1
#define LCD_D2 2
#define LCD_D3 3
#define LCD_RS 4
#define LCD_RW 5
#define LCD_E  6

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
void wait_ms(int);

// Wartezeit in Millisekunden bei fck = 8.000 MHz 
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

// Write CMD or DATA to LCD
void lcd_write(char lcdmode, unsigned char value)
{
    int t1;
        
    while(lcd_check_busy());    //Check busy flag
        
	LCD_DDR |= 0x0F;            //Set DDR as output
	RW_PORT &= ~(1 << LCD_RW);  //Set RW to write operation, i. e. =0
	
    E_PORT &= ~(1 << LCD_E);        //E=0
    if(!lcdmode)
	{
        RS_PORT &= ~(1 << LCD_RS);  //CMD
	}	
    else
	{
        RS_PORT |= (1 << LCD_RS);   //DATA
	}	
    wait_ms(2);	
        
    //HI NIBBLE    
    E_PORT |= (1 << LCD_E);          //E = 1
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
}

int lcd_check_busy(void)
{
	unsigned char value;
	
	LCD_DDR &= ~(0x0F);           //LCD_PORT bits 0:3 on rx mode
	
    RW_PORT |= (1 << LCD_RW);     //Read operation => RW=1
	
	RS_PORT &= ~(1 << LCD_RS);    //CMD => RS=0: for busy flag
	
	//Read data
	//Hi nibble
	E_PORT |= (1 << LCD_E);         //E=1
    wait_ms(1);       
	value = (LCD_PIN & 0x0F) << 4;
    E_PORT &= ~(1 << LCD_E);        //E=0	
		
	//Lo nibble
	E_PORT |= (1 << LCD_E);         //E=1
    wait_ms(1);       
	value += (LCD_PIN & 0x0F);
    E_PORT &= ~(1 << LCD_E);        //E=0	
		
	LCD_DDR |= (0x0F);              //LCD_PORT bits 0:3 on rx mode
		
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
    wait_ms(1);
    lcd_write(0, 0x28);
    wait_ms(1);
    lcd_write(0, 0x28);
    wait_ms(1);
    
    // Display on, Cursor off, Blink off 
    lcd_write(0, 0x0C);
    wait_ms(1);

    // No display shift, no cursor move
    lcd_write(0, 0x04);
    wait_ms(1);
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

    unsigned char customchar[]={0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, //Various bars
	                            0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	                            0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C,  
	                            0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 
	                            0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 
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

int main()
{
	LCD_DDR |= (1 << LCD_D0) | (1 << LCD_D1) | (1 << LCD_D2) | (1 << LCD_D3);
	LCD_DDR |= (1 << LCD_RS) | (1 << LCD_RW) | (1 << LCD_E); //LCD_PORT pins 0..6 as output
	    		
	//Display
	wait_ms(20); //Datasheet: "Wait for more than 15ms after VDD rises to 4.5V"
    lcd_init();
    defcustomcharacters();
    lcd_cls();
    
    lcd_putstring(0, 0, "LCD162 driver");
    lcd_putstring(1, 0, "micromaker.de");
        
    for(;;) 
	{
	}
	return 0;
}
