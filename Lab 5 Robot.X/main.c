/* 
 * File:   main.c
 * Author: David
 *
 * Created on den 16 maj 2016, 14:20
 */

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection Bits (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover Mode (Internal/External Switchover Mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config STVREN = OFF     // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will not cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LPBOR = OFF      // Low-Power Brown Out Reset (Low-Power BOR is disabled)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

#include <xc.h>
#include <stdint.h>


#define _XTAL_FREQ		20000000	// Den interna klockans frekvens. Ca 2 MHz
#define FOSC			_XTAL_FREQ	// Klockans frekvens
#define TOSC			1/FOSC		// Klockans periodtid

#define FPWM			5000		// 5 kHz, PWM-frekvens
#define TPWM			1/FPWM		// PWM-Periodtid
#define PWM_PRESCALER	4

#define TIMER0_PULSES 3				// Antalet pulser som det tar att ändra LED
uint8_t timer10ms = TIMER0_PULSES;	// Räknas ned vid interrupt

/*
 * Motor driver
 * Left
 *		B1	PWM4	RC1
 *		B2	PWM3	RA2	
 * Right
 *		A2	PWM2	RC3
 *		A1	PWM1	RC5
 */
#define LEFT_PWM_F		LATCbits.RC1
#define LEFT_PWM_B		LATAbits.RA2
#define RIGHT_PWM_F		LATCbits.RC3
#define RIGHT_PWM_B		LATCbits.RC5
#define LED				LATBbits.RB7



void systemInit(void);
void interrupt ISR(void);

void main(void)
{
	systemInit();
	
	while(1)
	{
		
	}
}


void systemInit()
{
	TRISCbits.TRISC1 = 0;	// LEFT_PWM_F
	TRISAbits.TRISA2 = 0;	// LEFT_PWM_B
	TRISCbits.TRISC3 = 0;	// RIGHT_PWM_F
	TRISCbits.TRISC5 = 0;	// RIGHT_PWM_B	
	TRISBbits.TRISB7 = 0;	// LED		
	
    // timer0 initiering (se 160502_timer.txt)
	OPTION_REG = 0b11010100;
	INTCON	   = 0b10100000;
	
	// Timer2 initiering
	T2CON = 0b00000111;		// Aktiverar Timer 2, prescaler = 16
	
	// PR2 initiering 
	PR2 = (FOSC / (4 * FPWM * PWM_PRESCALER) ) - 1;
	
	// PWM initiering
	CCP1CON = 0b00001100;		// Ställer in PWM samt bit <4:5> är första bitarna åt duty-cycle
	CCPR1L  = LED_Brightness;	// Håller värdet för de 8 sista bitar åt duty-cycle				
}

void interrupt ISR(void)
{
	
}