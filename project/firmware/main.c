/*
 * Author: Mrinmoy sarkar
 * email: msarkar@aggies.ncat.edu
 * Date: 10/8/2019
 */


#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include <stdio.h>

/* Graphic library context */
Graphics_Context g_sContext;

/* ADC results buffer */
static uint16_t resultsBuffer_ACC[3];
static uint16_t resultsBuffer_JOY[2];

uint8_t image[16384];
uint32_t pallet[256];

char codes[] = "START";
int codeindex=0;
int dummyflag = 0;

int index = 0;
int palletindex = 0;
int imageindex = 0;
int imageOrpallet = -1;//0 means image 1 means pallet
//int updateLCD = 0;
uint32_t palletData = 0;
Graphics_Image bitmap;


const eUSCI_UART_Config uartConfig =
{
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
    6,                                     // BRDIV = 78
    8,                                       // UCxBRF = 2
    32,                                       // UCxBRS = 0
    EUSCI_A_UART_NO_PARITY,                  // No Parity
    EUSCI_A_UART_LSB_FIRST,                  // LSB First
    EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
    EUSCI_A_UART_MODE,                       // UART mode
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
};


void processReceivedData(uint8_t data);

enum AR_States { AR_SMStart, AR_IDEAL, AR_PRESS_LR, AR_PRESS_UD} ar_state;
enum AR_States TickFct_Arrow(enum AR_States state);

enum BTN_States { BTN_SMStart, BTN_IDEAL, BTN_PRESS_SPACE, BTN_PRESS_X, BTN_PRESS_Z} btn_state;
enum BTN_States TickFct_Button(enum BTN_States state);

//enum IMG_States { IMG_SMStart, IMG_IDEAL, IMG_UPDATE} img_state;
//enum IMG_States TickFct_Image(enum IMG_States state);

int done = 0;
int main(void)
{
    /* Halting WDT and disabling master interrupts */
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();

    bitmap.bPP = 8;
    bitmap.numColors = 256;
    bitmap.pPalette = pallet;
    bitmap.pPixel = image;
    bitmap.xSize = 128;
    bitmap.ySize = 128;

    ar_state = AR_SMStart;
    btn_state = BTN_SMStart;
//    img_state = IMG_SMStart;

    /* Set the core voltage level to VCORE1 */
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

    /* Set 2 flash wait states for Flash bank 0 and 1*/
    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    /* Initializes Clock System */
    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN1);

    /* Selecting P1.2 and P1.3 in UART mode */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
              GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

      /* Setting DCO to 12MHz */
      CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);


      /* Configuring UART Module */
      MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

      /* Enable UART module */
      MAP_UART_enableModule(EUSCI_A0_BASE);

      /* Enabling interrupts */
      MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
      MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
      MAP_Interrupt_enableSleepOnIsrExit();
      MAP_Interrupt_enableMaster();


    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);

    /* Configures Pin 4.0, 4.2, and 6.1 as ADC input */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN0 | GPIO_PIN2, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN1, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Configures Pin 6.0 and 4.4 as ADC input */
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    /* Initializing ADC (ADCOSC/64/8) */
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);




    /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM2 (A11, A13, A14)  with no repeat)
         * with internal 2.5v reference */
    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM4, true);
    MAP_ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A14, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A13, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM2,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A11, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM3,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_configureConversionMemory(ADC_MEM4,
                ADC_VREFPOS_AVCC_VREFNEG_VSS,
                ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    /* Enabling the interrupt when a conversion on channel 2 (end of sequence)
     *  is complete and enabling conversions */
    MAP_ADC14_enableInterrupt(ADC_INT1|ADC_INT2);

    /* Enabling Interrupts */
    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableMaster();

    /* Setting up the sample timer to automatically step through the sequence
     * convert.
     */
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    /* Triggering the start of the sample */
    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();

    while(1)
    {
        MAP_PCM_gotoLPM0();
    }
}



/* EUSCI A0 UART ISR */
void EUSCIA0_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);
    MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);
    uint8_t data;
    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        data = MAP_UART_receiveData(EUSCI_A0_BASE);
        processReceivedData(data);
    }
}

/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM2. This signals the end of conversion and the results array is
 * grabbed and placed in resultsBuffer_ACC */
void ADC14_IRQHandler(void)
{
    uint64_t status;
    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);

    /* ADC_MEM2 conversion completed */
    if(status & ADC_INT2)
    {
        /* Store ADC14 conversion results */
        resultsBuffer_ACC[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer_ACC[1] = ADC14_getResult(ADC_MEM1);
        resultsBuffer_ACC[2] = ADC14_getResult(ADC_MEM2);
    }

    if(status & ADC_INT1)
    {
        resultsBuffer_JOY[0] = ADC14_getResult(ADC_MEM3);
        resultsBuffer_JOY[1] = ADC14_getResult(ADC_MEM4);
        ar_state = TickFct_Arrow(ar_state);
        btn_state = TickFct_Button(btn_state);
    }
}


void processReceivedData(uint8_t data)
{
    if(codeindex==5)
    {
        codeindex=0;
        if((char)data == 'I')
        {
            imageindex = 0;
            imageOrpallet = 0;
            dummyflag=1;
        }
        if((char)data == 'P')
        {
            palletindex = 0;
            imageOrpallet = 1;
            index = 0;
            dummyflag=1;
        }
    }

    if(codes[codeindex] == (char)data)
    {
        codeindex++;
    }
    else
    {
        codeindex = 0;
    }

    if(imageOrpallet==0 && dummyflag==0)
    {
        image[imageindex++] = data;
        if(imageindex == 16384)
        {
            imageindex = 0;
            imageOrpallet = -1;
        }
    }
    else if(imageOrpallet==1 && dummyflag==0)
    {
        if (index == 0) {palletData |= ((uint32_t) data) << 16; index++;}
        else if (index == 1) {palletData |= ((uint32_t) data) << 8; index++;}
        else if (index == 2) {palletData |= ((uint32_t) data); index=0; pallet[palletindex]=palletData; palletData=0; palletindex++;}
        if(palletindex==256)
        {
            palletindex = 0;
            if(imageindex==0 && palletindex==0)
            {
                Graphics_drawImage(&g_sContext, &bitmap, 0, 0);
            }
        }
    }
    dummyflag=0;
}

//enum IMG_States TickFct_Image(enum IMG_States state)
//{
//    switch(state)
//    {
//        case IMG_SMStart:
//            state = IMG_IDEAL;
//            break;
//        case IMG_IDEAL:
//            if(updateLCD==1)
//            {
//                state = IMG_UPDATE;
//            }
//            break;
//        case IMG_UPDATE:
//            Graphics_drawImage(&g_sContext, &bitmap, 0, 0);
//            state = IMG_IDEAL;
//            updateLCD = 0;
//            break;
//        default:
//            state = IMG_IDEAL;
//            break;
//    }
//    return state;
//}


enum BTN_States TickFct_Button(enum BTN_States state)
{
    switch(state)
    {
        case BTN_SMStart:
            state = BTN_IDEAL;
            break;
        case BTN_IDEAL:
            if(GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN5) == GPIO_INPUT_PIN_LOW)
            {
                char *str = "Z\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_PRESS_Z;
            }
            else if(GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN1) == GPIO_INPUT_PIN_LOW)
            {
                char *str = "SPACE_BAR\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_PRESS_SPACE;
            }
            else if(GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN1) == GPIO_INPUT_PIN_LOW)
            {
                char *str = "X\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_PRESS_X;
            }
            break;
        case BTN_PRESS_Z:
            if(GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN5) != GPIO_INPUT_PIN_LOW)
            {
                char *str = "Z_UP\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_IDEAL;
            }
            break;
        case BTN_PRESS_SPACE:
            if(GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN1) != GPIO_INPUT_PIN_LOW)
            {
                char *str = "SPACE_BAR_UP\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_IDEAL;
            }
            break;
        case BTN_PRESS_X:
            if(GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN1) != GPIO_INPUT_PIN_LOW)
            {
                char *str = "X_UP\n";
                while(*str != 0)
                {
                    MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
                }
                state = BTN_IDEAL;
            }
            break;
        default:
            state = BTN_SMStart;
            break;
    }
    return state;
}

enum AR_States TickFct_Arrow(enum AR_States state) {
  switch(state)
  {
     case AR_SMStart:
        state = AR_IDEAL;
        break;
     case AR_IDEAL:
         if(resultsBuffer_JOY[0] > 15000)
         {
            char *str = "RIGHT_ARROW\n";
            while(*str != 0)
            {
                MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
            }
            state = AR_PRESS_LR;
         }
         else if(resultsBuffer_JOY[0] < 50)
         {
           char *str = "LEFT_ARROW\n";
           while(*str != 0)
           {
               MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
           }
           state = AR_PRESS_LR;
         }
         else if(resultsBuffer_JOY[1] > 15000)
         {
           char *str = "UP_ARROW\n";
           while(*str != 0)
           {
               MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
           }
           state = AR_PRESS_UD;
         }
         else if(resultsBuffer_JOY[1] < 50)
         {
           char *str = "DOWN_ARROW\n";
           while(*str != 0)
           {
               MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
           }
           state = AR_PRESS_UD;
         }
        break;
     case AR_PRESS_LR:
         if(resultsBuffer_JOY[0]>6500 && resultsBuffer_JOY[0] <8000)
         {
             char *str = "LR_ARROW_UP\n";
             while(*str != 0)
             {
               MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
             }
             state = AR_IDEAL;
         }
         break;
     case AR_PRESS_UD:
         if(resultsBuffer_JOY[1]>7500 && resultsBuffer_JOY[1] <9000)
         {
             char *str = "UD_ARROW_UP\n";
             while(*str != 0)
             {
               MAP_UART_transmitData(EUSCI_A0_BASE, *str++);
             }
             state = AR_IDEAL;
         }
        break;
     default:
        state = AR_SMStart;
        break;
   }
  return state;
}
