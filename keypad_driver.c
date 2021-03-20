/*
* libMD407
* keypad_driver.c
* Keypad connected to PD8-PD15
* Driver for KEYPAD
*/
#include "libMD407.h"

int keypad_init( int initval );
void keypad_deinit( int deinitval);
int keypad_read(char *ptr, int len);

DEV_DRIVER_DESC KeyPad =
{
    {"Keypad"},
    keypad_init,
    keypad_deinit,
    0,
    0,
    0,
    0,
    0,
    0,
    keypad_read
};

/* KEYPAD types and constants definitions */
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

static int initiated = 0;

/* Define functions here */

// Activates row depending on row parameter
void activateRow(unsigned int row){
    unsigned int asd = GPIOD->ODR;
    asd &= 0x00FF;   // Resets odr high
    switch (row){
        case 1:
            asd |= 0x1000;    
            GPIOD->ODR = asd;
        break;
            
        case 2:
            asd |= 0x2000;
            GPIOD->ODR = asd;
        break;
        
        case 3:
            asd |= 0x4000;
            GPIOD->ODR = asd;
        break;
        
        case 4:
            asd |= 0x8000;
            GPIOD->ODR = asd;
        break;
        
        default:
            GPIOD->ODR = 0x0; // If invalid parameter, do not activate
        break;
        
    }
}

// Checks if and what column is activated.
int getCol(){
    unsigned int c;
    c = GPIOD->IDR;
    c = c>>8; // Shift to lower byte for easier management of values
    if(c & 8)
        return 4;
    if(c & 4)
        return 3;
    if(c & 2)
        return 2;
    if(c & 1)
        return 1;   
    return 0;
}

// Initiate keypad on bits 8-16 on port D
int keypad_init( int initval ) {
    if (initiated == 0) {
        GPIOD->MODER &= ~0xFFFF0000;
        GPIOD->MODER |= 0x55000000;
        GPIOD->OTYPER &= 0x00FF; // Push pull on rows
        GPIOD->PUPDR &= ~0xFFFF0000; // Pull down on columns
        GPIOD->PUPDR |= 0x00AA0000;
        
        initiated = 1;
    }
}

// Resets keypad config on port D 8-16.
void keypad_deinit( int deinitval) {
    GPIOD->MODER &= ~0xFFFF0000;
    GPIOD->PUPDR &= ~0xFFFF0000;
    
    initiated = 0;
}

// Returns the pressed key from the keypad, otherwise returns 0.
char keyb() {
    char rv;
    int activated = 0;
    int row = 0;
    for(int x = 1; x<=4; x++){ 
        activateRow(x);         // Activate row
        activated = getCol();   // Check for activated column
        row = x;
        if(activated != 0) break; // Break if key was pressed
    }
    char outputs[] = {'1','2','3','A','4','5','6','B','7','8','9','C','*','0','#','D'};
    if (activated != 0) {
        int index = ((row-1)*4)+activated-1;
        rv = outputs[index];
        return rv;
    }
    return 0;
}

// Reads the key pressed from the keypad, and stores it in the pointer
int keypad_read(char *ptr, int len) {
	unsigned char c = keyb();
	*ptr++ = c;  // Add to string
	// This section can be used if one wants to enter several keys and form a string
	/*for (int i = 0; i < len; i++) {
		char c = 0;
		while (1) { // Wait for key to be un-pressed
			c = keyb();
			if (c == 0) {
				break;
			}
		}
		while (1) { // Wait for key to be pressed
			c = keyb();
			if (c != 0) {
				break;
			}
		}
		*ptr++ = c;
		
	}*/
    return len;
}
