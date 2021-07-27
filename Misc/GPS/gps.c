/*****************************************************************/
/*                    GPS with ATMega32                          */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega32, 8 MHz                  */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow  2018                         */
/*  Fuses: -U lfuse:w:0xe4:m -U hfuse:w:0xd9:m                   */
/*****************************************************************/
// This code reads data from a GPS module via RS232 interface and 
// decodes the basic NMEA string of $GPRMC type, the so-called
// "RMC" (Recommended Minimum Specific GNSS Data) string.
// Displayed are Date, Time, QTH-Locator (Maidenhead), Lattitude and 
// Longitude and Ground speed
/*   PORTS */
// O U T P U T 
// LCD 
// RS      = PC2
// E       = PC3
// D4...D7 = PD4..PD7

// PD0 <=> TX at GPS module
// PD1 <=> RX at GPS module
 
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

/***************/
/* LCD-Display */
/***************/
#define LCD_INST 0x00
#define LCD_DATA 0x01

void lcd_write(char, unsigned char);
void lcd_write(char, unsigned char);
void set_rs(char);
void set_e(char);
void lcd_init(void);
void lcd_cls(void);
void lcd_line_cls(int);
void lcd_putchar(int, int, unsigned char);
void lcd_putstring(int, int, char*);
void lcd_putnumber(int, int, long, int);
void lcd_display_test(void);
void setcustomcharacters(void);

/*******************/
/* Stringhandling */
/*******************/
int strlen(char *s);
int instring(char*, char*);
int strcomp(char*, char*);
void int2asc(long, int, char*, int);

/*******************/
/*   U  A  R  T    */
/*******************/
void uart_init(void);
void uart_putc(char);
void uart_send_string(char*);
void init_rx_buffer(void);
char make_crc(int, int);

/**********************/
/* V A R I A B L E S  */
/**********************/
//USART defines & variables
#define RX_BUF_SIZE 64
char rx_buf[RX_BUF_SIZE];
int rx_buf_cnt = 0;

/*****************************/
/* Result string formatting  */
/*****************************/
void get_time(char*, char*);
int get_receiver_status(char*);
void get_latitude(char*, char*);
void get_longitude(char*, char*);
void get_latitude_ns(char*, char*);
void get_longitude_ew(char*, char*);
double get_gps_coordinate_decimal(char*, int, char*);
void get_ground_speed(char*, char*);
void get_ground_speed(char*, char*);
void get_date(char*, char*);
void calc_maidenhead(double, char, double, char, char*);

/**************************************/
/*            L  C  D                 */
/**************************************/
/* Send one byte to LCD */
void lcd_write(char lcdmode, unsigned char value)
{
    int x = 16, t1;
	
    set_e(0); 

    if(!lcdmode)
	{
        set_rs(0);
	}	
    else
	{
        set_rs(1);
	}	

    _delay_ms(1);
	
    set_e(1);
    
    /* Hi */
	for(t1 = 0; t1 < 4; t1++)
	{
	    if(value & x)
	    {
	       PORTD |= x;
	    }
        else	
	    {
           PORTD &= ~(x);     
	    }  
		
		x *= 2;
	}	
	set_e(0);
	
	x = 16;

	set_e(1);
	/* Lo */
	for(t1 = 0; t1 < 4; t1++)
	{
	    if((value & 0x0F) * 16 & x)
	    {
	       PORTD |= x;              
	    }
        else	
	    {
           PORTD &= ~(x);          
	    }  
		
		x *= 2;
	}

    set_e(0);
}

/* RS */
void set_rs(char status)
{
    if(status)
	{
        PORTC |= (1 << PC2);
	}	
    else
	{
	    PORTC &= ~(1 << PC2);
	}	
}

/* E */
void set_e(char status)
{
    if(status)
	{
        PORTC |= (1 << PC3);
	}	
    else
	{
	    PORTC &= ~(1 << PC3);
	}	
}

void lcd_putchar(int row, int col, unsigned char ch)
{
    lcd_write(LCD_INST, col + 128 + row * 0x40);
    lcd_write(LCD_DATA, ch);
}


void lcd_putstring(int row, int col, char *s)
{
    unsigned char t1;

    for(t1 = col; *(s); t1++)
	{
        lcd_putchar(row, t1, *(s++));
	}	
}

void lcd_putnumber(int y, int x, long number, int dec)
{
	char *buf;
	
	buf = malloc(32);
	int2asc(number, dec, buf, 31);
	lcd_putstring(y, x, buf);
	free(buf);
}	
	
void lcd_cls(void)
{
    lcd_write(LCD_INST, 1);
}

void lcd_init(void)
{
    
    lcd_write(LCD_INST, 40);

    //Matrix 5*7
    lcd_write(LCD_INST, 8);

    /* Display on, Cursor off, Blink off */
    /* Entrymode !cursoincrease + !displayshifted */
    lcd_write(LCD_INST, 12);
      	
	//4-Bit-Mode
    lcd_write(LCD_INST, 2);	
	
	lcd_cls();
}

void lcd_line_cls(int ln)
{
    int t1;
	
	for(t1 = 0; t1 < 15; t1++)
	{
	    lcd_putchar(1, t1, 32);
	}
}	


//Define own chars
void setcustomcharacters(void)
{
    int i1;
    unsigned char adr=64;

    unsigned char customchar[]={ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     // \0
		                         0x04, 0x0A, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};    //°-sign
    lcd_write(LCD_INST, 0);
    lcd_write(LCD_DATA, 0);

    //Send data to CGRAM in lcd
    for (i1 = 0; i1 < 16; i1++) 
    {
        lcd_write(LCD_INST, adr++);
        lcd_write(LCD_DATA, customchar[i1]);
    }
}

/***********************/
//
// STRING-FUNCTIONS
//
/**********************/
//Convert int number to string
void int2asc(long num, int dec, char *buf, int buflen)
{
    int i, c, xp = 0, neg = 0;
    long n, dd = 1E09;
    
    //Write 0 to buffer in case value == 0
    if(!num)
    {
		buf[0] = '0';
		buf[1] = 0;
		return;
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
	        *(buf + 9 - c + ++xp) = ',';
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
}

//Compare 2 strings
int strcomp(char *s1, char *s2)
{
    int t1;
    for(t1 = 0; t1 < strlen(s2); t1++)
    {
        if(*(s1 + t1) != *(s2 + t1))
	    {
	       return 0;
		}
    }
    
    return 1;   
}

//Get length of string
int strlen(char *s)
{
   int t1 = 0;

   while(*(s + t1++));

   return (t1 - 1);
}

//Find s2 in s1 if present, hand back position in string if yes
int instring(char *s1, char *s2)
{
   int t1, t2, ok;

   for(t1 = 0; *(s1 + t1) ; t1++)
   {
	    ok = 1;
	    for(t2 = 0; t2 < strlen(s2); t2++)
	    {
	  
	        if(*(s1 + t1 + t2) != *(s2 + t2))
	        {
	            ok = 0;
	        }
	    }
	    
		if(ok)
	    {
	        return t1;
	    }
    }

    return 0;
}

//************/
//    UART
//************/
//Init UART
void uart_init()
{
    /* 9600 Baud */
    UBRRL = 51; 
    UBRRH = 0;

    /* RX and TX on */
    UCSRB = (1<<RXEN)|(1<<TXEN);    

    /* 8 Datenbits, 1 Stopbit, keine Paritaet */
    UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0);  
}

//Send 1 char to UART
void uart_putc(char tx_char)
{
    while(!(UCSRA & (1<<UDRE)));

    UDR = tx_char;
}

//Send one string to UART
void uart_send_string(char *s)
{
    int t1 = 0;
	while(*(s + t1))
	{
	    uart_putc(*(s + t1++));
	}	
	uart_putc(13);
	uart_putc(10);

}

uint8_t uart_getc(void)
{
    while(!(UCSRA & (1<<RXC)));   // warten bis Zeichen verfuegbar
    return UDR;                   // Zeichen aus UDR an Aufrufer zurueckgeben
}

//Init RX buffer
void init_rx_buffer(void)
{
	int t1;
	
    for(t1 = 0; t1 < RX_BUF_SIZE - 1; t1++)
    {
		rx_buf[t1] = 0;
	}
	rx_buf_cnt = 0;
}		

/* CRC-calculation */
char make_crc(int buflen, int addchar)
{
    int t1, x = 0;

    for(t1 = 0; t1 < buflen; t1++) /* Puffer bis dato */
	{
        x = x ^ rx_buf[t1]; 
    }
	x = x ^ addchar;                /* Sendebyte */

    return x;
}
		
///////////////////// END UART ///////////////

//////////////////////////////
//                          //
// NMEA decode functions    //
//                          //
//////////////////////////////

//Format date string
void get_date(char *buffer2, char *buffer3)
{
	
    int t1, t2 = 0;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 9th datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 9 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 10 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
		
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		if((t1 - p1) == 2 || (t1 - p1) == 4)
		{
			*(buffer3 + t2++) = '.';
		}	
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
}	

//Format time string
void get_time(char *buffer2, char *buffer3)
{
	
    int t1, t2 = 0;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find first datafield
	t1 = 0;
	while(*(buffer2 + t1) != ',' && t1 < RX_BUF_SIZE)
	{
		t1++;
	}
	p1 = t1 + 1;
	
	//Find dot in timestring
	t1 = p1 + 1;
	while(*(buffer2 + t1) != '.' && t1 < RX_BUF_SIZE)
	{
		t1++;
	}
	p2 = t1;
		
	//Copy relevant prt of string to buffer3	
    for(t1 = p1; t1 < p2; t1++)	
    {
		if(t1 == 9 || t1 == 11)
		{
			*(buffer3 + t2++) = ':';
		}	
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
}	

//Format receiver status string
int get_receiver_status(char *buffer)
{
	int t1, t2 = 0;
    int p1;
        
	//Find second datafield
	t1 = 0;
	while(t2 != 2 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	if(*(buffer + p1) == 'A')
    {
		return 0;
	}
	else
	{
		return 1;
	}	
}	

//Format latitude to degrees
void get_latitude(char *buffer2, char *buffer3)
{
	
    int t1, t2;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 3rd datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 3 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 4 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
	
	//No data available
	if(p2 - p1 == 1)
	{
		return;
	}	
	
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		if(t1 == p1 + 2)
		{
			*(buffer3 + t2++) = 1; //°
		}	
			
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
}	

//Format latitude orientation
void get_latitude_ns(char *buffer2, char *buffer3)
{
	
    int t1, t2;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 4th datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 4 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 5 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
		
	//No data available
	if(p2 - p1 == 1)
	{
		return;
	}	
	
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
	
}	

//Format longitude
void get_longitude(char *buffer2, char *buffer3)
{
	
    int t1, t2;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 5th datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 5 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 6 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
		
	//No data available
	if(p2 - p1 == 1)
	{
		return;
	}	
	
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		if(t1 == p1 + 3)
		{
			*(buffer3 + t2++) = 1; //°
		}	
			
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
	
}	

//Format longitude orientation
void get_longitude_ew(char *buffer2, char *buffer3)
{
	
    int t1, t2;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 5th datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 6 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 7 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
	
	//No data available
	if(p2 - p1 == 1)
	{
		return;
	}	
	
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
	
}	

//Return the floating point value of a GPS coordinate (latitude or 
//langitude) - Parameters: ctype==0: longitude, ctype==1 => lattiude
double get_gps_coordinate_decimal(char *buf, int ctype, char *retbuf)
{
	int t1, t2;
	int sp; //Position of 1st comma of relevant data field
	double rval = 0;
	double x;
	
	char *l_str = malloc(20);
			
	if(!ctype)
	{
		sp = 5;  //Longitude
	}
	else
	{
		sp = 3; //Lattitude
	}	
				
	//Init temporary string
	for(t1 = 0; t1 < 20; t1++)
	{
		*(l_str + t1) = 0;
	}
	
	//Search start position of value (3rd or 5th ',')
	t1 = 0; t2 = 0;
	while(buf[t1] != 0 && t2 != sp)
	{
		if(buf[t1] == ',')
		{
			t2++;
		}	
		t1++;
	}
	sp = t1;	
			
	//Load relevant part of original string to new bufferstring
	t2 = 0;
	for(t1 = sp; *(buf + t1) != ','; t1++)
	{
		*(l_str + t2++) = buf[t1]; 
	}

    //Check multiplier			
	if(!ctype)
	{
		x = 100;
	}
	else
	{
		x = 10;
	}
	
	//Convert string to FP number
	for(t1 = 0; l_str[t1] != 0; t1++)
	{
		if(l_str[t1] != '.')
		{
		    rval += (double)(l_str[t1] - 48) * x;
		    x /= 10;
		}
	}
	
	free(l_str);
	
	//Get orientation indicator (N/S or W/E)
	if(!ctype)
	{
		sp = 6; //Longitude
	}
	else
	{
		sp = 4; //Lattitude
	}	
			
	//Search start position (4th or 6th ',')
	//where letter is expected
	t1 = 0; t2 = 0;
	while(buf[t1] != 0 && t2 != sp)
	{
		if(buf[t1] == ',')
		{
			t2++;
		}	
		t1++;
	}
	sp = t1;	
		
	retbuf[0] = buf[sp];
			
	return rval;
}	

//Ground speed
void get_ground_speed(char *buffer2, char *buffer3)
{
	
    int t1, t2;
    int p1, p2;
    
    //Zero buffer3
    for(t1 = 0; t1 < RX_BUF_SIZE; t1++)	
    {
		*(buffer3 + t1) = 0;
	}
	
	//Find 5th datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 7 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p1 = t1;
	
	//Find end of datafield
	t1 = 0;
	t2 = 0;
	while(t2 != 8 && t1 < RX_BUF_SIZE)
	{
		if(*(buffer2 + t1) == ',')
		{
			t2++;
		}	
		t1++;
	}
	p2 = t1;
		
	//No data available
	if(p2 - p1 == 1)
	{
		return;
	}	
	
	//Copy relevant prt of string to buffer3	
	t2 = 0;
    for(t1 = p1; t1 < p2 - 1; t1++)	
    {
		*(buffer3 + t2++) = *(buffer2 + t1);
	}	
	
}	

//Calc QTH locator from decimal value for latitude and
//longitude. o1 and o2 are signifier for 'E or 'W' (longitude) resp. 
//'N' or 'S' for latitude
//example for calling: calc_maidenhead(8.002, 'E', 49.1002, 'N', buf);
void calc_maidenhead(double longitude, char o1, double latitude, char o2, char *buffer)
{
    
    //Longitude
    int deg_lo, lo1, lo2, lo3;
    double deg10_lo;
    
    //Lattitude
    int deg_la, la1, la2, la3;
    double deg10_la;
    
    deg_lo = longitude; //Truncate values for longitude and latitude
    deg_la = latitude;
    
    deg10_lo = (longitude - deg_lo) * 12; //Calculate fraction behind decimal separator
    deg10_la = (latitude - deg_la) * 24;
    
    //Longitude
    if(o1 == 'E')
    {
        lo1 = 180 + deg_lo;
    }
    else
    {
        lo1 = 180 - deg_lo;
    }
    lo2 = lo1 / 20;
    lo3 = lo1 - lo2 * 20;

    //Lattitude
    if(o2 == 'N')
    {
        la1 = 90 + deg_la;
    }
    else
    {
        la1 = 90 - deg_la;
    }
    la2 = la1 / 10;
    la3 = la1 - la2 * 10;
    
    *(buffer + 0) = lo2 + 65;
    *(buffer + 1) = la2 + 65;
    *(buffer + 2) = lo3 / 2 + '0';
    *(buffer + 3) = la3 + '0';
    *(buffer + 4) = (int) deg10_lo + 'A';
    *(buffer + 5) = (int) deg10_la + 'A';
 }

int main()
{
	uint8_t ch;
   	int t1, t2, t3;
   	int dispdelay = 1000;
   	
   	double lon, lat;
   	char *rbuf0, *rbuf1, *rstr;
   	
   	//NMEA messages 
   	char *msg_code = "$GPRMC"; //RMC Recommended Minimum Specific GNSS Data
      	                          
   	//char *msg_code = "$GPGLL"; //GLL Geographic Position - Latitude Longitude
   	//char *msg_code = "$GPGSV"; //GSV Satellites in view
   	//char *msg_code = "$GPVTG"; //VTG Course and ground speed
   	//char *msg_code = "$GPZDA"; //ZDA Time and Date
   	
   	//Buffer for result string
  	char *buf2;
  	char *buf3;
  	
   	int valid; //Check string for correct infotype ($GPRMC in this case!)
   		
	/* Set ports */
    DDRD = 0xF0; //LCD data PD4...PD7
    DDRC = 0x0C; //LCD RS and E PC2 and PC3,

	//Display
    lcd_init();
	_delay_ms(50);

    setcustomcharacters();   //Define °-Sign as char #1
  		
   	uart_init();  	//Init UART
   	    
    lcd_putstring(0, 0, "Searching for");
    lcd_putstring(1, 0, "GPS data...");
    _delay_ms(1000);
    
    //Prepare buffers
    init_rx_buffer();                  		    	    
    buf2 = malloc(RX_BUF_SIZE);
    buf3 = malloc(RX_BUF_SIZE);
    
    for(;;) 
	{
        ch = uart_getc();
   	   	   	   	    
        if(ch == 10 || ch == 13 || rx_buf_cnt >= RX_BUF_SIZE)
        {
			/*
			for(t1 = 0; t1 < 15; t1++)
			{
			     lcd_putchar(0, t1, rx_buf[t1]);
			     lcd_putchar(1, t1, rx_buf[t1 + 15]);
			}
			*/
	    
	        //Find start of NMEA-message
	        for(t1 = rx_buf_cnt; t1 >= 0; t1--)
		    {   
			    //Scan for specific identifier of NMEA message RMC Example "$GPRMC"
		        valid = 1;
		        for(t2 = 0; t2 < 5; t2++)
		        {
				    if(rx_buf[t1 + t2] != msg_code[t2])
				    {
					    valid = 0;
				    }
			    }		
		    		    
		        if(valid) //Infotype OK, data found
		        {
					//Copy rx-buffer to buffer2
					
					//Init buffer2
					for(t2 = 0; t2 < RX_BUF_SIZE; t2++)
					{
						*(buf2 + t2) = 0;
					}	
					
					//Copy relevant part of rx-buffer to buffer2
					t3 = 0;
					for(t2 = t1; t2 < rx_buf_cnt && rx_buf[t2] != '*'; t2++)
			        {
					    *(buf2 + t3) = rx_buf[t2];
					    t3++;
					}   
					
					lcd_cls();
								    
					//Display information
					lcd_putstring(0, 0, "  GPS Receiver");
                    lcd_putstring(1, 0, "   DK7IH 2018");
                    _delay_ms(dispdelay);
					lcd_cls();

					//Message string
					lcd_putstring(0, 2, "Message type");
					lcd_putstring(1, 4, msg_code);
					_delay_ms(dispdelay);
					lcd_cls();
					
					//Receiver status
					lcd_putstring(0, 0, "Receiver status");
					if(get_receiver_status(buf2) == 0)
					{
						lcd_putstring(1, 7, "OK");
						_delay_ms(dispdelay);
					    lcd_cls();					
					    
					    //DATE
					    lcd_putstring(0, 6, "Date");
					    get_date(buf2, buf3);
					    lcd_putstring(1, 4, buf3);
					    _delay_ms(dispdelay);
					    lcd_cls();

					    //TIME
					    lcd_putstring(0, 3, "Time (UTC)");
					    get_time(buf2, buf3);
					    lcd_putstring(1, 4, buf3);
					    _delay_ms(dispdelay);
					    lcd_cls();
						
    					//Lattitude
					    lcd_putstring(0, 4, "Lattitude");
										
					    get_latitude(buf2, buf3);
					    lcd_putstring(1, 2, buf3);
					    get_latitude_ns(buf2, buf3); //N or S
					    lcd_putstring(1, 14, buf3);
					    _delay_ms(dispdelay);
					    lcd_cls();
					
					    //Longitude
					    lcd_putstring(0, 4, "Longitude");
					    get_longitude(buf2, buf3);
					    lcd_putstring(1, 2, buf3);
					    lcd_putstring(0, 4, "Longitude"); //E or W
					    get_longitude_ew(buf2, buf3);
					    lcd_putstring(1, 15, buf3);
					    _delay_ms(dispdelay);
					    lcd_cls();

                        //Ground speed
					    lcd_putstring(0, 2, "Ground speed");
					    get_ground_speed(buf2, buf3);
					    lcd_putstring(1, 5, buf3);
					    lcd_putstring(1, 12, "[kn]");
					    _delay_ms(dispdelay);
					    lcd_cls();
						
						//QTH locator (Maidenhead)
						lcd_putstring(0, 2, "QTH Locator");
						rbuf0 = malloc(4);
						rbuf1 = malloc(4);
						for(t1 = 0; t1 < 4; t1++)
						{
							*(rbuf0 + t1) = 0;
							*(rbuf1 + t1) = 0;
						}	
					
						lon = get_gps_coordinate_decimal(buf2, 0, rbuf0); //Calc longitude
						lat = get_gps_coordinate_decimal(buf2, 1, rbuf1); //Calc latitude
					
						rstr = malloc(7);
						for(t1 = 0; t1 < 7; t1++)
						{
							*(rstr + t1) = 0;
						}	
						calc_maidenhead(lon, rbuf0[0], lat, rbuf1[0], rstr);
						//calc_maidenhead(8.3362, 'E', 49.0145, 'N', rstr); //Test for "JN49EA"
						lcd_putstring(1, 4, rstr);
						_delay_ms(dispdelay);
				    	free(rbuf0);   			        
   			        	free(rbuf1);
   			        	free(rstr);
						lcd_cls(); 
					}	
					else	
					{
						lcd_putstring(1, 7, "ERR");
					}	
					_delay_ms(dispdelay);
					lcd_cls();
		        }	
		    }    
	        rx_buf_cnt = 0;    
	    }
	    else
	    {
		    rx_buf[rx_buf_cnt++] = ch;
	    }			  
    }
	return 0;
}

