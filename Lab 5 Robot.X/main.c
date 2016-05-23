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

#define SPEED_MAX		PWM_MAX
#define deltaSpeed		3	

#define TIMER0_PULSES   2			// Antalet pulser som det tar att ändra LED
int8_t timer0 = TIMER0_PULSES;		// Räknas ned vid interrupt


/*
 * Motor driver
 * Left - Yellow
 *		A2	PWM2	RC3 Forward
 *		A1	PWM1	RC5	Back

 * Right - Green
 *		B2	PWM3	RA2	Forward
 *		B1	PWM4	RC1	Back
 */

// Outputs
#define LEFT_PWM_F		PWM2DCH
#define LEFT_PWM_B		PWM1DCH
#define RIGHT_PWM_F		PWM3DCH
#define RIGHT_PWM_B		PWM4DCH
#define LED				LATBbits.LATB7

// Inputs
#define SENSOR_1		PORTAbits.RA4
#define SENSOR_2		PORTCbits.RC4
#define SENSOR_3		PORTCbits.RC6
#define SENSOR_4		PORTCbits.RC7
#define SENSOR_5		PORTCbits.RC0
#define SENSOR_6		PORTCbits.RC2
#define SENSOR_7		PORTBbits.RB4
#define SENSOR_8		PORTBbits.RB5
#define BUTTON			PORTBbits.RB6


// === Declarations ===
void systemInit(void);
void interrupt ISR(void);
void forwBack(void);
void followTape(void);
void startMotors(void);

// ====== main ======
void main(void)
{
	systemInit();
	char run = 0;
	
	LED = 1;
	
	while(1){
#if 0 // debug
		
		RIGHT_PWM_F = SPEED_MAX;
		LEFT_PWM_F = SPEED_MAX;
				
#else // main program
		if(BUTTON)
			run = 1;
		
		if(run)
		{
			followTape();
		} // if(run)
#endif
	} // while(1)
} // main

void systemInit()
{	
	// Outputs
	TRISCbits.TRISC1 = 0; 	// PWM outputs	
	TRISAbits.TRISA2 = 0;	
	TRISCbits.TRISC3 = 0;	
	TRISCbits.TRISC5 = 0;
	TRISBbits.TRISB7 = 0;	// LED		

	// Inputs
	TRISBbits.TRISB6 = 1;	// Button
	
	TRISAbits.TRISA4 = 1;	// IR-Sensor 1
	TRISCbits.TRISC4 = 1;	// IR-Sensor 2
	TRISCbits.TRISC6 = 1;	// IR-Sensor 3
	TRISCbits.TRISC7 = 1;	// IR-Sensor 4
	TRISCbits.TRISC0 = 1;	// IR-Sensor 5
	TRISCbits.TRISC2 = 1;	// IR-Sensor 6
	TRISBbits.TRISB4 = 1;	// IR-Sensor 7
	TRISBbits.TRISB5 = 1;	// IR-Sensor 8

	// Ports set to digital
	ANSELB = 0;				
	ANSELC = 0;	
	ANSELA = 0;
	
	// Internal clock
	OSCCON = 0b01111010;	// 16 MHz

    // timer00 initiering till interrupt 
	OPTION_REG = 0b11010000;
	INTCON	   = 0b10100000;
	
	// Timer2 initiering till PWM
	T2CON = 0b00000111;		// Aktiverar Timer 2, prescaler = 16
	
	// PR2 initiering 
	PR2 = (FOSC / (4 * FPWM * PWM_PRESCALER) ) - 1;
	
	// PWM initiering
	PWM1CON = 0b11000000;
	PWM2CON = 0b11000000;
	PWM3CON = 0b11000000;
	PWM4CON = 0b11000000;
	
	// PWM Duty Cycle 
	PWM1DCH = 0; // High Bits (de 8 största bitarna)
	PWM1DCL = 0; // Low Bits (de 2 minsta bitarna)
	PWM2DCH = 0;
	PWM2DCL = 0;
	PWM3DCH = 0; 
	PWM3DCL = 0; 
	PWM4DCH = 0;
	PWM4DCL = 0;
}

void interrupt ISR(void)
{
	GIE = 0;
    if(T0IE && T0IF)
    {
		timer0--;
        T0IF = 0;
    }
	GIE = 1;
}

void forwBack(void)
{
	static char state = 0;
	static char speed = 0;
	
	switch(state)
	{
	case 0:	
		LED = 0;
		if(timer0 <= 0)
		{
			speed++;
			timer0 = TIMER0_PULSES;
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
		if(timer0 <= 0)
		{
			speed--;
			timer0 = TIMER0_PULSES;
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
		if(timer0 <= 0)
		{
			speed++;
			timer0 = TIMER0_PULSES;
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
		if(timer0 <= 0)
		{
			speed--;
			timer0 = TIMER0_PULSES;
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
	}	// end switch
}

void followTape(void)
{
	static uint8_t state = 1;
	
	// Temporär hastighet som den skall ställa in sig till
	static uint8_t tempSpeed_Left_Forw = SPEED_MAX;
	static uint8_t tempSpeed_Right_Forw = SPEED_MAX;
	static uint8_t tempSpeed_Left_Back = 0;
	static uint8_t tempSpeed_Right_Back = 0;
	
	// Den aktuella hastigheten som motorerna sätts till varje cykel
	static uint8_t speed_Left_Forw = 0;
	static uint8_t speed_Right_Forw = 0;
	static uint8_t speed_Left_Back = 0;
	static uint8_t speed_Right_Back = 0;
	
	switch(state)
	{
		case 1: // Refresh
		
			// Accelerering till tempSpeed
			if(timer0 <= 0)
			{
				// Left
				if (speed_Left_Forw > 0 && tempSpeed_Left_Back)
				{
					tempSpeed_Left_Forw = 0;
				}
				
				// Accelerera
				if (tempSpeed_Left_Forw > speed_Left_Forw)
				{
					if (speed_Left_Forw + deltaSpeed >= SPEED_MAX - 2)
						speed_Left_Forw = SPEED_MAX;
					else
						speed_Left_Forw += deltaSpeed;
				}
				// Retardera
				else if (tempSpeed_Left_Forw < speed_Left_Forw)
				{
					if (speed_Left_Forw <= deltaSpeed)
						speed_Left_Forw = 0;
					else
						speed_Left_Forw -= deltaSpeed;
				}
				
				// Right
				if (speed_Right_Forw > 0 && tempSpeed_Right_Back)
				{
					tempSpeed_Right_Forw = 0;
				}
				
				
				if (tempSpeed_Right_Forw > speed_Right_Forw)
				{
					if (speed_Right_Forw + deltaSpeed >= SPEED_MAX - 2)
						speed_Right_Forw = SPEED_MAX;
					else
						speed_Right_Forw += deltaSpeed;
				}
				else if (tempSpeed_Right_Forw < speed_Right_Forw)
				{
					if (speed_Right_Forw <= deltaSpeed)
						speed_Right_Forw = 0;
					else
						speed_Right_Forw -= deltaSpeed;
				}	
				
				timer0 = TIMER0_PULSES;
			}
			
			LEFT_PWM_F = speed_Left_Forw;
			RIGHT_PWM_F = speed_Right_Forw;
			LEFT_PWM_B = speed_Left_Back;
			RIGHT_PWM_B = speed_Right_Back;
			
			
			// Sensorkoll
			if (SENSOR_4 || SENSOR_5)
				state = 2; // Forward
			
			if (SENSOR_6)
				state = 3; // Left 1

			if (SENSOR_7)
				state = 4; // Left 2
			
			if (SENSOR_8)
				state = 5; // Left 3

			if (SENSOR_3 || SENSOR_2)
				state = 6; // Right 1

			if (SENSOR_2)
				state = 7; // Right 2

			if (SENSOR_1)
				state = 8; // Right 3
			
			if (SENSOR_1 && SENSOR_8)
				state = 8;
			

			break;
		
		case 2: // Forward
			LED = 0;
			tempSpeed_Left_Forw = SPEED_MAX;
			tempSpeed_Right_Forw = SPEED_MAX;
			speed_Left_Back = 0;
			speed_Right_Back = 0;
			
			
			state = 1;
			
			break;
		case 3: // Left 1
			LED = 1;
			tempSpeed_Left_Forw = 40;
			tempSpeed_Right_Forw = SPEED_MAX;
			speed_Left_Back = 0;
			speed_Right_Back = 0;
			
			state = 1;
			
			break;
		case 4: // Left 2
			LED = 1;
			tempSpeed_Left_Forw = 20;
			tempSpeed_Right_Forw = SPEED_MAX;
			speed_Left_Back = 0;
			speed_Right_Back = 0;
			
			state = 1;
			
			break;
			
		case 5: // Left 3
			LED = 1;
			tempSpeed_Left_Forw = 0;
			tempSpeed_Right_Forw = SPEED_MAX;
			speed_Left_Back = 40;
			speed_Right_Back = 0;
			
			state = 1;
			
			break;
		case 6: // Right 1
			LED = 1;
			tempSpeed_Left_Forw = SPEED_MAX;
			tempSpeed_Right_Forw = 40;
			speed_Left_Back = 0;
			speed_Right_Back = 0;
			
			
			state = 1;
			
			break;
		case 7: // Right 2
			LED = 1;
			tempSpeed_Left_Forw = SPEED_MAX;
			tempSpeed_Right_Forw = 20;
			speed_Left_Back = 0;
			speed_Right_Back = 0;
			
			
			state = 1;
		
		case 8: // Right 3
			LED = 1;
			tempSpeed_Left_Forw = SPEED_MAX;
			tempSpeed_Right_Forw = 0;
			speed_Left_Back = 0;
			speed_Right_Back = 40;
			
			
			state = 1;
			
			break;
		case 9:	
			LED = 1;
			tempSpeed_Left_Forw = 0;
			tempSpeed_Right_Forw = 0;
			
			state = 1;
			
			break;
		default:
			state = 1;
			break;
	};
}
