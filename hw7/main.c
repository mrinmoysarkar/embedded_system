#include "msp.h"

int main(void)
{
    // Hold the watchdog

    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    P2->DIR |= BIT4;     //Make P2.6 an output pin Green LED on MKII
    P2->DIR |= BIT6;     //Make P5.6 an output pin Red LED  on MKII
    P2->OUT &= ~BIT4;   //turn green off initially
    P2->OUT &= ~BIT6;   //Turn red LED off initially

    P3->DIR &= ~BIT5;    //Make P3.5 an input pin
    P3->REN = BIT5;     // Enable pull resistor on P3.5
    P3->OUT = BIT5;      // Connect the pull resistor to Vcc (i.e. make it a pull-up)

    P3->IES = BIT5;      // Interrupt on high-to-low transition (Falling edged trigger on P3.5)
    P3->IFG = 0;         // Clear all P3 interrupt flags
    P3->IE = BIT5;      // Enable interrupt for P3.5 (Pin interrupt enable)

    // Enable Port 3 interrupt on the NVIC (Port Interrupt Enable & Prioriterize)
    NVIC->ISER[1] =  BIT5;///This Sets the P3 interrupt enable bit in the ISER1 register



    P2->DIR |= BIT1;     //Make P2.6 an output pin Green LED on Launchpad
    P2->DIR |= BIT2;     //Make P5.6 an output pin Blue LED  on Launchpad
    P2->OUT &= ~BIT1;   //turn green off initially
    P2->OUT &= ~BIT2;   //Turn blue LED off initially

    P1->DIR &= ~BIT1;    //Make P1.1 an input pin
    P1->REN = BIT1;     // Enable pull resistor on P1.1
    P1->OUT = BIT1;      // Connect the pull resistor to Vcc (i.e. make it a pull-up)

    P1->IES = BIT1;      // Interrupt on high-to-low transition (Falling edged trigger on P1.1)
    P1->IFG = 0;         // Clear all P1 interrupt flags
    P1->IE = BIT1;      // Enable interrupt for P1.1 (Pin interrupt enable)

    // Enable Port 1 interrupt on the NVIC (Port Interrupt Enable & Prioriterize)
    NVIC->ISER[1] =  BIT3;///This Sets the P1 interrupt enable bit in the ISER1 register

    NVIC->IP[PORT1_IRQn] = 0;//Set the interrupt priority to 0 for port P1 interrupt
    NVIC->IP[PORT3_IRQn] = 64;//Set the interrupt priority to 64 for port P3 interrupt

   __enable_interrupts();           //Global Enable prioritized interrupts

    while(1){
    // Go to LPM4 (Low Power Mode #4)
        __sleep();
    }

}

/* Port1 ISR */
void PORT1_IRQHandler(void)
{
    // Blinking the output on the Turquoise light on Launchpad IF P1.1 is what caused the interrupt
     if(P1->IFG & BIT1)
     {
         P2->OUT ^= BIT1;
         P2->OUT ^= BIT2;
         __delay_cycles(6000000); // 2 second delay
         P2->OUT ^= BIT1;
         P2->OUT ^= BIT2;
         __delay_cycles(6000000); // 2 second delay
     }

    P1->IFG &= ~BIT1;       //clear the flag so that the interrupt will not immediately happen again
}

/* Port3 ISR */
void PORT3_IRQHandler(void)
{
    // Toggling the output on the Yellow light on MKII IF P3.5 is what caused the interrupt
     if(P3->IFG & BIT5)
     {
         P2->OUT ^= BIT4;
         P2->OUT ^= BIT6;
         __delay_cycles(6000000); // 2 second delay
         P2->OUT ^= BIT4;
         P2->OUT ^= BIT6;
         __delay_cycles(6000000); // 2 second delay
     }


    P3->IFG &= ~BIT5;       //clear the flag so that the interrupt will not immediately happen again
}

