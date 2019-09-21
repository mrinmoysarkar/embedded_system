
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
char dataReceived[1000];
int index = 0;
int palletindex = 0;
int imageindex = 0;
int imageOrpallet=0;//0 means image 1 means pallet
uint32_t palletData = 0;
Graphics_Image bitmap;
int delay = 0;
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

void drawTitle(void);
void drawAccelData(void);
void num2str(uint16_t x, char* str);

/*
 * Main function
 */
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



    /* Selecting P1.2 and P1.3 in UART mode */
      MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
              GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

      /* Setting DCO to 12MHz */
      CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);

      //![Simple UART Example]
      /* Configuring UART Module */
      MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

      /* Enable UART module */
      MAP_UART_enableModule(EUSCI_A0_BASE);

      /* Enabling interrupts */
      MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
      MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
      MAP_Interrupt_enableSleepOnIsrExit();
      MAP_Interrupt_enableMaster();
      //![Simple UART Example]

    /* Initializes display */
    Crystalfontz128x128_Init();

    /* Set default screen orientation */
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    /* Initializes graphics context */
    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
    drawTitle();

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



/* EUSCI A0 UART ISR - Echoes data back to PC host */
void EUSCIA0_IRQHandler(void)
{
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

    MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);
//    char *str = "Hello mrinmoy\n";
    uint8_t data;
    if(status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        data = MAP_UART_receiveData(EUSCI_A0_BASE);

        if(imageOrpallet == 0)
        {
            image[imageindex++] = data;
            if(imageindex == 16384)
            {
                imageOrpallet = 1;
                imageindex = 0;
            }
        }
        else if(imageOrpallet == 1)
        {
            if (index == 0) {palletData |= ((uint32_t) data) << 16; index++;}
            else if (index == 1) {palletData |= ((uint32_t) data) << 8; index++;}
            else if (index == 2) {palletData |= ((uint32_t) data); index=0; pallet[palletindex]=palletData; palletData=0; palletindex++;}
            if(palletindex==256)
            {
//                Graphics_clearDisplay(&g_sContext);
                Graphics_drawImage(&g_sContext, &bitmap, 0, 0);
                palletindex = 0;
                imageOrpallet = 0;
            }
        }
//        dataReceived[index++] = data;
//        index++;
//        done = 1;
//        MAP_UART_transmitData(EUSCI_A0_BASE,data);
//        printf("%d\t",data);
//        if(dataReceived[index-1]==13)//13 is equal to enter button on key board
//        {
//            dataReceived[index] = '\0';
////            printf("%s", dataReceived);
//            index = 0;
//            Graphics_clearDisplay(&g_sContext);
//            Graphics_drawImage(&g_sContext, &bitmap, 0, 0);
////            Graphics_drawStringCentered(&g_sContext,
////                                            (int8_t *)dataReceived,
////                                            AUTO_STRING_LENGTH,
////                                            64,
////                                            30,
////                                            OPAQUE_TEXT);
//        }
        //MAP_UART_transmitData(EUSCI_A0_BASE, MAP_UART_receiveData(EUSCI_A0_BASE));
//        while(*str != 0)
//        {
//            MAP_UART_transmitData(EUSCI_A0_BASE,*str++);
//        }
    }

}


/*
 * Clear display and redraw title + accelerometer data
 */
void drawTitle()
{
//    Graphics_clearDisplay(&g_sContext);
//    Graphics_drawStringCentered(&g_sContext,
//                                    (int8_t *)"Accelerometer:",
//                                    AUTO_STRING_LENGTH,
//                                    64,
//                                    30,
//                                    OPAQUE_TEXT);
//    drawAccelData();
}


/*
 * Redraw accelerometer data
 */
void drawAccelData()
{
//    char string[10];
//    sprintf(string, "X: %5d", resultsBuffer_ACC[0]);
//    Graphics_drawStringCentered(&g_sContext,
//                                    (int8_t *)string,
//                                    8,
//                                    64,
//                                    50,
//                                    OPAQUE_TEXT);
//
//    sprintf(string, "Y: %5d", resultsBuffer_ACC[1]);
//    Graphics_drawStringCentered(&g_sContext,
//                                    (int8_t *)string,
//                                    8,
//                                    64,
//                                    70,
//                                    OPAQUE_TEXT);
//
//    sprintf(string, "Z: %5d", resultsBuffer_ACC[2]);
//    Graphics_drawStringCentered(&g_sContext,
//                                    (int8_t *)string,
//                                    8,
//                                    64,
//                                    90,
//                                    OPAQUE_TEXT);

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

        /*
         * Draw accelerometer data on display and determine if orientation
         * change thresholds are reached and redraw as necessary
         */
//        if (resultsBuffer_ACC[0] < 5600) {
//            if (Lcd_Orientation != LCD_ORIENTATION_LEFT) {
//                Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_LEFT);
//                drawTitle();
//            }
//            else
//                drawAccelData();
//        }
//        else if (resultsBuffer_ACC[0] > 10400) {
//            if (Lcd_Orientation != LCD_ORIENTATION_RIGHT){
//                Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_RIGHT);
//                drawTitle();
//            }
//            else
//                drawAccelData();
//        }
//        else if (resultsBuffer_ACC[1] < 5600) {
//            if (Lcd_Orientation != LCD_ORIENTATION_UP){
//                Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
//                drawTitle();
//            }
//            else
//                drawAccelData();
//        }
//        else if (resultsBuffer_ACC[1] > 10400) {
//            if (Lcd_Orientation != LCD_ORIENTATION_DOWN){
//                Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_DOWN);
//                drawTitle();
//            }
//            else
//                drawAccelData();
//        }
//        else
//            drawAccelData();
    }

    if(status & ADC_INT1)
       {
           /* Store ADC14 conversion results */
        resultsBuffer_JOY[0] = ADC14_getResult(ADC_MEM3);
        resultsBuffer_JOY[1] = ADC14_getResult(ADC_MEM4);

           /* Determine if JoyStick button is pressed */
           int buttonPressed = 0;
           if (!(P4IN & GPIO_PIN1))
           {
               buttonPressed = 1;
           }
           if(buttonPressed)
           {
               char *str = "JOY_BUTTON_CLICK\n";
               while(*str != 0)
               {
//                   printf("%c",*str);
                   MAP_UART_transmitData(EUSCI_A0_BASE, *str++);

               }
//               printf("sending data\n");
               while (!(P4IN & GPIO_PIN1));
           }
           if (((int)resultsBuffer_JOY[0]-7251) > 500 || ((int)resultsBuffer_JOY[0]-7251) < -500 || ((int)resultsBuffer_JOY[1]-8295) > 500 ||((int)resultsBuffer_JOY[1]-8295) < -500)
           {
               char strx[] = "XXXXXXXXXXXXXXXX\n";

               num2str(resultsBuffer_JOY[0], strx);
               int i,j;
               for(i=0;i<17;i++)
               {
                  MAP_UART_transmitData(EUSCI_A0_BASE, strx[i]);
               }
               char stry[] = "YYYYYYYYYYYYYYYY\n";

               num2str(resultsBuffer_JOY[1], stry);
               for( i = 0;i<17;i++)
               {
                   MAP_UART_transmitData(EUSCI_A0_BASE, stry[i]);
               }
//               delay = 0;
//
               for(i=0;i<100;i++)
               {
                   for(j=0;j<1000;j++);
               }
           }
//           delay++;
       }
}

void num2str(uint16_t x, char *str)
{
    int i;
    for(i=15;(x != 0 && i >=0);i--)
    {
//        str[i] = '1'?(x & 0x8000)!=0:'0';
//        x = x << 1;
        str[i] = (char)('0'+ x%10);
        x=x/10;
    }
}
