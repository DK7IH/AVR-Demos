/*****************************************************************/
/*              MCP4725 DAC sensor with ATMega32                 */
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega32, 8 MHz                  */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow                               */
/*                    NOV-2018                                   */ 
/*****************************************************************/

/*    PORTS */
// O U T P U T 
// LCD 
// RS      = PC2
// E       = PC3
// D4...D7 = PD4..PD7

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
void lcd_display_test(void);
void setcustomcharacters(void);

/*******************/
/* Stringhandling */
/*******************/
int strlen(char *s);
int instring(char*, char*);
int strcomp(char*, char*);
void int2asc(long, int, char*, int);

/***************/
/*    T W I    */
/***************/
//I²C
void TWIInit(void);
void TWIStart(void);
void TWIStop(void);
uint8_t TWIReadACK(void);
uint8_t TWIReadNACK(void);
uint8_t TWIGetStatus(void);

/***************/
/*   MCP4725   */
/***************/
void mcp4725_set_value(int);

int sinewave[] =
{ 
2048, 2073, 2098, 2123, 2148, 2174, 2199, 2224,
2249, 2274, 2299, 2324, 2349, 2373, 2398, 2423,
2448, 2472, 2497, 2521, 2546, 2570, 2594, 2618,
2643, 2667, 2690, 2714, 2738, 2762, 2785, 2808,
2832, 2855, 2878, 2901, 2924, 2946, 2969, 2991,
3013, 3036, 3057, 3079, 3101, 3122, 3144, 3165,
3186, 3207, 3227, 3248, 3268, 3288, 3308, 3328,
3347, 3367, 3386, 3405, 3423, 3442, 3460, 3478,
3496, 3514, 3531, 3548, 3565, 3582, 3599, 3615,
3631, 3647, 3663, 3678, 3693, 3708, 3722, 3737,
3751, 3765, 3778, 3792, 3805, 3817, 3830, 3842,
3854, 3866, 3877, 3888, 3899, 3910, 3920, 3930,
3940, 3950, 3959, 3968, 3976, 3985, 3993, 4000,
4008, 4015, 4022, 4028, 4035, 4041, 4046, 4052,
4057, 4061, 4066, 4070, 4074, 4077, 4081, 4084,
4086, 4088, 4090, 4092, 4094, 4095, 4095, 4095,
4095, 4095, 4095, 4095, 4094, 4092, 4090, 4088,
4086, 4084, 4081, 4077, 4074, 4070, 4066, 4061,
4057, 4052, 4046, 4041, 4035, 4028, 4022, 4015,
4008, 4000, 3993, 3985, 3976, 3968, 3959, 3950,
3940, 3930, 3920, 3910, 3899, 3888, 3877, 3866,
3854, 3842, 3830, 3817, 3805, 3792, 3778, 3765,
3751, 3737, 3722, 3708, 3693, 3678, 3663, 3647,
3631, 3615, 3599, 3582, 3565, 3548, 3531, 3514,
3496, 3478, 3460, 3442, 3423, 3405, 3386, 3367,
3347, 3328, 3308, 3288, 3268, 3248, 3227, 3207,
3186, 3165, 3144, 3122, 3101, 3079, 3057, 3036,
3013, 2991, 2969, 2946, 2924, 2901, 2878, 2855,
2832, 2808, 2785, 2762, 2738, 2714, 2690, 2667,
2643, 2618, 2594, 2570, 2546, 2521, 2497, 2472,
2448, 2423, 2398, 2373, 2349, 2324, 2299, 2274,
2249, 2224, 2199, 2174, 2148, 2123, 2098, 2073,
2048, 2023, 1998, 1973, 1948, 1922, 1897, 1872,
1847, 1822, 1797, 1772, 1747, 1723, 1698, 1673,
1648, 1624, 1599, 1575, 1550, 1526, 1502, 1478,
1453, 1429, 1406, 1382, 1358, 1334, 1311, 1288,
1264, 1241, 1218, 1195, 1172, 1150, 1127, 1105,
1083, 1060, 1039, 1017,  995,  974,  952,  931,
910,  889,  869,  848,  828,  808,  788,  768,
749,  729,  710,  691,  673,  654,  636,  618,
600,  582,  565,  548,  531,  514,  497,  481,
465,  449,  433,  418,  403,  388,  374,  359,
345,  331,  318,  304,  291,  279,  266,  254,
242,  230,  219,  208,  197,  186,  176,  166,
156,  146,  137,  128,  120,  111,  103,   96,
88,   81,   74,   68,   61,   55,   50,   44,
39,   35,   30,   26,   22,   19,   15,   12,
10,    8,    6,    4,    2,    1,    1,    0,
0,    0,    1,    1,    2,    4,    6,    8,
10,   12,   15,   19,   22,   26,   30,   35,
39,   44,   50,   55,   61,   68,   74,   81,
88,   96,  103,  111,  120,  128,  137,  146,
156,  166,  176,  186,  197,  208,  219,  230,
242,  254,  266,  279,  291,  304,  318,  331,
345,  359,  374,  388,  403,  418,  433,  449,
465,  481,  497,  514,  531,  548,  565,  582,
600,  618,  636,  654,  673,  691,  710,  729,
749,  768,  788,  808,  828,  848,  869,  889,
910,  931,  952,  974,  995, 1017, 1039, 1060,
1083, 1105, 1127, 1150, 1172, 1195, 1218, 1241,
1264, 1288, 1311, 1334, 1358, 1382, 1406, 1429,
1453, 1478, 1502, 1526, 1550, 1575, 1599, 1624,
1648, 1673, 1698, 1723, 1747, 1772, 1797, 1822,
1847, 1872, 1897, 1922, 1948, 1973, 1998, 2023
}; 

/**********************/
/* V A R I A B L E S  */
/**********************/
//Timer
unsigned long runseconds = 0;

/**************************************/
/* Funktionen und Prozeduren fuer LCD */
/**************************************/
//Anschlussbelegeung am uC:
//LCD-Data: PD0..PD4
//RS: PC0
//E: PC1

/* Ein Byte (Befehl bzw. Zeichen) zum Display senden */
void lcd_write(char lcdmode, unsigned char value)
{
    int x = 16, t1;
	
    set_e(0); 

    if(!lcdmode)
	{
        set_rs(0);    /* RS=0 => Befehl */
	}	
    else
	{
        set_rs(1);    /* RS=1 => Zeichen */
	}	

    _delay_ms(1);
	
    set_e(1);
    //PORTD = (value & 0xF0);

    /* Hi nibble */
	for(t1 = 0; t1 < 4; t1++)
	{
	    if(value & x)
	    {
	       PORTD |= x;              // Bit setzen
	    }
        else	
	    {
           PORTD &= ~(x);          // Bit löschen
	    }  
		
		x *= 2;
	}	
	set_e(0);
	
	x = 16;

	set_e(1);
	
	/* Lo nibble */
	for(t1 = 0; t1 < 4; t1++)
	{
	    if((value & 0x0F) * 16 & x)
	    {
	       PORTD |= x;              // Bit setzen
	    }
        else	
	    {
           PORTD &= ~(x);          // Bit löschen
	    }  
		
		x *= 2;
	}

    set_e(0);

}

/* RS setzen */
void set_rs(char status) /* PORT PB6  */
{
    if(status)
	{
        PORTC |= (1 << 2);
	}	
    else
	{
	    PORTC &= ~(1 << 2);
	}	
}

/* E setzen */
void set_e(char status)  /* PORT PB7*/
{
    if(status)
	{
        PORTC |= (1 << 3);
	}	
    else
	{
	    PORTC &= ~(1 << 3);
	}	
}

/* Ein Zeichen (Char) zum Display senden, dieses in */
/* Zeile row und Spalte col positionieren           */
void lcd_putchar(int row, int col, unsigned char ch)
{
    lcd_write(LCD_INST, col + 128 + row * 0x40);
    lcd_write(LCD_DATA, ch);
}


/* Eine Zeichenkette direkt in das LCD schreiben */
/* Parameter: Startposition, Zeile und Pointer   */
void lcd_putstring(int row, int col, char *s)
{
    unsigned char t1;

    for(t1 = col; *(s); t1++)
	{
        lcd_putchar(row, t1, *(s++));
	}	
}


/* Display loeschen */
void lcd_cls(void)
{
    lcd_write(LCD_INST, 1);
}


/* LCD-Display initialisieren */
void lcd_init(void)
{
    /* Grundeinstellungen: 2 Zeilen, 5x7 Matrix, 4 Bit */
    lcd_write(LCD_INST, 40);
    lcd_write(LCD_INST, 40);
    lcd_write(LCD_INST, 40);

    //MAtrix 5*7
    lcd_write(LCD_INST, 8);

    /* Display on, Cursor off, Blink off */
    lcd_write(LCD_INST, 12);

    /* Entrymode !cursoincrease + !displayshifted */
    lcd_write(LCD_INST, 4);
	
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

    unsigned char customchar[]={0x04, 0x0A, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    lcd_write(LCD_INST, 0);
    lcd_write(LCD_DATA, 0);

    //Send data to CGRAM in lcd
    for (i1=0; i1<8; i1++) {
        lcd_write(LCD_INST, adr++);
        lcd_write(LCD_DATA, customchar[i1]);
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

/////////////////////////
// INTERRUPT HANDLERS  //
/////////////////////////
//Timer1
ISR(TIMER1_OVF_vect)
{
    runseconds++;

    TCNT1 = 0;       // Timerregister auf 0 
}

/***************/
/*   MCP4725   */
/***************/
//Send comand to MCP4725
void mcp4725_set_value(int value)
{
   TWIStart();
   TWIWrite(0xC0); //Device address
   TWIWrite(64);       		    	
   TWIWrite((value >> 4) & 0xFF); //8 MSBs
   TWIWrite(value & 0xF0);        //4LSBs
   TWIStop();			
		
} 

int main()
{

    int t1;	
    
	/* Set ports */
    /* OUTPUT */
    DDRC = 0x0C; //LCD RS and E at PC2 and PC3,
	             	 
    DDRD = 0xF0; //LCD data on PD4...PD7, LED at PD2
	             
    PORTC = 0x03; //Pull ups for SDL,SCA
    
    //Init TWI
    TWIInit();
	
	//Display
    lcd_init();
	_delay_ms(50);
	
	setcustomcharacters();	
	//Watchdog off
	WDTCR = 0;
	WDTCR = 0;
   	    
    lcd_putstring(0, 0, "MCP4725 DAC");
    lcd_putstring(1, 0, "DK7IH 2018");
    
    for(;;) 
	{
			for(t1 = 0; t1 < 512; t1 += 8)
		{
            mcp4725_set_value(sinewave[t1]);
        }    
	}
	return 0;
}

