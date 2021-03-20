/*
 * libmd407
 * usart_driver.c
 * USART driver
 * DRIVERS for STDIN, STDOUT, and STDERR
 */
 
#include "libmd407.h"
 
/* Function definitions */
int usart_init(int initval);
void usart_deinit( int deinitval);
int usart_write(char *ptr, int len);
int usart_read(char *ptr, int len);  

DEV_DRIVER_DESC StdIn = {
	{"stdin"},
	usart_init,
	usart_deinit,
	0,
	_isatty,
	0,
	0,
	0,
	0,
	usart_read
};

DEV_DRIVER_DESC StdOut = {
	{"stdout"},
	usart_init,
	usart_deinit,
	0,
	_isatty,
	0,
	0,
	0,
	usart_write
};

DEV_DRIVER_DESC StdErr = {
	{"stderr"},
	usart_init,
	usart_deinit,
	0,
	_isatty,
	0,
	0,
	0,
	usart_write,
	0
};

/* Constants etc */
typedef struct tag_usart {
    volatile unsigned short sr;
    volatile unsigned short unused0;
    volatile unsigned short dr;
    volatile unsigned short unused1;
    volatile unsigned short brr;
    volatile unsigned short unused2;
    volatile unsigned short cr1;
    volatile unsigned short unused3;
    volatile unsigned short cr2;
    volatile unsigned short unused4;
    volatile unsigned short cr3;
    volatile unsigned short unused5;
    volatile unsigned short gtpr;
} USART;

#define USART1 ((USART * ) 0x40011000)
#define USART2 ((USART * ) 0x40004400)
#define BIT_UE  (1<<13) // USART ENABLE
#define BIT_TE  (1<<3)  // TRANSMITTER ENABLE
#define BIT_RE  (1<<2)  // RECIEVER ENABLE
#define BIT_RXNE (1<<5) // RECIEVE DATA REGISTER NOT EMPTY
#define BIT_TXE (1<<7)  // TRANSMIT DATA REGISTER EMPTY

static int initiated = 0;

/* Usart method implementations*/

// Enables USART/transmitter/reciever and sets baud-rate
int usart_init(int initval) {
    if (initiated == 0) {
        USART1->brr = 0x2D9; // Set division in baudrate register
        USART1->cr2 = 0;
        USART1->cr1 = BIT_UE | BIT_TE | BIT_RE; // Start usart/transmitter/reciever
        USART2->brr = 0x2D9;
        USART2->cr2 = 0;
        USART2->cr1 = BIT_UE | BIT_TE | BIT_RE;
        
        initiated = 1;
    }

}

void usart_deinit( int deinitval) {
    USART1->brr = 0;
    USART1->cr2 = 0;
    USART1->cr1 = 0;
	USART2->brr = 0;
    USART2->cr2 = 0;
    USART2->cr1 = 0;
    
    initiated = 0;
}

// Writes data to the USART data register when the TX register is empty
int _outchar(char c) {
    while ((USART1->sr & BIT_TXE) == 0); // Waits for TX register to be empty 
    USART1->dr = (unsigned char) c; // Writes c to data register
}

// Checks if the RX register contains data and returns it
// Returns 0 if empty
char _tstchar() {
    if ((USART1->sr & BIT_RXNE) == BIT_RXNE) 
        return (char) USART1->dr; 
    return 0;
}

// Writes a string of length len to the terminal
int usart_write(char *ptr, int len) {
    for (int i = 0; i < len; i++) {
        _outchar(*ptr++);
    }
	return len;
} 

// Gets a string of length len from the terminal
int usart_read(char *ptr, int len) {
    char c = 0;
    for (int i = 0; i < len; i++) {
        while ((c =_tstchar()) == 0); // Wait for next character
        _outchar(c);
        if (c == 0xA) { // Press enter to cut off early
            return len;
        }
        *ptr++ = c;  // Add new character to string
    }
}
