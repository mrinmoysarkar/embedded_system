#include "msp.h"


/**
 * main.c
 */

#define TIMER_PERIOD 30000  // equivalent to 10ms

void initPorts(void);
void SysTick_Init(void);

enum TG_States { Tg_SMStart, Tg1, Tg2, Tg3, Tg4 } tgstate; // toggle states
enum TG_States TickFct_ToggleLED(enum TG_States state); // tick function for toggle led

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	tgstate = Tg_SMStart; // initialize the toggle state
	initPorts(); //initialize port
    SysTick_Init(); // initialize timer
    __enable_irq(); // enable global interrupt

    while(1) {
     __sleep(); // sleep the main loop
    }
}


void SysTick_Handler(void)
{
    tgstate = TickFct_ToggleLED(tgstate);
}

void SysTick_Init(void)
{
    CS->KEY = CS_KEY_VAL ;                  // Unlock CS module for register access
    CS->CTL0 = 0;                           // Reset tuning parameters
    CS->CTL0 |= CS_CTL0_DCORSEL_1;           // Set DCO to 3MHz
    CS->KEY = 0;                            // Lock CS module from unintended accesses
    SysTick->LOAD = TIMER_PERIOD; // set timer period
    SysTick->VAL = 0; // initialize counter with value 0
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk  | SysTick_CTRL_TICKINT_Msk; // enable timer interrupt
}

void initPorts(void)
{
    // configure switch 2 of Launch pad as input
    P1->DIR &= ~BIT4;
    P1->REN = BIT4;
    P1->OUT = BIT4;
    // configure green led of Launch pad as output
    P2->DIR |= BIT1;
    P2->OUT = 0x00;
}

enum TG_States TickFct_ToggleLED(enum TG_States state)
{
   switch(state)
   {
       case Tg_SMStart:
           state = Tg1;
           break;
       case Tg1:
           if((P1->IN & BIT4) == 0x00)
           {
               state = Tg2;
           }
           break;
       case Tg2:
           if((P1->IN & BIT4) != 0x00)
           {
              state = Tg3;
           }
           break;
       case Tg3:
           if((P1->IN & BIT4) == 0x00)
           {
              state = Tg4;
           }
          break;
       case Tg4:
           if((P1->IN & BIT4) != 0x00)
           {
              state = Tg1;
           }
          break;
       default:
          state = Tg1;
          break;
   }
   switch(state)
   {
       case Tg1:
           P2->OUT &= ~BIT1; // turn off green led
           break;
       case Tg2:
           P2->OUT |= BIT1; // turn on green led
           break;
       case Tg3:
           P2->OUT |= BIT1; // turn on green led
           break;
       case Tg4:
           P2->OUT &= ~BIT1; // turn off green led
           break;
   }
   return state;
}
