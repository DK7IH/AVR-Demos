/*****************************************************************/ 
/*               LCD12864-Demo with ATMega32                     */ 
/*  ************************************************************ */
/*  Mikrocontroller:  ATMEL AVR ATmega32, 8 MHz                  */
/*                                                               */
/*  Compiler:         GCC (GNU AVR C-Compiler)                   */
/*  Autor:            Peter Rachow (DK7IH)                       */
/*  Letzte Aenderung: 2018-12-25                                 */
/*****************************************************************/
//8 bit parallel Version
// O U T P U T for LCD 

//Connection LCD to uC: 
//LCD-Data: PD0..PD7
//RS:       PC0
//RW:       PC1
//E:        PC2
//RST       PC3

#define F_CPU 8000000
#define FONTHEIGHT 8

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <util/delay.h>

  ///////////////////
 //  LCD-Display  //
///////////////////
#define LCD_DATA_DDR DDRD        
#define LCD_DATA_PORT PORTD      
#define LCD_CTRL_DDR DDRB        
#define LCD_CTRL_PORT PORTB      

#define LCD_RS 0
#define LCD_RW 1  
#define LCD_E  2  
#define LCD_RST 6 

int main(void);

void lcd_write(char, unsigned char);
char lcd_read(char);
void set_rs(char);
void set_e(char);
void set_rw(char);
int is_lcd_busy(void);
void lcd_init(void);
void lcd_cls(void);
void lcd_putchar(int, int, unsigned char, int);
void lcd_putchar2(int, int, unsigned char, int);
void lcd_putchar3(int, int, unsigned char, int);
void lcd_putstring_a(int, int, char*, int, int);
void lcd_putstring_b(int, int, char*, int);
void lcd_putnumber(int, int, long, int, int, int);

//STRING FUNCTIONS
int int2asc(long, int, char*, int);

//Font for graphics LCD 5x8
unsigned char font[] =
{
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x00
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x01
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x02
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x03
0x00,0x04,0x0E,0x1F,0x1F,0x0E,0x04,0x00,	// 0x04
0x04,0x0E,0x0E,0x04,0x1F,0x1F,0x04,0x00,	// 0x05
0x00,0x04,0x0E,0x1F,0x1F,0x04,0x0E,0x00,	// 0x06
0x0E,0x1F,0x15,0x1F,0x11,0x1F,0x0E,0x00,	// 0x07
0x0E,0x11,0x1B,0x11,0x15,0x11,0x0E,0x00,	// 0x08
0x00,0x0A,0x1F,0x1F,0x1F,0x0E,0x04,0x00,	// 0x09
0x0E,0x11,0x1B,0x11,0x15,0x11,0x0E,0x00,	// 0x0A
0x00,0x07,0x03,0x0D,0x12,0x12,0x0C,0x00,	// 0x0B
0x0E,0x11,0x11,0x0E,0x04,0x0E,0x04,0x00,	// 0x0C
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x0D
0x03,0x0D,0x0B,0x0D,0x0B,0x1B,0x18,0x00,	// 0x0E
0x00,0x15,0x0E,0x1B,0x0E,0x15,0x00,0x00,	// 0x0F
0x08,0x0C,0x0E,0x0F,0x0E,0x0C,0x08,0x00,	// 0x10
0x02,0x06,0x0E,0x1E,0x0E,0x06,0x02,0x00,	// 0x11
0x04,0x0E,0x1F,0x04,0x1F,0x0E,0x04,0x00,	// 0x12
0x0A,0x0A,0x0A,0x0A,0x0A,0x00,0x0A,0x00,	// 0x13
0x0F,0x15,0x15,0x0D,0x05,0x05,0x05,0x00,	// 0x14
0x0E,0x11,0x0C,0x0A,0x06,0x11,0x0E,0x00,	// 0x15
0x00,0x00,0x00,0x00,0x00,0x1E,0x1E,0x00,	// 0x16
0x04,0x0E,0x1F,0x04,0x1F,0x0E,0x04,0x0E,	// 0x17
0x04,0x0E,0x1F,0x04,0x04,0x04,0x04,0x00,	// 0x18
0x04,0x04,0x04,0x04,0x1F,0x0E,0x04,0x00,	// 0x19
0x00,0x04,0x06,0x1F,0x06,0x04,0x00,0x00,	// 0x1A
0x00,0x04,0x0C,0x1F,0x0C,0x04,0x00,0x00,	// 0x1B
0x00,0x00,0x00,0x10,0x10,0x10,0x1F,0x00,	// 0x1C
0x00,0x0A,0x0A,0x1F,0x0A,0x0A,0x00,0x00,	// 0x1D
0x04,0x04,0x0E,0x0E,0x1F,0x1F,0x00,0x00,	// 0x1E
0x1F,0x1F,0x0E,0x0E,0x04,0x04,0x00,0x00,	// 0x1F
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x20
0x04,0x0E,0x0E,0x04,0x04,0x00,0x04,0x00,	// 0x21
0x1B,0x1B,0x12,0x00,0x00,0x00,0x00,0x00,	// 0x22
0x00,0x0A,0x1F,0x0A,0x0A,0x1F,0x0A,0x00,	// 0x23
0x08,0x0E,0x10,0x0C,0x02,0x1C,0x04,0x00,	// 0x24
0x19,0x19,0x02,0x04,0x08,0x13,0x13,0x00,	// 0x25
0x08,0x14,0x14,0x08,0x15,0x12,0x0D,0x00,	// 0x26
0x0C,0x0C,0x08,0x00,0x00,0x00,0x00,0x00,	// 0x27
0x04,0x08,0x08,0x08,0x08,0x08,0x04,0x00,	// 0x28
0x08,0x04,0x04,0x04,0x04,0x04,0x08,0x00,	// 0x29
0x00,0x0A,0x0E,0x1F,0x0E,0x0A,0x00,0x00,	// 0x2A
0x00,0x04,0x04,0x1F,0x04,0x04,0x00,0x00,	// 0x2B
0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x08,	// 0x2C
0x00,0x00,0x00,0x1F,0x00,0x00,0x00,0x00,	// 0x2D
0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00,	// 0x2E
0x00,0x01,0x02,0x04,0x08,0x10,0x00,0x00,	// 0x2F
0x0E,0x11,0x13,0x15,0x19,0x11,0x0E,0x00,	// 0x30
0x04,0x0C,0x04,0x04,0x04,0x04,0x0E,0x00,	// 0x31
0x0E,0x11,0x01,0x06,0x08,0x10,0x1F,0x00,	// 0x32
0x0E,0x11,0x01,0x0E,0x01,0x11,0x0E,0x00,	// 0x33
0x02,0x06,0x0A,0x12,0x1F,0x02,0x02,0x00,	// 0x34
0x1F,0x10,0x10,0x1E,0x01,0x11,0x0E,0x00,	// 0x35
0x06,0x08,0x10,0x1E,0x11,0x11,0x0E,0x00,	// 0x36
0x1F,0x01,0x02,0x04,0x08,0x08,0x08,0x00,	// 0x37
0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E,0x00,	// 0x38
0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C,0x00,	// 0x39
0x00,0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x00,	// 0x3A
0x00,0x00,0x0C,0x0C,0x00,0x0C,0x0C,0x08,	// 0x3B
0x02,0x04,0x08,0x10,0x08,0x04,0x02,0x00,	// 0x3C
0x00,0x00,0x1F,0x00,0x00,0x1F,0x00,0x00,	// 0x3D
0x08,0x04,0x02,0x01,0x02,0x04,0x08,0x00,	// 0x3E
0x0E,0x11,0x01,0x06,0x04,0x00,0x04,0x00,	// 0x3F
0x0E,0x11,0x17,0x15,0x17,0x10,0x0E,0x00,	// 0x40
0x0E,0x11,0x11,0x11,0x1F,0x11,0x11,0x00,	// 0x41
0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E,0x00,	// 0x42
0x0E,0x11,0x10,0x10,0x10,0x11,0x0E,0x00,	// 0x43
0x1E,0x11,0x11,0x11,0x11,0x11,0x1E,0x00,	// 0x44
0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F,0x00,	// 0x45
0x1F,0x10,0x10,0x1E,0x10,0x10,0x10,0x00,	// 0x46
0x0E,0x11,0x10,0x17,0x11,0x11,0x0F,0x00,	// 0x47
0x11,0x11,0x11,0x1F,0x11,0x11,0x11,0x00,	// 0x48
0x0E,0x04,0x04,0x04,0x04,0x04,0x0E,0x00,	// 0x49
0x01,0x01,0x01,0x01,0x11,0x11,0x0E,0x00,	// 0x4A
0x11,0x12,0x14,0x18,0x14,0x12,0x11,0x00,	// 0x4B
0x10,0x10,0x10,0x10,0x10,0x10,0x1F,0x00,	// 0x4C
0x11,0x1B,0x15,0x11,0x11,0x11,0x11,0x00,	// 0x4D
0x11,0x19,0x15,0x13,0x11,0x11,0x11,0x00,	// 0x4E
0x0E,0x11,0x11,0x11,0x11,0x11,0x0E,0x00,	// 0x4F
0x1E,0x11,0x11,0x1E,0x10,0x10,0x10,0x00,	// 0x50
0x0E,0x11,0x11,0x11,0x15,0x12,0x0D,0x00,	// 0x51
0x1E,0x11,0x11,0x1E,0x12,0x11,0x11,0x00,	// 0x52
0x0E,0x11,0x10,0x0E,0x01,0x11,0x0E,0x00,	// 0x53
0x1F,0x04,0x04,0x04,0x04,0x04,0x04,0x00,	// 0x54
0x11,0x11,0x11,0x11,0x11,0x11,0x0E,0x00,	// 0x55
0x11,0x11,0x11,0x11,0x11,0x0A,0x04,0x00,	// 0x56
0x11,0x11,0x15,0x15,0x15,0x15,0x0A,0x00,	// 0x57
0x11,0x11,0x0A,0x04,0x0A,0x11,0x11,0x00,	// 0x58
0x11,0x11,0x11,0x0A,0x04,0x04,0x04,0x00,	// 0x59
0x1E,0x02,0x04,0x08,0x10,0x10,0x1E,0x00,	// 0x5A
0x0E,0x08,0x08,0x08,0x08,0x08,0x0E,0x00,	// 0x5B
0x00,0x10,0x08,0x04,0x02,0x01,0x00,0x00,	// 0x5C
0x0E,0x02,0x02,0x02,0x02,0x02,0x0E,0x00,	// 0x5D
0x04,0x0A,0x11,0x00,0x00,0x00,0x00,0x00,	// 0x5E
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3F,	// 0x5F
0x0C,0x0C,0x04,0x00,0x00,0x00,0x00,0x00,	// 0x60
0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0x61
0x10,0x10,0x1E,0x11,0x11,0x11,0x1E,0x00,	// 0x62
0x00,0x00,0x0E,0x11,0x10,0x11,0x0E,0x00,	// 0x63
0x01,0x01,0x0F,0x11,0x11,0x11,0x0F,0x00,	// 0x64
0x00,0x00,0x0E,0x11,0x1E,0x10,0x0E,0x00,	// 0x65
0x06,0x08,0x08,0x1E,0x08,0x08,0x08,0x00,	// 0x66
0x00,0x00,0x0F,0x11,0x11,0x0F,0x01,0x0E,	// 0x67
0x10,0x10,0x1C,0x12,0x12,0x12,0x12,0x00,	// 0x68
0x04,0x00,0x04,0x04,0x04,0x04,0x06,0x00,	// 0x69
0x02,0x00,0x06,0x02,0x02,0x02,0x12,0x0C,	// 0x6A
0x10,0x10,0x12,0x14,0x18,0x14,0x12,0x00,	// 0x6B
0x04,0x04,0x04,0x04,0x04,0x04,0x06,0x00,	// 0x6C
0x00,0x00,0x1A,0x15,0x15,0x11,0x11,0x00,	// 0x6D
0x00,0x00,0x1C,0x12,0x12,0x12,0x12,0x00,	// 0x6E
0x00,0x00,0x0E,0x11,0x11,0x11,0x0E,0x00,	// 0x6F
0x00,0x00,0x1E,0x11,0x11,0x11,0x1E,0x10,	// 0x70
0x00,0x00,0x0F,0x11,0x11,0x11,0x0F,0x01,	// 0x71
0x00,0x00,0x16,0x09,0x08,0x08,0x1C,0x00,	// 0x72
0x00,0x00,0x0E,0x10,0x0E,0x01,0x0E,0x00,	// 0x73
0x00,0x08,0x1E,0x08,0x08,0x0A,0x04,0x00,	// 0x74
0x00,0x00,0x12,0x12,0x12,0x16,0x0A,0x00,	// 0x75
0x00,0x00,0x11,0x11,0x11,0x0A,0x04,0x00,	// 0x76
0x00,0x00,0x11,0x11,0x15,0x1F,0x0A,0x00,	// 0x77
0x00,0x00,0x12,0x12,0x0C,0x12,0x12,0x00,	// 0x78
0x00,0x00,0x12,0x12,0x12,0x0E,0x04,0x18,	// 0x79
0x00,0x00,0x1E,0x02,0x0C,0x10,0x1E,0x00,	// 0x7A
0x06,0x08,0x08,0x18,0x08,0x08,0x06,0x00,	// 0x7B
0x04,0x04,0x04,0x00,0x04,0x04,0x04,0x00,	// 0x7C
0x0C,0x02,0x02,0x03,0x02,0x02,0x0C,0x00,	// 0x7D
0x0A,0x14,0x00,0x00,0x00,0x00,0x00,0x00,	// 0x7E
0x04,0x0E,0x1B,0x11,0x11,0x1F,0x00,0x00,	// 0x7F
0x0E,0x11,0x10,0x10,0x11,0x0E,0x04,0x0C,	// 0x80
/* 
//If you opearte a microcontroller with more memory space 
//than an ATmega32 you can also use the following 127 characters!
0x12,0x00,0x12,0x12,0x12,0x16,0x0A,0x00,	// 0x81
0x03,0x00,0x0E,0x11,0x1E,0x10,0x0E,0x00,	// 0x82
0x0E,0x00,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0x83
0x0A,0x00,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0x84
0x0C,0x00,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0x85
0x0E,0x0A,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0x86
0x00,0x0E,0x11,0x10,0x11,0x0E,0x04,0x0C,	// 0x87
0x0E,0x00,0x0E,0x11,0x1E,0x10,0x0E,0x00,	// 0x88
0x0A,0x00,0x0E,0x11,0x1E,0x10,0x0E,0x00,	// 0x89
0x0C,0x00,0x0E,0x11,0x1E,0x10,0x0E,0x00,	// 0x8A
0x0A,0x00,0x04,0x04,0x04,0x04,0x06,0x00,	// 0x8B
0x0E,0x00,0x04,0x04,0x04,0x04,0x06,0x00,	// 0x8C
0x08,0x00,0x04,0x04,0x04,0x04,0x06,0x00,	// 0x8D
0x0A,0x00,0x04,0x0A,0x11,0x1F,0x11,0x00,	// 0x8E
0x0E,0x0A,0x0E,0x1B,0x11,0x1F,0x11,0x00,	// 0x8F
0x03,0x00,0x1F,0x10,0x1E,0x10,0x1F,0x00,	// 0x90
0x00,0x00,0x1E,0x05,0x1F,0x14,0x0F,0x00,	// 0x91
0x0F,0x14,0x14,0x1F,0x14,0x14,0x17,0x00,	// 0x92
0x0E,0x00,0x0C,0x12,0x12,0x12,0x0C,0x00,	// 0x93
0x0A,0x00,0x0C,0x12,0x12,0x12,0x0C,0x00,	// 0x94
0x18,0x00,0x0C,0x12,0x12,0x12,0x0C,0x00,	// 0x95
0x0E,0x00,0x12,0x12,0x12,0x16,0x0A,0x00,	// 0x96
0x18,0x00,0x12,0x12,0x12,0x16,0x0A,0x00,	// 0x97
0x0A,0x00,0x12,0x12,0x12,0x0E,0x04,0x18,	// 0x98
0x12,0x0C,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0x99
0x0A,0x00,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0x9A
0x00,0x00,0x01,0x0E,0x16,0x1A,0x1C,0x20,	// 0x9B
0x06,0x09,0x08,0x1E,0x08,0x09,0x17,0x00,	// 0x9C
0x0F,0x13,0x15,0x15,0x15,0x19,0x1E,0x00,	// 0x9D
0x00,0x11,0x0A,0x04,0x0A,0x11,0x00,0x00,	// 0x9E
0x02,0x05,0x04,0x0E,0x04,0x04,0x14,0x08,	// 0x9F
0x06,0x00,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0xA0
0x06,0x00,0x04,0x04,0x04,0x04,0x06,0x00,	// 0xA1
0x06,0x00,0x0C,0x12,0x12,0x12,0x0C,0x00,	// 0xA2
0x06,0x00,0x12,0x12,0x12,0x16,0x0A,0x00,	// 0xA3
0x0A,0x14,0x00,0x1C,0x12,0x12,0x12,0x00,	// 0xA4
0x0A,0x14,0x00,0x12,0x1A,0x16,0x12,0x00,	// 0xA5
0x0E,0x01,0x0F,0x11,0x0F,0x00,0x0F,0x00,	// 0xA6
0x0C,0x12,0x12,0x12,0x0C,0x00,0x1E,0x00,	// 0xA7
0x04,0x00,0x04,0x0C,0x10,0x11,0x0E,0x00,	// 0xA8
0x1E,0x25,0x2B,0x2D,0x2B,0x21,0x1E,0x00,	// 0xA9
0x00,0x00,0x3F,0x01,0x01,0x00,0x00,0x00,	// 0xAA
0x10,0x12,0x14,0x0E,0x11,0x02,0x07,0x00,	// 0xAB
0x10,0x12,0x14,0x0B,0x15,0x07,0x01,0x00,	// 0xAC
0x04,0x00,0x04,0x04,0x0E,0x0E,0x04,0x00,	// 0xAD
0x00,0x00,0x09,0x12,0x09,0x00,0x00,0x00,	// 0xAE
0x00,0x00,0x12,0x09,0x12,0x00,0x00,0x00,	// 0xAF
0x15,0x00,0x2A,0x00,0x15,0x00,0x2A,0x00,	// 0xB0
0x15,0x2A,0x15,0x2A,0x15,0x2A,0x15,0x2A,	// 0xB1
0x2A,0x3F,0x15,0x3F,0x2A,0x3F,0x15,0x3F,	// 0xB2
0x04,0x04,0x04,0x04,0x04,0x04,0x04,0x04,	// 0xB3
0x04,0x04,0x04,0x3C,0x04,0x04,0x04,0x04,	// 0xB4
0x06,0x00,0x04,0x0A,0x11,0x1F,0x11,0x00,	// 0xB5
0x0E,0x00,0x04,0x0A,0x11,0x1F,0x11,0x00,	// 0xB6
0x0C,0x00,0x04,0x0A,0x11,0x1F,0x11,0x00,	// 0xB7
0x1E,0x21,0x2D,0x29,0x2D,0x21,0x1E,0x00,	// 0xB8
0x14,0x34,0x04,0x34,0x14,0x14,0x14,0x14,	// 0xB9
0x14,0x14,0x14,0x14,0x14,0x14,0x14,0x14,	// 0xBA
0x00,0x3C,0x04,0x34,0x14,0x14,0x14,0x14,	// 0xBB
0x14,0x34,0x04,0x3C,0x00,0x00,0x00,0x00,	// 0xBC
0x00,0x04,0x0E,0x10,0x10,0x0E,0x04,0x00,	// 0xBD
0x11,0x0A,0x04,0x1F,0x04,0x1F,0x04,0x00,	// 0xBE
0x00,0x00,0x00,0x3C,0x04,0x04,0x04,0x04,	// 0xBF
0x04,0x04,0x04,0x07,0x00,0x00,0x00,0x00,	// 0xC0
0x04,0x04,0x04,0x3F,0x00,0x00,0x00,0x00,	// 0xC1
0x00,0x00,0x00,0x3F,0x04,0x04,0x04,0x04,	// 0xC2
0x04,0x04,0x04,0x07,0x04,0x04,0x04,0x04,	// 0xC3
0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00,	// 0xC4
0x04,0x04,0x04,0x3F,0x04,0x04,0x04,0x04,	// 0xC5
0x05,0x0A,0x0E,0x01,0x0F,0x11,0x0F,0x00,	// 0xC6
0x05,0x0A,0x04,0x0A,0x11,0x1F,0x11,0x00,	// 0xC7
0x14,0x17,0x10,0x1F,0x00,0x00,0x00,0x00,	// 0xC8
0x00,0x1F,0x10,0x17,0x14,0x14,0x14,0x14,	// 0xC9
0x14,0x37,0x00,0x3F,0x00,0x00,0x00,0x00,	// 0xCA
0x00,0x3F,0x00,0x37,0x14,0x14,0x14,0x14,	// 0xCB
0x14,0x17,0x10,0x17,0x14,0x14,0x14,0x14,	// 0xCC
0x00,0x3F,0x00,0x3F,0x00,0x00,0x00,0x00,	// 0xCD
0x14,0x37,0x00,0x37,0x14,0x14,0x14,0x14,	// 0xCE
0x11,0x0E,0x11,0x11,0x11,0x0E,0x11,0x00,	// 0xCF
0x0C,0x10,0x08,0x04,0x0E,0x12,0x0C,0x00,	// 0xD0
0x0E,0x09,0x09,0x1D,0x09,0x09,0x0E,0x00,	// 0xD1
0x0E,0x00,0x1F,0x10,0x1E,0x10,0x1F,0x00,	// 0xD2
0x0A,0x00,0x1F,0x10,0x1E,0x10,0x1F,0x00,	// 0xD3
0x0C,0x00,0x1F,0x10,0x1E,0x10,0x1F,0x00,	// 0xD4
0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00,	// 0xD5
0x06,0x00,0x0E,0x04,0x04,0x04,0x0E,0x00,	// 0xD6
0x0E,0x00,0x0E,0x04,0x04,0x04,0x0E,0x00,	// 0xD7
0x0A,0x00,0x0E,0x04,0x04,0x04,0x0E,0x00,	// 0xD8
0x04,0x04,0x04,0x3C,0x00,0x00,0x00,0x00,	// 0xD9
0x00,0x00,0x00,0x07,0x04,0x04,0x04,0x04,	// 0xDA
0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,	// 0xDB
0x00,0x00,0x00,0x00,0x3F,0x3F,0x3F,0x3F,	// 0xDC
0x04,0x04,0x04,0x00,0x04,0x04,0x04,0x00,	// 0xDD
0x0C,0x00,0x0E,0x04,0x04,0x04,0x0E,0x00,	// 0xDE
0x3F,0x3F,0x3F,0x3F,0x00,0x00,0x00,0x00,	// 0xDF
0x06,0x0C,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xE0
0x00,0x1C,0x12,0x1C,0x12,0x12,0x1C,0x10,	// 0xE1
0x0E,0x0C,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xE2
0x18,0x0C,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xE3
0x0A,0x14,0x00,0x0C,0x12,0x12,0x0C,0x00,	// 0xE4
0x0A,0x14,0x0C,0x12,0x12,0x12,0x0C,0x00,	// 0xE5
0x00,0x00,0x12,0x12,0x12,0x1C,0x10,0x10,	// 0xE6
0x00,0x18,0x10,0x1C,0x12,0x1C,0x10,0x18,	// 0xE7
0x18,0x10,0x1C,0x12,0x12,0x1C,0x10,0x18,	// 0xE8
0x06,0x00,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xE9
0x0E,0x00,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xEA
0x18,0x00,0x12,0x12,0x12,0x12,0x0C,0x00,	// 0xEB
0x06,0x00,0x12,0x12,0x12,0x0E,0x04,0x18,	// 0xEC
0x06,0x00,0x11,0x0A,0x04,0x04,0x04,0x00,	// 0xED
0x00,0x0E,0x00,0x00,0x00,0x00,0x00,0x00,	// 0xEE
0x0C,0x0C,0x08,0x00,0x00,0x00,0x00,0x00,	// 0xEF
0x00,0x00,0x00,0x0E,0x00,0x00,0x00,0x00,	// 0xF0
0x00,0x04,0x0E,0x04,0x00,0x0E,0x00,0x00,	// 0xF1
0x00,0x00,0x1F,0x00,0x00,0x1F,0x00,0x00,	// 0xF2
0x30,0x1A,0x34,0x0B,0x15,0x07,0x01,0x00,	// 0xF3
0x0F,0x15,0x15,0x0D,0x05,0x05,0x05,0x00,	// 0xF4
0x0E,0x11,0x0C,0x0A,0x06,0x11,0x0E,0x00,	// 0xF5
0x00,0x04,0x00,0x1F,0x00,0x04,0x00,0x00,	// 0xF6
0x00,0x00,0x00,0x0E,0x06,0x00,0x00,0x00,	// 0xF7
0x0C,0x12,0x12,0x0C,0x00,0x00,0x00,0x00,	// 0xF8
0x00,0x00,0x00,0x0A,0x00,0x00,0x00,0x00,	// 0xF9
0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,	// 0xFA
0x08,0x18,0x08,0x08,0x00,0x00,0x00,0x00,	// 0xFB
0x1C,0x08,0x0C,0x18,0x00,0x00,0x00,0x00,	// 0xFC
0x18,0x04,0x08,0x1C,0x00,0x00,0x00,0x00,	// 0xFD
0x00,0x00,0x1E,0x1E,0x1E,0x1E,0x00,0x00,	// 0xFE
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 	// 0xFF
*/
};
	
  /////////////////////////////////////
 // Functions for LCD12864 control  //
/////////////////////////////////////
//Write instruction (code==0) or data (code==1) to LCD
void lcd_write(char lcdmode, unsigned char value)
{
	LCD_DATA_DDR = 0xFF;     //Set port for write operation
	
	set_rw(0);	     //Write operation
	set_rs(lcdmode); //0 for instruction, 1 for data
       
	LCD_DATA_PORT = value;
    set_e(1);
    _delay_us(10);
	set_e(0);	
		
	set_rs(0);
}    

//Read data from LCD
char lcd_read(char lcdmode)
{
	unsigned char value;
	
	LCD_DATA_DDR = 0x00;  //Set port for read operation
	    
    set_rw(1);	          //Read operation
	set_rs(lcdmode);      //Get value 0: for busy flag, 1 for other data
	
	set_e(1);             //Read data
    _delay_us(10);       
	value = PIND;
    set_e(0);	
	
	//set_rs(0);
	
	return value;
}    

//Set RW line
void set_rw(char status)  
{
    if(status)
	{
       LCD_CTRL_PORT |= (1 << LCD_RW);
	}	
    else
	{
	    LCD_CTRL_PORT &= ~(1 << LCD_RW);
	}	
}

//Set RS line
void set_rs(char status) 
{
    if(status)
	{
        LCD_CTRL_PORT |= (1 << LCD_RS);
	}	
    else
	{
	    LCD_CTRL_PORT &= ~(1 << LCD_RS);
	}	
}

//Set E line
void set_e(char status)  
{
    if(status)
	{
        LCD_CTRL_PORT |= (1 << LCD_E);
	}	
    else
	{
	    LCD_CTRL_PORT &= ~(1 << LCD_E);
	}	
}

//Check for busy flag (BF)
int is_lcd_busy(void)
{
	int v = lcd_read(0);
	_delay_us(10);
	v = lcd_read(0);
			
	if(v & 0x80)
	{
		return -1;
	}
	else
	{
		return 0;
	}	
		
}

//Send one character to LCD (Normal size)
//
void lcd_putchar(int row0, int col0, unsigned char ch0, int inv)
{
	int t1;
	int odd = 0;
	unsigned char v1, v2;
	int col = col0 / 2;
	int row = row0 * FONTHEIGHT;
	unsigned char ch;
		
	if(row & 0x20)  //Enter lower part of screen => go to next page
	{
        row &= ~0x20;
        col |= 8;
    }
        
	if(col0 & 1) //Detect odd coloumn
	{
		odd = 1;
	}
		
	for(t1 = 0; t1 < FONTHEIGHT; t1++)
    {
	    //Set address
        lcd_write(0, 0x80 + row + t1);
        lcd_write(0, 0x80 + col);
             
        //Get old values of 2 GDRAM bytes	
	    v1 = lcd_read(1);                
        v1 = lcd_read(1);
        v2 = lcd_read(1);

        //Set address
        lcd_write(0, 0x80 + row + t1);
        lcd_write(0, 0x80 + col);
        
        if(!inv)
        {
			ch = font[ch0 * FONTHEIGHT + t1];
		}
		else	
        {
			ch = ~font[ch0 * FONTHEIGHT + t1];
		}
		     
        if(odd)
        {     
            //Write data on RIGHT side of existing character
            lcd_write(1, v1);
            lcd_write(1, ch);
        }
        else    
        {   
			//Write data on LEFT side of existing character
            lcd_write(1, ch);
            lcd_write(1, v2);
        }
    }    
}   

//Send one character to LCD (DOUBLE size and normal width)
//
void lcd_putchar2(int row0, int col0, unsigned char ch0, int inv)
{
	int t1, t2;
	int odd = 0;
	unsigned char v1, v2;
	int col = col0 >> 1;
	int row = row0 * FONTHEIGHT;
	unsigned char ch;
		
	if(row & 0x20)  //Enter lower part of screen => go to next page
	{
        row &= ~0x20;
        col |= 8;
    }
        
	if(col0 & 1) //Detect odd coloumn
	{
		odd = 1;
	}
		
	for(t1 = 0; t1 < FONTHEIGHT; t1++)
    {
		if(!inv) //Calculate character position in array and xor invert number if needed
        {
			ch = (font[ch0 * FONTHEIGHT + t1]);
		}
		else	
        {
			ch = (~font[ch0 * FONTHEIGHT + t1]);
		}
		
		for(t2 = 0; t2 < 2; t2++)
		{
	        //Set address
            lcd_write(0, 0x80 + row + t1 * 2 + t2);
            lcd_write(0, 0x80 + col);
             
            //Get old values of 2 GDRAM bytes	
	        v1 = lcd_read(1);                
            v1 = lcd_read(1);
            v2 = lcd_read(1);

            //Set address
            lcd_write(0, 0x80 + row + t1 * 2 + t2);
            lcd_write(0, 0x80 + col);
        		     
            if(odd)
            {     
                //Write data on RIGHT side of existing character
                lcd_write(1, v1);
                lcd_write(1, ch);
            }
            else    
            {   
			    //Write data on LEFT side of existing character
                lcd_write(1, ch);
                lcd_write(1, v2);
            }    
        }
    }    
}   

//Send one character to LCD (DOUBLE size and DOUBLE width)
//
void lcd_putchar3(int row0, int col0, unsigned char ch0, int inv)
{
	int t1, t2;
	unsigned char ch;
	//unsigned int i[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int i[FONTHEIGHT] = {0, 0, 0, 0, 0, 0, 0, 0};
	int col = col0 >> 1;
	int row = row0 * FONTHEIGHT;
	
	if(row & 0x20)  //Enter lower part of screen => go to next page
	{
        row &= ~0x20;
        col |= 8;
    }
        
		
	for(t1 = 0; t1 < FONTHEIGHT; t1++)
    {
		if(!inv) //Calculate character position in array and xor invert number if needed
        {
			ch = (font[ch0 * FONTHEIGHT + t1]);
		}
		else	
        {
			ch = (~font[ch0 * FONTHEIGHT + t1]);
		}
		
		//Double 8 to 16 bits
	    i[t1] = 0;
		for(t2 = 7; t2 > -1; t2--)
		{
			if(ch & (1 << t2))
			{
				i[t1] += (1 << ((t2 << 1) + 1)) + (1 << (t2 << 1)); //Double bit pattern 2 by 1
			}
		}
	}
	
	t2 = 0;
	for(t1 = 0; t1 < FONTHEIGHT; t1++)
	{
		for(t2 = 0; t2 < 2; t2++)
		{
	        //Set address
            lcd_write(0, 0x80 + row + t1 * 2 + t2);
            lcd_write(0, 0x80 + col);
                       
            lcd_write(1, ((i[t1] >> 8) & 0xFF));
            lcd_write(1, i[t1] & 0xFF); 
            //lcd_putnumber(t1, 0, i[t1] , -1, 0, 0);
        }    
    }    
		
}	

//Send string (\0 terminated) to LCD normal or double height
void lcd_putstring_a(int row, int col, char *s, int size, int inv)
{
    unsigned char t1;

    for(t1 = col; *(s); t1++)
	{
		if(!size)
		{
            lcd_putchar(row, t1, *(s++), inv);
        }
        else    
        {
            lcd_putchar2(row, t1, *(s++), inv);
        }
	}	
}

//String in DOUBLE height and DOUBLE width
void lcd_putstring_b(int row, int col, char *s, int inv)
{
    unsigned char t1;

    for(t1 = col; *(s); t1++)
	{
	    lcd_putchar3(row, t1 * 2, *(s++), inv);
	}	
}

//Clear LCD
void lcd_cls(void)
{
	int x, y;
    for(x = 0; x < 16; x++)
    {
		for(y = 0; y < 64; y++)
		{
			//Set address
            lcd_write(0, 0x80 + y);
            lcd_write(0, 0x80 + x);
             
            //Write data
            lcd_write(1, 0);
            lcd_write(1, 0);
        }
    }    
}

//Convert a number to a string and print it
//col, row: Coordinates, Num: int or long to be displayed
//dec: Set position of decimal separator
//
//inv: Set to 1 if inverted charactor is required
void lcd_putnumber(int col, int row, long num, int dec, int lsize, int inv)
{
    char *s = malloc(16);
	if(s != NULL)
	{
	    int2asc(num, dec, s, 16);
	    lcd_putstring_a(col, row, s, lsize, inv);
	    free(s);
	}	
	else
	{
		lcd_putstring_a(col, row, "Error", 0, 0);
	}	
	free(s);
}
  

//Init LCD
void lcd_init(void)
{            
    //Reset
    LCD_CTRL_PORT &= ~(1 << LCD_RST);
    _delay_ms(5);
    LCD_CTRL_PORT |= (1 << LCD_RST);
    _delay_ms(40);
    
    lcd_write(0, 0x30);	//Use 8-bit mode parallel
    _delay_ms(1);
         
    lcd_write(0, 0x0C); //All on Cursor on, Blink on , Display on
    _delay_ms(1);
        
    lcd_write(0, 0x01); //Perform CLS in text mode to eliminate random chars from screen
    _delay_ms(20);
    
    lcd_write(0, 0x34); //Switch to extended mode, redefine function set
    _delay_us(100);
    
    lcd_write(0, 0x36); //Add graphic mode
    _delay_us(100);
                   
    lcd_write(0, 0x12); //Display control and display ON
    _delay_us(100);
}

//////////////////////
// STRING FUNCTIONS //
//////////////////////
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


int main(void)
{
    // Set ports for LCD output and input data
    LCD_CTRL_DDR = 0xFF; //LCD RS, RW, E and RST
	LCD_DATA_DDR = 0xFF; //LCD data
   	                 		
	//Display init
	_delay_ms(100);
    lcd_init();
	_delay_ms(100);
	
    lcd_cls();
        
    lcd_putstring_a(0, 0, "LCD 12864 ST7920", 0, 0);
    lcd_putstring_a(1, 0, "   DK7IH 2018   ", 0, 1);
    lcd_putstring_a(2, 0, "Graphical Fonts:", 0, 0);
    lcd_putstring_a(3, 0, "8x8px.", 0, 0);
    lcd_putnumber(4, 0, 1234, 1, 0, 0);    
       
	lcd_putstring_a(4, 0, "16x8px.", 1, 0);
	
	lcd_putstring_b(6, 0, "16x16px.", 0);
			    
    for(;;) 
	{
    
    
	}
	return 0;
}
