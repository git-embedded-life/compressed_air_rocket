/*
 * File:   main.c
 * Author: jeram
 *
 * Created on May 7, 2022, 7:33 AM
 */

#include "config.h"
#include <xc.h>

#define _XTAL_FREQ              (8000000)
#define TMR_MS                  (33)

#define ARM_DEPRESS_MS          (500)
#define ARM_ABORT_DEPRESS_MS    (100)
#define LAUNCH_DEPRESS_MS       (500)
#define FET_ON_MS               (500)
#define ARMED_DELAY_MS          (2000)
#define ABORT_DELAY_MS          (1000)

#define PORT_LED                PORTAbits.RA1
#define LATCH_LED               LATAbits.LATA1
#define PORT_FET                PORTAbits.RA2
#define LATCH_FET               LATAbits.LATA2
#define PORT_ARM                PORTAbits.RA0
#define PORT_LAUNCH             PORTAbits.RA3

typedef enum
{
    STATE_Reset = 0,
    STATE_Arm_Wait,
    STATE_Armed,
    STATE_Armed_Delay,
    STATE_Abort_Delay,
    STATE_Launch        
} state_t;


void state_machine(int period_ms)
{
    static state_t state = (state_t)0;
    static unsigned long elapsed_next_state_ms;
    static unsigned long elapsed_timeout_ms;
    
    switch (state)
    {
        case STATE_Reset:
            elapsed_next_state_ms = 0;
            elapsed_timeout_ms = 0;
            state = STATE_Arm_Wait;
            PORT_LED = 0;
            PORT_FET = 0;
            break;
        
        case STATE_Arm_Wait:
            if (elapsed_next_state_ms >= ARM_DEPRESS_MS)
            {
                state = STATE_Armed_Delay;
                elapsed_next_state_ms = 0;
                elapsed_timeout_ms = 0;
                break;
            }
            
            if (PORT_ARM == 0)
            {
                elapsed_next_state_ms += (unsigned long)period_ms;
            }
            else
            {
                elapsed_next_state_ms = 0;
            }
            break;
            
        case STATE_Armed_Delay:
            if (LATCH_LED == 0)
            {
                PORT_LED = 1;
            }

            if (elapsed_next_state_ms >= ARMED_DELAY_MS)
            {
                state = STATE_Armed;
                elapsed_next_state_ms = 0;
                elapsed_timeout_ms = 0;
                break;
            }
            
            elapsed_next_state_ms += (unsigned long)period_ms;
            break;
            
        case STATE_Armed:
            if (elapsed_next_state_ms >= LAUNCH_DEPRESS_MS)
            {
                state = STATE_Launch;
                elapsed_next_state_ms = 0;
                elapsed_timeout_ms = 0;
                break;
            }
            
            if (elapsed_timeout_ms >= ARM_ABORT_DEPRESS_MS)
            {
                state = STATE_Abort_Delay;
                elapsed_next_state_ms = 0;
                elapsed_timeout_ms = 0;
               break;
            }
            
            if (PORT_LAUNCH == 0)
            {
                elapsed_next_state_ms += (unsigned long)period_ms;
            }
            else
            {
                elapsed_next_state_ms = 0;
            }
            
            if (PORT_ARM == 0)
            {
                elapsed_timeout_ms += (unsigned long)period_ms;
            }
            else
            {
                elapsed_timeout_ms = 0;
            }
            break;
            
        case STATE_Abort_Delay:
            if (LATCH_LED == 1)
            {
                PORT_LED = 0;
            }
            
            if (elapsed_next_state_ms >= ARMED_DELAY_MS)
            {
                state = STATE_Reset;
                break;
            }
            
            elapsed_next_state_ms += (unsigned long)period_ms;
            break;

        case STATE_Launch:
            if (LATCH_FET == 0)
            {
                PORT_FET = 1;
            }
            
            if (elapsed_next_state_ms >= FET_ON_MS)
            {
                state = STATE_Reset;
                break;
            }
            
            elapsed_next_state_ms += (unsigned long)period_ms;       
            break;
            
        default:
            state = (state_t)0;
    }
}

void main(void) {
    // State machine timer
    OSCCONbits.IRCF = 0b110; // 8MHz
    OPTION_REGbits.T0CS = 0;
    OPTION_REGbits.PSA = 0;
    OPTION_REGbits.PS = 0b111;

    OPTION_REGbits.nWPUEN = 0;
    
    // FET to power valve
    ANSELAbits.ANSA2 = 0;
    TRISAbits.TRISA2 = 0;
    PORT_FET = 0;
    
    // LED indicator
    ANSELAbits.ANSA1 = 0;
    TRISAbits.TRISA1 = 0;
    PORT_LED = 0;
    
    // Arm button
    ANSELAbits.ANSA0 = 0;
    TRISAbits.TRISA0 = 1;
    WPUAbits.WPUA0 = 1;
    
    // Launch button
    WPUAbits.WPUA3 = 1;
    
    for(;;)
    {
        if(INTCONbits.TMR0IF == 1)
        {
            INTCONbits.TMR0IF = 0;
            state_machine(TMR_MS);
        }
    }
    
    return;
}
