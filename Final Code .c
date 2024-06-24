#include <xc.h>                // Include device-specific header
#include <pic16f877a.h>        // PIC16F877A specific definitions
#include <string.h>            // String library
#include <stdio.h>
#include <math.h>

#define _XTAL_FREQ 20000000    // Define the oscillator frequency (20 MHz)

// CONFIG
#pragma config FOSC = HS       // Oscillator Selection bits (HS oscillator)
#pragma config WDTE = OFF      // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF     // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF     // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = ON        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit
#pragma config CPD = OFF       // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF       // Flash Program Memory Write Enable bits (Write protection off)
#pragma config CP = OFF        // Flash Program Memory Code Protection bit (Code protection off)

#define RS PORTDbits.RD2       // Define RS pin for LCD
#define EN PORTDbits.RD3       // Define EN pin for LCD

// Function to send commands to the LCD
void lcd_cmd(unsigned char cmd) {
    PORTD = (cmd & 0xF0);       // Send higher nibble
    EN = 1;                     // Enable high
    RS = 0;                     // Command mode
    __delay_ms(10);             // Delay
    EN = 0;                     // Enable low
    PORTD = ((cmd << 4) & 0xF0);// Send lower nibble
    EN = 1;                     // Enable high
    RS = 0;                     // Command mode
    __delay_ms(10);             // Delay
    EN = 0;                     // Enable low
}

// Function to send data to the LCD
void lcd_data(unsigned char data) {
    PORTD = (data & 0xF0);      // Send higher nibble
    EN = 1;                     // Enable high
    RS = 1;                     // Data mode
    __delay_ms(10);             // Delay
    EN = 0;                     // Enable low
    PORTD = ((data << 4) & 0xF0);// Send lower nibble
    EN = 1;                     // Enable high
    RS = 1;                     // Data mode
    __delay_ms(10);             // Delay
    EN = 0;                     // Enable low
}

// Function to initialize the LCD
void lcd_init() {
    lcd_cmd(0x02);              // Initialize LCD in 4-bit mode
    lcd_cmd(0x28);              // 4-bit mode, 2 lines, 5x7 matrix
    lcd_cmd(0x0C);              // Display on, cursor off
    lcd_cmd(0x06);              // Increment cursor
    lcd_cmd(0x01);              // Clear display
    __delay_ms(20);             // Delay
}

// Function to send a string to the LCD
void lcd_string(const unsigned char *str, unsigned char num) {
    unsigned char i;
    for (i = 0; i < num; i++) {
        lcd_data(str[i]);       // Send each character to the LCD
    }
}

void display(int number);

// Main function
void main() {
    TRISD = 0x00;               // Set PORTD as output
    __delay_ms(50);             // Delay

    TRISC = 0b11110000;         // Set RC4 - RC7 as input (Echo pins)
    __delay_ms(50);             // Delay

    lcd_init();                 // Initialize the LCD
    __delay_ms(50);             // Delay

    //int a[4] = {0, 0, 0, 0};    // Array to store distance values
    T1CON = 0x10;               // Configure Timer1
    __delay_ms(20);

    lcd_cmd(0x80);  // Set cursor position
    lcd_string("A  cm  B  cm  ", 14); // Display "A" for 1st sensor
    __delay_ms(40);  // Delay
    lcd_cmd(0xC0);  // Set cursor position
    lcd_string("C  cm  D  cm  ", 14); // Display "C" for 3rd sensor
    __delay_ms(40);  // Delay
    while (1) {                 // Infinite loop
        
        int a[4] = {0, 0, 0, 0};    // Array to store distance values
        __delay_ms(20);  // Delay
        
        
        
        for (int i = 0; i < 4; i++) {
            __delay_ms(5);

            TMR1H = 0;          // Reset Timer1 high byte
            TMR1L = 0;          // Reset Timer1 low byte

            PORTC = (1 << i);   // Trigger the i-th sensor
            __delay_us(10);     // Short delay
            PORTC = 0;          // Clear trigger pin

            while (!(PORTC & (0x10 << i))); // Wait for echo to go high of i + 4
            TMR1ON = 1;         // Start Timer1
            while (PORTC & (0x10 << i)); // Wait for echo to go low of i + 4
            TMR1ON = 0;         // Stop Timer1
            a[i] = (TMR1L | (TMR1H << 8)) / 50; // Read Timer1 value
            a[i] = (a[i] + 1) / 3;    // Calibration adjustment to cm

            if (i == 0) {        // If first sensor
                lcd_cmd(0x81);  // Set cursor position
                __delay_ms(40);
                
            } else if (i == 1) { // If second sensor
                int result = (a[0] > a[1]) ? (a[0] - a[1]) : (a[1] - a[0]);
                double res1 = atan(result / 8.0) * (180.0 / 3.14159265);
                int angle = (int)res1;
                lcd_cmd(0x8E);
                display(angle);
                __delay_ms(40);  // Delay
                lcd_cmd(0x88);  // Set cursor position
                __delay_ms(40);
            } else if (i == 2) { // If third sensor
                lcd_cmd(0xC1);  // Set cursor position
                __delay_ms(30);
            } else if (i == 3) { // If fourth sensor
                int result2 = (a[2] > a[3]) ? (a[2] - a[3]) : (a[3] - a[2]);
                double res12 = atan(result2 / 8.0) * (180.0 / 3.14159265);
                int angle2 = (int)res12;
                lcd_cmd(0xCE);
                display(angle2);
                __delay_ms(40);
                lcd_cmd(0xC8);  // Set cursor position
                __delay_ms(30);
            }

            // Display distance and control an LED based on the distance
            if (a[i] >= 1 && a[i] <= 5) {
                lcd_string("Alert", 5);
                PORTDbits.RD1 = 1; // Turn on LED and buzzer
                PORTDbits.RD0 = 1;
                __delay_ms(1000); // Delay
                PORTDbits.RD1 = 0; // Turn on LED and buzzer
                PORTDbits.RD0 = 0;
                lcd_cmd(0x10); // Shift right
                lcd_cmd(0x10); // Shift right
                lcd_cmd(0x10); // Shift right
                lcd_cmd(0x10); // Shift right
                lcd_cmd(0x10); // Shift right
                __delay_ms(2);
                lcd_string("  cm ", 5);
                __delay_ms(20); // Delay
            } 
            if (a[i] >= 6 && a[i] <= 99) {
                display(a[i]);
                __delay_ms(20); // Delay
            }

            if (a[i] >= 5 && a[i] <= 10) {
                PORTDbits.RD1 = 1; // Turn on LED and buzzer
                __delay_ms(30); // Delay
                PORTDbits.RD1 = 0; // Turn off LED and buzzer
                __delay_ms(20); // Delay
                PORTDbits.RD1 = 1; // Turn on LED and buzzer
                __delay_ms(30); // Delay
                PORTDbits.RD1 = 0; // Turn on LED and buzzer
                __delay_ms(20); // Delay
                
            }

             
            if (a[i] > 99 || a[i] < 1) {
                lcd_string(" -- ", 4); // Display "FAR"
                __delay_ms(20); // Delay
            }

            __delay_ms(100);     // Delay
        }
        __delay_ms(100);         // Delay
    }
    return;
}

void display(int number) {
    if ((number / 10) > 0) {
        switch (number / 10) {
            case 0:
                lcd_string("0", 1);
                break;
            case 1:
                lcd_string("1", 1);
                break;
            case 2:
                lcd_string("2", 1);
                break;
            case 3:
                lcd_string("3", 1);
                break;
            case 4:
                lcd_string("4", 1);
                break;
            case 5:
                lcd_string("5", 1);
                break;
            case 6:
                lcd_string("6", 1);
                break;
            case 7:
                lcd_string("7", 1);
                break;
            case 8:
                lcd_string("8", 1);
                break;
            case 9:
                lcd_string("9", 1);
                break;
        }
        
        lcd_cmd(0x14); // Shift right
        __delay_ms(20);
        lcd_cmd(0x10); // Shift right
        __delay_ms(20);
    }

    switch (number % 10) {
        case 0:
            lcd_string("0", 1);
            break;
        case 1:
            lcd_string("1", 1);
            break;
        case 2:
            lcd_string("2", 1);
            break;
        case 3:
            lcd_string("3", 1);
            break;
        case 4:
            lcd_string("4", 1);
            break;
        case 5:
            lcd_string("5", 1);
            break;
        case 6:
            lcd_string("6", 1);
            break;
        case 7:
            lcd_string("7", 1);
            break;
        case 8:
            lcd_string("8", 1);
            break;
        case 9:
            lcd_string("9", 1);
            break;
    }
}
