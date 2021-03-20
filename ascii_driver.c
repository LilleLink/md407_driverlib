/*
* libMD407
* asciidisplay_driver.c
* Display connected to PE
* Driver for ASCIIDISPLAY
*/
#include "libMD407.h"

int asciidisplay_init( int initval );
void asciidisplay_deinit( int deinitval);
int asciidisplay_write(char *ptr, int len);

DEV_DRIVER_DESC AsciiDisplay =
{
    {"AsciiDisplay"},
    asciidisplay_init,
    asciidisplay_deinit,
    0,
    0,
    0,
    0,
    0,
    asciidisplay_write,
    0
};

/* ASCIIDISPLAY types and constants definitions */
typedef struct{
    volatile unsigned int MODER;
    volatile unsigned short OTYPER;
    volatile unsigned short Unused0;
    volatile unsigned int OSPEEDR;
    volatile unsigned int PUPDR;
    union{
        volatile struct{
            volatile unsigned char IDR_LOW;
            volatile unsigned char IDR_HIGH;
        };
        volatile unsigned short IDR;
    };
    volatile unsigned short Unused1;
    union{
        volatile struct{
            volatile unsigned char ODR_LOW;
            volatile unsigned char ODR_HIGH;
        };
        volatile unsigned short ODR;
    };
    volatile unsigned short Unused2;
    volatile unsigned int BSRR;
    volatile unsigned int LCKR;
    volatile unsigned int AFRL;
    volatile unsigned int AFRH;
} GPIO;

#define GPIOA ((GPIO *) 0x40020000)
#define GPIOB ((GPIO *) 0x40020400)
#define GPIOC ((GPIO *) 0x40020800)
#define GPIOD ((GPIO *) 0x40020C00)
#define GPIOE ((GPIO *) 0x40021000)

#define B_E 0x40 // Enable workcycle
#define B_SELECT 4 // Always set to 1
#define B_RW 2 // Sets the desired operation (read/write)
#define B_RS 1 // Sets the desired register (data/control)

// Definitions used for delay
#define STK_CTRL 0xE000E010
#define STK_LOAD (volatile unsigned int*)   0xE000E014
#define STK_VAL (volatile unsigned int*)    0xE000E018
#define SIMULATOR 0x0 // Dummy definition for scaling delays

static int initiated = 0;

/* Define functions here */

// Delay functions for creating appropriate delays between operations on the asciidisplay.
void delay_250ns()
{
    // Processor is 168MHz, to delay 2.5*10^-7 sek we need to delay 42 clockpulses
    // 168*10^6*2.5*10^-7 = 42.
    unsigned int countValue = (168 / 4) - 1;
    *((unsigned int*)STK_CTRL) = 0;
    *((unsigned int*)STK_LOAD) = countValue;
    *((unsigned int*)STK_VAL) = 0;
    *((unsigned int*)STK_CTRL) = 5;

    while((*((unsigned int*)STK_CTRL) & 0x10000) == 0)
        ; // wait until its done

    *((unsigned int*)STK_CTRL) = 0;
}

void delay_mikro(unsigned int us)
{
    for(int i = 0; i < us * 4; i++) { // 1 us = 250*4ns
        delay_250ns();
    }
}

void delay_milli(unsigned int ms) {
    #ifdef SIMULATOR
        ms = ms/10000; // Because simulator is slower
        ms++;
    #endif
    delay_mikro(1000*ms);
}

// Helper functions for running operations on the asciidisp.
void ascii_ctrl_bit_set(unsigned char x)
{
    unsigned char c = GPIOE->ODR_LOW;
    c |= x;
    GPIOE->ODR_LOW = B_SELECT | c;
}

void ascii_ctrl_bit_clear(unsigned char x)
{
    unsigned char c = GPIOE->ODR_LOW;
    c &= ~x;
    GPIOE->ODR_LOW = B_SELECT | c;
}

void ascii_write_controller(unsigned char byte)
{
    delay_250ns();
    ascii_ctrl_bit_set(B_E);

    GPIOE->ODR_HIGH = byte;

    delay_250ns();

    ascii_ctrl_bit_clear(B_E);

    delay_250ns();
}

void ascii_write_cmd(unsigned char command)
{
    ascii_ctrl_bit_clear(B_RS);
    ascii_ctrl_bit_clear(B_RW);

    ascii_write_controller(command);
}

unsigned char ascii_read_controller(void)
{

    ascii_ctrl_bit_set(B_E); // Initiate work cycle
    delay_mikro(1);
    unsigned char c = GPIOE->IDR_HIGH;

    ascii_ctrl_bit_clear(B_E); // Work cycle done

    return c;
}

unsigned char ascii_read_status(void)
{

    GPIOE->MODER = 0x00005555; // Changes the port function to be able to read from IDR

    unsigned char c;
    ascii_ctrl_bit_clear(B_RS); // Select register
    ascii_ctrl_bit_set(B_RW);   // Selects read
    
    c = ascii_read_controller();

    GPIOE->MODER = 0x55555555;

    return c;
}

// Waits for the asciidisplay to be ready for a new operation.
void waitForStatus()
{
    while((ascii_read_status() & 0x80) == 0x80);
    delay_mikro(8);
}

// Initiates asciidisplay on E port, pin 1-16.
int asciidisplay_init( int initval ) {
    if (initiated == 0) {
            GPIOE->MODER = 0x55555555;
        waitForStatus();

        ascii_write_cmd(0x38); // Sets rows and char size
        delay_mikro(50);    // Delay for the display to run the command

        waitForStatus();

        ascii_write_cmd(0xE); // Lights the display
        delay_mikro(50);

        waitForStatus();
        ascii_write_cmd(0x1); // Clears the display

        delay_milli(2);

        ascii_write_cmd(0x6); // Sets marker direction to increment (right) 
        delay_mikro(50);
        
        initiated = 1;
    }
}

// Resets port E
void asciidisplay_deinit( int deinitval) {
    GPIOE->MODER = 0x00000000;
    initiated = 0;
}

// Writes parameter data to the data register on the asciidisplay.
void ascii_write_data(unsigned char data)
{

    ascii_ctrl_bit_set(B_RS);
    ascii_ctrl_bit_clear(B_RW);

    ascii_write_controller(data);
}

// Function used to change row on the asciidisplay.
void ascii_gotoxy(int x, int y)
{

    int adress = x - 1;
    if(y == 2) {
        adress += 0x40;
    }
    ascii_write_cmd(0x80 | adress);
}

// Writes a string to the asciidisplay.
int asciidisplay_write(char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        waitForStatus();    // Waits for the display to be ready
        if (i == 21) {
            ascii_gotoxy(1,2);  // Changes row after 20 characters.
        }
        ascii_write_data(*ptr++);
        delay_mikro(50); // Delay for the character to be written
    }
}