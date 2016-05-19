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


#define _XTAL_FREQ		16000000		// Den interna klockans frekvens. Ca 2 MHz
#define FOSC			_XTAL_FREQ	// Klockans frekvens
#define TOSC			1/FOSC		// Klockans periodtid

#define FPWM			5000		// 5 kHz, PWM-frekvens
#define TPWM			1/FPWM		// PWM-Periodtid
#define PWM_PRESCALER	4
#define PWM_MAX			200

#define TIMER0_PULSES   10			// Antalet pulser som det tar att ändra LED
int8_t timer = TIMER0_PULSES;	// Räknas ned vid interrupt

char speed = 0;

/*
 * Motor driver
 * Left - Yellow
 *		A2	PWM2	RC3 Forward
 *		A1	PWM1	RC5	Back

 * Right - Green
 *		B2	PWM3	RA2	Forward
 *		B1	PWM4	RC1	Back
 */


#define LEFT_PWM_F		PWM2DCH
#define LEFT_PWM_B		PWM1DCH
#define RIGHT_PWM_F		PWM3DCH
#define RIGHT_PWM_B		PWM4DCH
#define LED				LATBbits.LATB7
#define BUTTON			PORTBbits.RB6
#define SENSOR			PORTBbits.RB5


void systemInit(void);
void interrupt ISR(void);

void main(void)
{
	systemInit();
	char run = 0;
	
	
	LED = 0;
	while(1){
#if 1
			if(SENSOR)
				LED = 1;
			else 
				LED = 0;
				
#else
		if(BUTTON)
			run = 1;
		
		if(run)
		{
			static char state = 0;

			switch(state)
			{
			case 0:
				LED = 0;
				if(timer <= 0)
				{
					speed++;
					timer = TIMER0_PULSES;
				}

				RIGHT_PWM_F = speed;
				LEFT_PWM_F = speed;

				if(speed == PWM_MAX)
				{
					state = 1;
					__delay_ms(2000);
				}
				break;

			case 1:
				LED = 0;
				if(timer <= 0)
				{
					speed--;
					timer = TIMER0_PULSES;
				}

				RIGHT_PWM_F = speed;
				LEFT_PWM_F = speed;

				if(speed == 0)
				{
					state = 2;
					RIGHT_PWM_F = 0;
					LEFT_PWM_F = 0;
				}

				break;
			case 2:
				LED = 1;
				if(timer <= 0)
				{
					speed++;
					timer = TIMER0_PULSES;
				}

				RIGHT_PWM_B = speed;
				LEFT_PWM_B = speed;

				if(speed == PWM_MAX)
				{
					RIGHT_PWM_B = PWM_MAX;
					LEFT_PWM_B = PWM_MAX;
					state = 3;
				}
				break;

			case 3:
				LED = 1;
				if(timer <= 0)
				{
					speed--;
					timer = TIMER0_PULSES;
				}

				RIGHT_PWM_B = speed;
				LEFT_PWM_B = speed;

				if(speed == 0)
				{
					state = 0;
					RIGHT_PWM_B = 0;
					LEFT_PWM_B = 0;
				}

				break;
			}
		}
#endif
	}
}

void systemInit()
{
	TRISCbits.TRISC1 = 0;	
	TRISAbits.TRISA2 = 0;	
	TRISCbits.TRISC3 = 0;	
	TRISCbits.TRISC5 = 0;	
	TRISBbits.TRISB7 = 0;	// LED		
	TRISBbits.TRISB6 = 1;	// Button
	
	TRISBbits.TRISB5 = 1;	// IR-Sensor 8
	
	OSCCON = 0b01111010;	// 16 MHz

    // timer0 initiering (se 160502_timer.txt)
	OPTION_REG = 0b11010100;
	INTCON	   = 0b10100000;
	
	// Timer2 initiering
	T2CON = 0b00000111;		// Aktiverar Timer 2, prescaler = 16
	
	// PR2 initiering 
	PR2 = (FOSC / (4 * FPWM * PWM_PRESCALER) ) - 1;
	
	// PWM initiering
	PWM1CON = 0b11000000;
	PWM2CON = 0b11000000;
	PWM3CON = 0b11000000;
	PWM4CON = 0b11000000;
	
	PWM1DCH = 0; // PWM Duty Cycle High Bits (de 8 största bitarna)
	PWM1DCL = 0;
	PWM2DCH = 0;
	PWM2DCL = 0;
	PWM3DCH = 0; 
	PWM3DCL = 0; 
	PWM4DCH = 0;
	PWM4DCL = 0;
	
	ANSELB = 0;

}

void interrupt ISR(void)
{
	GIE = 0;
    if(T0IE && T0IF)
    {
		timer--;
        T0IF = 0;
    }
	GIE = 1;
}