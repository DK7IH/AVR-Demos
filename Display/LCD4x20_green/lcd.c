/*****************************************************************/
/*              LCD-Demo 4x20 with ATMega644p                    */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega328 8 MHz                  */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*  Letzte Aenderung:                                            */
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
#define LCD_DDR DDRD
#define LCD_PORT PORTD  //Port for LCD D0:D3
#define RS_PORT PORTD   //Port for LCD RS line
#define E_PORT PORTD    //Port for LCD E line
#define RW_PORT PORTD    //Port for LCD RW line
#define LCD_D0 1
#define LCD_D1 2
#define LCD_D2 4
#define LCD_D3 8
#define LCD_E 16
#define LCD_RW 32
#define LCD_RS 64


//LCD hardware based functions
void lcd_write(char, unsigned char);
void lcd_write(char, unsigned char);
void lcd_init(void);
void lcd_cls(void);
void lcd_line_cls(int);
void lcd_putchar(int, int, unsigned char);
void lcd_putstring(int, int, char*);
int lcd_putnumber(int, int, long, int, int, char, char);
void defcustomcharacters(void);
int lcd_check_busy(void);
void lcd_drawframe(int, int, int, int);

//LCD display for radio
void s_meter(int);
void show_frequency (unsigned long);
void show_voltage(int);
void show_vfo(int);
void show_mem(int);
void show_sideband(int);
void lcd_set_icon(int, int);
void lcd_set_batt_icon(int);
void show_msg(char*);
void show_pa_temp(int);

  ///////////////////////////////
 //   L   C   D   Module 2x8  //
///////////////////////////////

// Write CMD or DATA to LCD
void lcd_write(char lcdmode, unsigned char value)
{
    int t1;
    
    while(lcd_check_busy()); //Check busy flag
    
	LCD_DDR = 0x7F;         //Set DDR as output
	RW_PORT &= ~(LCD_RW);   //Set RW to write operation, i. e. =0
	
    E_PORT &= ~(LCD_E);     //E=0
    if(!lcdmode)
	{
        RS_PORT &= ~(LCD_RS); //CMD
	}	
    else
	{
        RS_PORT |= LCD_RS;   //DATA
	}	
    
    //HI NIBBLE    
    E_PORT |= LCD_E; //E = 1
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
	E_PORT &= ~(LCD_E);
	
	//LO NIBBLE
	E_PORT |= LCD_E;
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
    E_PORT &= ~(LCD_E);
    _delay_ms(1);
}


//Send one char to LCD
void lcd_putchar(int row, int col, unsigned char ch)
{
	int offs[] = {0x00, 0x40, 0x14, 0x54}; //Individual memory offset for each LCD line
	
	lcd_write(0, 0x80 + offs[row] + col);
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
    // 4-Bit mode, 5 pixels width matrix
    lcd_write(0, 0x28);
        
    //4-line mode
    lcd_write(0, 0x2C); //RE=1
    lcd_write(0, 0x09);
    lcd_write(0, 0x28); //RE=0
            
    // Display on, Cursor off, Blink off 
    lcd_write(0, 0x0C);
    
    // No display shift, no cursor move
    lcd_write(0, 0x04);
    
}

//Set icon by number, -1 switches allicons off
void lcd_set_icon(int icon, int status)
{
		
	//Set icon
    lcd_write(0, 0x2C); //RE=1
    lcd_write(0, 0x40); //Set SEGRAM addr to 0x00
    lcd_write(0, 0x40 + icon); //Select icon addr
	lcd_write(1, status); //Activate icon in given mode
	lcd_write(0, 0x28); //RE=0
}	

//Set icon by number, -1 switches allicons off
void lcd_set_batt_icon(int status)
{
	int v[] = {0x18, 0x1C, 0x1E, 0x1F};
		
	//Set icon
    lcd_write(0, 0x2C); //RE=1
    lcd_write(0, 0x40); //Set SEGRAM addr to 0x00
    lcd_write(0, 0x40 + 0x0F); //Select icon addr
	lcd_write(1, v[status]); //Activate icon in given mode
    lcd_write(0, 0x28); //RE=0

}	

int lcd_check_busy(void)
{
	unsigned char value;
	
	LCD_DDR = 0x70; //LCD_PORT bits 0:3 on rx mode
	   
	
    PORTC &= ~(0x01);
    RW_PORT |= LCD_RW;     //Read operation => RW=1
	
	RS_PORT &= ~(LCD_RS); //CMD => RS=0: for busy flag
	
	//Read data
	//Hi nibble
	E_PORT |= LCD_E;          //E=1
    _delay_us(1);       
	value = (PIND & 0x0F) << 4;
    E_PORT &= ~(LCD_E);       //E=0	
		
	//Lo nibble
	E_PORT |= LCD_E;          //E=1
    _delay_us(1);       
	value += (PIND & 0x0F);
    E_PORT &= ~(LCD_E);       //E=0	
		
	LCD_DDR = 0x7F;
	
	PORTC |= 0x01;   
	
	return (value >> 8) & 1;
}  

void lcd_drawframe(int r0, int c0, int r1, int c1)
{
	int t1;
	
	lcd_putchar(r0, c0, 5);
	lcd_putchar(r0, c1, 5);
	lcd_putchar(r1, c1, 5);
	lcd_putchar(r1, c0, 5);
	
	for(t1 = c0 + 1; t1 < c1; t1++)
	{
	    lcd_putchar(r0, t1, 6);
	    lcd_putchar(r1, t1, 6);
	}    
	
	for(t1 = r0 + 1; t1 < r1; t1++)
	{
	    lcd_putchar(t1, c0, 7);
	    lcd_putchar(t1, c1, 7);
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
    {
        digits = 1;
    }
    
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

//CLS for one LCD line
void lcd_line_cls(int ln)
{
    int t1;
	
	for(t1 = 0; t1 < 15; t1++)
	{
	    lcd_putchar(1, t1, 32);
	}
}	



  ///////////////////////////////
 //   TRX display functions   //
///////////////////////////////

//Display current frequency divide by 100 with
//dec. separator
void show_frequency(unsigned long f)
{
	int row = 2, col = 10;
	lcd_putnumber(row, col, f / 100, -1, 1, 'l', 0);
	lcd_putstring(row, col + 6, "kHz");
}	

void show_voltage(int v10)
{
	int row = 1, col = 0;
	lcd_putchar(row, col + lcd_putnumber(row, col, v10, -1, 1, 'l', 0), 'V');
}	

void show_vfo(int vfo)
{
	int row = 0, col = 0;
		
	lcd_putstring(row, col, "VFO ");
	lcd_putchar(row, col + 3, vfo + 65);
}

void show_mem(int m)
{
	int row = 0, col = 5;
		
	lcd_putstring(row, col, "M");
	lcd_putchar(row, col + 1, m + 48);
}

void show_sideband(int sb)
{
	int row = 0, col = 9;
	char *sidebandstr[] = {"LSB", "USB"};
	
	lcd_putstring(row, col, sidebandstr[sb]);
}

void show_pa_temp(int temp)
{
	int row = 0, col = 14;
	char unitstr[] = {0xB2, 'C', 0};
	
	lcd_putstring(row, col + lcd_putnumber(row, col, temp, -1, 1, 'l', 0), unitstr);
}

void show_msg(char* txt)
{
	int row = 2, col = 0;
		
	lcd_putstring(row, col, txt);
}

	
//Show meter value
void s_meter(int value)
{
	int c[] = {32, 0, 1, 2, 3, 4};
	int v, t1;
	int maxs = 30, row = 3, col = 0;
		
	v = value;
	if(v > maxs)
	{
		v = maxs;
	}	
	
	//Full blocks first
	for(t1 = 0; t1 < v / 5; t1++)
	{
		lcd_putchar(row, col + t1, 4);
	}	
	
	//Remaining char
	if(col + t1 < 20)
	{
	     lcd_putchar(row, col + t1, c[v - (v / 5) * 5]);
	}     
}
	
int main()
{
	int t1;	
	
	//Set port for LCD
	DDRD = 0xFF;
	
	DDRC = 0x01; //Check LED
	
		    		
	//Display
	_delay_ms(20); //Datasheet: "Wait for more than 15ms after VDD rises to 4.5V"
    lcd_init();
    
    lcd_cls();
    
    defcustomcharacters();		

    show_vfo(0);   
    show_mem(0);
    show_sideband(1);
    show_frequency(14200000);  		    	
    show_voltage(124);
    show_msg("OK.");

    
    show_pa_temp(301);
    
    //lcd_set_icon(7, ICONSOLID);        
    //lcd_set_batt_icon(1);
    lcd_drawframe(1, 9, 3, 19);        
    
    for(;;) 
	{
		
		for(t1 = 0; t1 < 30; t1++)
		{
			s_meter(t1);
		}	
        for(t1 = 30; t1 > 0; t1--)
		{
			s_meter(t1);
		}
		
	}
	return 0;
}

