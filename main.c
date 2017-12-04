/* 
 * File:   main.c
 * Author: KDS
 *
 * Created on November 22, 2017, 7:53 PM
 * Last Edited - 29/11/17
 * 
 * This code takes the output of the heart rate circuit,
 * converts it to a pulse
 * counts the number of pulses over a 10 second period
 * multiplies by 6 to get what the value would be for a minute 
 * and outputs the value to the screen
 */

//INT0 is used for the keypad interrupt pin
//RB3 is used for the temperature pin
//INT2 is used for the heart rate interrupt pin


/*________________________________Heart_Rate_______________________________*/

#include <p18f452.h>
#include "xlcd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <delays.h>
#include <timers.h>



#pragma config OSC = HS
#pragma config WDT = OFF
#pragma config LVP = OFF

#define _XTAL_FREQ 4000000


//Global Vars
int i;
char k[1];
char ClrScn[20] = "                   ";
int tc = 0;
int risingEdges=0;
int bpm;


/*---------------------------Delays---------------------------*/
void DelayFor18TCY(void)
{
    Delay1TCY();
 	Delay1TCY();
    Delay1TCY();
    Delay1TCY();
 	Delay1TCY();
    Delay1TCY();
 	Delay1TCY();
 	Delay1TCY();
 	Delay10TCYx(1);
}
 
void DelayXLCD(void)     // minimum 5ms
{
    Delay1KTCYx(5); 		// Delay of 5ms
                            // Cycles = (TimeDelay * Fosc) / 4
                            // Cycles = (5ms * 4MHz) / 4
                            // Cycles = 5,000

}

void DelayPORXLCD(void)   // minimum 15ms
{
    Delay1KTCYx(15);		// Delay of 15ms
                            // Cycles = (TimeDelay * Fosc) / 4
                            // Cycles = (15ms * 4MHz) / 4
                            // Cycles = 15,000

}

/*---------------------------LCD---------------------------*/

 void LCD_setup(void){
    PORTD = 0X00;
    TRISD = 0x00;
    
    OpenXLCD(FOUR_BIT & LINES_5X7);
    while(BusyXLCD());
    SetDDRamAddr(0x00);              //Start writing at top left hand side of screen
    WriteCmdXLCD( SHIFT_DISP_LEFT );
    while(BusyXLCD());
    WriteCmdXLCD( BLINK_ON );
    while(BusyXLCD());
 }
 
 void clrscn(void){
    while(BusyXLCD());
    SetDDRamAddr(0x00);        //1st row
    while(BusyXLCD());
    putsXLCD(ClrScn);           //clear row

    while(BusyXLCD());
    SetDDRamAddr(0x40);        //2nd row
    while(BusyXLCD());
    putsXLCD(ClrScn);           //clear row

    while(BusyXLCD());
    SetDDRamAddr(0x10);        //3rd row
    while(BusyXLCD());
    putsXLCD(ClrScn);           //clear row

    while(BusyXLCD());
    SetDDRamAddr(0x50);        //4th row
    while(BusyXLCD());
    putsXLCD(ClrScn);          //clear row
}
 
 /*---------------------------TMR0---------------------------*/
 
 void TMR0_setup(void){
    OpenTimer0(TIMER_INT_ON & T0_SOURCE_INT & T0_PS_1_128 & T0_16BIT );
}
 
 
 /*---------------------------HR---------------------------*/
 
void disp_BPM(int x){
    char bpmDisplay[80];
    sprintf(bpmDisplay, "%d bpm         ",x);
    while( BusyXLCD() );
    SetDDRamAddr( 0x00 );
    putrsXLCD("Heart Rate: ");
    while( BusyXLCD() );
    SetDDRamAddr( 0x40 );
    putsXLCD(bpmDisplay);
}
 
 
/*------------------------------INTERRUPT---------------------------*/
void HR_ISR(void);

#pragma code high_vector = 0x08
 void high_interrupt_vector(void){
     _asm 
     GOTO HR_ISR
     _endasm
 }
#pragma code 

#pragma interrupt HR_ISR
void HR_ISR (void){
	if (INTCONbits.TMR0IF==1){      //check if TMR0 has overflowed
        INTCONbits.TMR0IE=0;        //disable interrupts
        INTCONbits.TMR0IF = 0;      //clear interrupt flag
        
        tc++;                       //increment counter
        if(tc==10)
        {// if time counter is equal to 10 (10 seconds have passed))
            tc = 0;                 // reset 1s counter
            
            bpm = risingEdges*6;   //multiply the number of rising edges by 6 for bpm
            disp_BPM(bpm);         //output bpm

            WriteTimer0(57546);         //prime timer for 1s 
           
            //reset counter for next time               
            risingEdges=0; 
        }
        else
        {// if 10s did not pass yet
            WriteTimer0(57546);         //prime TMR0 for 1s
        }
        
        INTCONbits.TMR0IE=1;
    }
    
    //beat counter
    if (INTCON3bits.INT2IF==1 ) {//if there is an interrupt at the int2 pin
        INTCONbits.TMR0IE = 0;
        INTCON3bits.INT2IE = 0 ;
        
        
        risingEdges++;              //increase rising edge counter
        INTCON3bits.INT2IF = 0;     //Clear INT2 Interrupt Flag
        
        
        INTCONbits.TMR0IE = 1;
        INTCON3bits.INT2IE = 1;
    }
    
    
} 


//____________________MAIN CODE_____________________//

void main(){
    bpm = 0;    

    //Setup Interrupts
    INTCON3bits.INT2IF = 0;         //Clear INT2 Interrupt Flag
    INTCON3bits.INT2IE = 1 ;        //Enable INT2 Interrupts  
    INTCONbits.TMR0IF = 0;          //Clear TMR0 Interrupt Flag
    INTCONbits.TMR0IE = 1;          //Enable TMR0 Interrupt
    INTCON2bits.TMR0IP = 1;         //Set TMR0 as High Priority
    INTCON2bits.INTEDG0 = 1;        //Set interrupts to occur on rising edges
    INTCONbits.GIEH= 1;             //Enable Global Interrupts last to prevent unwanted interrupts. 
    
    //Set up PORT B pins
    TRISBbits.RB2 = 1;
    TRISBbits.RB7 = 0;
    PORTBbits.RB7 = 0x00;
    
    //setup LCD and TMR0
    TMR0_setup();
    LCD_setup();
    
    
    WriteTimer0(57546);             // prime TMR0 for 1s
              
    while(1){
        disp_BPM(bpm);    
    }
    
}