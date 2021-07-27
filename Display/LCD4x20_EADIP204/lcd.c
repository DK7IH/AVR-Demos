        ///////////////////////////////////////////////////////////////////
       //               ATMega644AP  and Line LCD 4x20  EADIP           //
     ///////////////////////////////////////////////////////////////////
    //                                                               //
   //  Compiler:         GCC (GNU AVR C-Compiler)                   //
  //  Autor:            Peter Rachow                               //
 //  Last modification: 2019-01-14                                //
///////////////////////////////////////////////////////////////////
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
#define LCD_DDR DDRD         //DDR for LCD D0:D3
#define LCD_PORT PORTD      //Port for LCD D0:D3
#define RS_PORT PORTD      //Port for LCD RS line
#define E_PORT PORTD      //Port for LCD E line
#define RW_PORT PORTD    //Port for LCD RW line
#define LCD_D0 1   //yellow
#define LCD_D1 2   //darkgreen
#define LCD_D2 4   //white
#define LCD_D3 8   //darkblue
#define LCD_RS 16  //darkblue
#define LCD_RW 32  //white
#define LCD_E  64  //green
#define LCD_BL 128 //violet

#define ICONSOLID 0x10
#define ICONOFF   0x00

//LCD hardware based functions
void lcd_write(char, unsigned char);
void lcd_init(void);
void lcd_cls(void);
void lcd_line_cls(int);
void lcd_putchar(int, int, unsigned char);
void lcd_putstring(int, int, char*);
int lcd_putnumber(int, int, long, int, int, char, char);
void defcustomcharacters(void);
int lcd_check_busy(void);

//LCD display for radio
void s_meter(int);
void show_frequency(long); 
void show_tx_frequency(long, int);
void show_pa_temp(int, int);
void show_voltage(int);
void show_vfo(int, int);

void show_sideband(int, int);
void lcd_set_icon(int, int);
void lcd_set_batt_icon(int);
void lcd_setbacklight(int);
void show_data(long, int, int, int, int, int, int, int);
void show_msg(char*);
void show_agc(int, int);
void show_tone(int, int);
void show_if_mode(int);

  ///////////////////////////////
 //  L   C   D   Module 4x20  //
///////////////////////////////

// Write CMD or DATA to LCD
void lcd_write(char lcdmode, unsigned char value)
{
    int t1;
    
   // while(lcd_check_busy());  //Check busy flag
    
	LCD_DDR |= 0x0F;          //Set DDR data lines as output
	RW_PORT &= ~(LCD_RW);     //Set RW to write operation, i. e. =0
	
    E_PORT &= ~(LCD_E);       //E=0
    if(!lcdmode)
	{
        RS_PORT &= ~(LCD_RS); //CMD
	}	
    else
	{
        RS_PORT |= LCD_RS;    //DATA
	}	
    
    //HI NIBBLE    
    E_PORT |= LCD_E; //E = 1
    for(t1 = 0; t1 < 4; t1++)
	{
	    if((value >> 4) & (1 << t1))
	    {
	       LCD_PORT |= (1 << t1);      
	    }
        else	
	    {
           LCD_PORT &= ~(1 << t1);     
	    }  
	}	
	E_PORT &= ~(LCD_E);
	_delay_ms(2);
	//LO NIBBLE
	E_PORT |= LCD_E;
	for(t1 = 0; t1 < 4; t1++)
	{
	    if(value & (1 << t1))
	    {
	       LCD_PORT |= (1 << t1);      
	    }
        else	
	    {
           LCD_PORT &= ~(1 << t1);     
	    }  
	}
    E_PORT &= ~(LCD_E);
    _delay_ms(2);
}


//Send one char to LCD
void lcd_putchar(int row, int col, unsigned char ch)
{
	//lcd_write(0, 0x80 + col + row * 0x40);
    lcd_write(0, 0x80 + col + row * 0x20);
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
	int t1;
		   
    // Basic settings of LCD
    // 4-Bit mode, 2 lines, 5 pixels width matrix
    lcd_write(0, 0x28);
        
    //4-line mode
    lcd_write(0, 0x2C); //RE=1
    lcd_write(0, 0x09);
    lcd_write(0, 0x28); //RE=0
        
    //Switch icons off
    lcd_write(0, 0x2C); //RE=1
    lcd_write(0, 0x40); //Set SEGRAM addr to 0x00
    for(t1 = 0; t1 < 15; t1++)
    {
		lcd_write(1, 0);
	}	
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
void lcd_set_batt_icon(int volts1)
{
	int v[] = {0x10, 0x18, 0x1C, 0x1E, 0x1F};
	int t1;
 
    for(t1 = 11; t1 < 15; t1++)
	{
	    if(volts1 >= t1 * 10 && volts1 < (t1 + 1) * 10)
		{
		    //Set icon
            lcd_write(0, 0x2C); //RE=1
            lcd_write(0, 0x40); //Set SEGRAM addr to 0x00
            lcd_write(0, 0x40 + 0x0F); //Select icon addr
	        lcd_write(1, v[t1 - 10]); //Activate icon in given mode
            lcd_write(0, 0x28); //RE=0
        }   
    }   
}	

int lcd_check_busy(void)
{
	unsigned char value;
	
	LCD_DDR &= ~(0x0F);    //LCD_PORT data line bits D0:D3 to rx mode
	   
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
		
	LCD_DDR |= 0x0F;         //Set DDR 0:3 as output
	
	return (value >> 8) & 1;
}  


//Set backlight
void lcd_setbacklight(int duty_cycle)
{
	double dc = 255 - (duty_cycle * 2.55);
	
	OCR2A = (int) dc;

}	

//Define chars
void defcustomcharacters(void)
{
    int i1;
    unsigned char adr=64;

    unsigned char customchar[]={0x0A, 0x0E, 0x0A, 0x00, 0x04, 0x04, 0x04, 0x00,  //HI
		                        0x08, 0x08, 0x0E, 0x00, 0x0E, 0x0A, 0x0E, 0x00,  //LO
		                        0x04, 0x0A, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,  //°
		                        0x11, 0x19, 0x15, 0x13, 0x11, 0x00, 0x00, 0x00,  //N
		                        0x00, 0x00, 0x00, 0x1E, 0x11, 0x1E, 0x14, 0x13,  //R
		                        0x1F, 0x13, 0x15, 0x15, 0x13, 0x15, 0x15, 0x1F}; //RX 
		                    
    lcd_write(0, 0);
    lcd_write(1, 0);

    //Send data to CGRAM in lcd
    for (i1 = 0; i1 < 48; i1++)
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
void show_frequency(long f)
{
	int row = 2, col = 11;
	lcd_putnumber(row, col, f / 100, -1, 1, 'l', 0);
	lcd_putstring(row, col + 6, "kHz");
}	

void show_tx_frequency(long f, int showmode) //Show TX freq for split mode
{
	int row, col;
	
	if(showmode)
	{
	    row = 2, col = 0;
	}
	else
	{
	    row = 3, col = 11;
	}   
	    
	if(f > 0)
	{
	    lcd_putnumber(row, col, f / 100, -1, 1, 'l', 0);
	    lcd_putstring(row, col + 6, "kHz");
	}
	else
	{
		lcd_putstring(row, col, "           ");
	}	
	
}	

void show_voltage(int v10)
{
	int row = 0, col = 15;
	lcd_putchar(row, col + lcd_putnumber(row, col, v10, -1, 1, 'l', 0), 'V');
}	

void show_vfo(int vfo,  int showmode)
{
	int row, col;
		
    if(showmode)
	{
		row = 2, col = 2;
	}
	else
	{	
		row = 0, col = 10;
	}   
	
	lcd_putstring(row, col, "VFO");
	lcd_putchar(row, col + 3, vfo + 0x80);
	
}	

void show_sideband(int sb, int showmode)
{
	int row, col;
	
	char *sidebandstr[] = {"LSB", "USB", "AM "};
	sidebandstr[2][2] = 5;
	    
	if(showmode)
	{
		row = 2, col = 9;
	}
	else
	{	
		row = 0, col = 6;
	}   
		
	lcd_putstring(row, col, sidebandstr[sb]);    
}

void show_pa_temp(int temp, int sensor)
{
	int row = 1, col = 0;
	char unitstr[] = {2, 'C', 0};
	
	lcd_putstring(row, col + lcd_putnumber(row, col, temp, -1, 1, 'l', 0), unitstr);
	lcd_putchar(row, col + 6, sensor + 16);
}
	
//Show meter value
void s_meter(int value)
{
	int c[] = {32, 212, 211, 210, 209, 208};
	int v, t1;
	int maxs = 35, row = 3, col = 0;
	
	//Clear meter
	if(value == -1)
	{
		for(t1 = 0; t1 < 7; t1++)	 
		{
			lcd_putchar(row, col + t1, 32);
		}
		return;
	}
			
	v = value;
	if(v > maxs)
	{
		v = maxs;
	}	
	
	//Full blocks first
	for(t1 = 0; t1 < v / 5; t1++)
	{
		lcd_putchar(row, col + t1, 208);
	}	
	
	//Remaining char
	if(col + t1 < 20)
	{
	     lcd_putchar(row, col + t1, c[v - (v / 5) * 5]);
	     lcd_putchar(row, col + t1 + 1, 32);
	}     
}

void show_msg(char *msgtxt)
{
	if(msgtxt[0] == 0)
	{
		lcd_putstring(2, 0, "       ");
		return;
	}
		
	lcd_putstring(2, 0, msgtxt);	
}	

void show_agc(int agcset, int showmode)
{
   	int row, col;
	
	if(showmode)
	{
		row = 2, col = 8;
	}
	else
	{	
		row = 0, col = 0;
	}   
	
	switch(agcset)
	{
	    case 0: lcd_putstring(row, col, "Fast");
	            break;
	    case 1: lcd_putstring(row, col, "Slow");
	            break;
	 }  
}		        

void show_tone(int toneset, int showmode)
{
   	int row, col;
	
	if(showmode)
	{
		row = 2, col = 8;
	}
	else
	{	
		row = 1, col = 16;
	}   
	
	switch(toneset)
	{
	    case 0: lcd_putstring(row, col, "LOW ");
				break;
	    case 1: lcd_putstring(row, col, "NORM");
				break;
	 }  
}		  

	
//Show all radio data at once
void show_data(long freq, int side_band, int pa_temp, int sensor, int vfo, int vdd, int agc_set, int tone_set)
{
    show_frequency(freq);  
    show_sideband(side_band, 0);
    show_pa_temp(pa_temp, sensor);
    show_vfo(vfo, 0);    
    show_voltage(vdd);
    lcd_set_batt_icon(vdd);
    show_agc(agc_set, 0);
    show_tone(tone_set, 0);
}

int main()
{        
    //OUTPUT PORT D	
    LCD_DDR |= 0xFF;    //LCD RS,RW, E, BACKLIGHT

    //Display TRX data
    lcd_init();
    _delay_ms(50);        
    lcd_cls();
    _delay_ms(50);        
    defcustomcharacters();
    _delay_ms(50);        
    show_data(0, 0, 300 / 10, 0, 0, 120, 0, 0);
    
	for(;;) 
	{
		lcd_putstring(0, 0, "HI!");

	}
    return 0;
}
 

