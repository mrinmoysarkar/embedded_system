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



#define TOTAL_LCD_BYTES 1024 // total data received at a time from the serial port using DMA
#define PALLET_CHUNK    7    // total chunk of data to get the whole image frame

#pragma DATA_ALIGN(controlTable, TOTAL_LCD_BYTES*2) // align memory to copy the data from serial port to memory

uint8_t controlTable[TOTAL_LCD_BYTES]; // to hold the received data
char RXData_ping[TOTAL_LCD_BYTES]; // to hold received data buffer 1
char RXData_pong[TOTAL_LCD_BYTES]; // to hold received data buffer 2



/* Graphic library context */
Graphics_Context g_sContext;

/* ADC results buffer */

static uint16_t resultsBuffer_JOY[2]; // to hold joystick data

uint8_t image[16384]; // to hold image data
uint32_t pallet[256]; // to hold pallet data


// some variable to manage the received data correctly
int count = 0;
int palletindex = 0;
int imageindex = 0;
bool sendsync = false;

Graphics_Image bitmap; //bitmap image variable used by the graphics library


// baudrate 460800 bps
const eUSCI_UART_ConfigV1 uartConfig =
{
    EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
    1,                                     // BRDIV = 1
    10,                                       // UCxBRF = 10
    0,                                       // UCxBRS = 0
    EUSCI_A_UART_NO_PARITY,                  // No Parity
    EUSCI_A_UART_LSB_FIRST,                  // LSB First
    EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
    EUSCI_A_UART_MODE,                       // UART mode
    EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
};




void init_CLOCK(void); // declaration of function to initialize clock system
void init_UART(void);  // declaration of function to initialize UART system
void init_ADC(void);   // declaration of function to initialize ADC system
void init_GPIO(void);  // declaration of function to initialize GPIO
void init_DMA(void);   // declaration of function to initialize DMA system
void init_LCD(void);   // declaration of function to initialize LCD
void init_SM(void);    // declaration of function to initialize state machine

void process_ReceivedData(bool ping_or_pong); // declaration of function to process the data received using the DMA from UART module
void send_Command(char *cmd); // declaration of function to send command to the host PC

enum AR_States { AR_SMStart, AR_IDEAL, AR_PRESS_LR, AR_PRESS_UD} ar_state; // states of the JoyStick input state machine
enum AR_States TickFct_Arrow(enum AR_States state); // transition function of the JoyStick input state machine

enum BTN_States { BTN_SMStart, BTN_IDEAL, BTN_PRESS_SPACE, BTN_PRESS_X, BTN_PRESS_Z} btn_state; // states of the Button input state machine
enum BTN_States TickFct_Button(enum BTN_States state); // transition function of the Button input state machine


// main function of the firmware
int main(void)
{
    /* Halting WDT and disabling master interrupts */
    MAP_WDT_A_holdTimer();
    MAP_Interrupt_disableMaster();
    init_CLOCK();
    init_GPIO();
    init_LCD();
    init_SM();
    init_UART();
    init_ADC();
    init_DMA();
    MAP_Interrupt_enableMaster();

    while(1)
    {
        MAP_PCM_gotoLPM0(); // enter into low power mode
    }
}


// initialize the clock system of the launchpad
void init_CLOCK(void)
{
    MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

    MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
    MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

    MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_12);
    MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);
}

// initialize the three button used by the game controller
void init_GPIO(void)
{
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P3, GPIO_PIN5);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P5, GPIO_PIN1);
    GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P4, GPIO_PIN1);
}

// initialize the BoosterPack's LCD to be update with the Current score of the game
void init_LCD(void)
{
    bitmap.bPP = 8;
    bitmap.numColors = 256;
    bitmap.pPalette = pallet;
    bitmap.pPixel = image;
    bitmap.xSize = 128;
    bitmap.ySize = 128;

    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

    Graphics_initContext(&g_sContext, &g_sCrystalfontz128x128, &g_sCrystalfontz128x128_funcs);
    Graphics_setForegroundColor(&g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_setBackgroundColor(&g_sContext, GRAPHICS_COLOR_WHITE);
    GrContextFontSet(&g_sContext, &g_sFontFixed6x8);
}

// initialize the state machines initial state
void init_SM(void)
{
    ar_state = AR_SMStart;
    btn_state = BTN_SMStart;
}

// initialize serial port of the launchpad
void init_UART(void)
{
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
    MAP_UART_enableModule(EUSCI_A0_BASE);
}

// initialize the analog to digital communication to get the JoyStick movement
void init_ADC(void)
{

    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);

    MAP_ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);
    MAP_ADC14_configureConversionMemory(ADC_MEM1, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

    MAP_ADC14_enableInterrupt(ADC_INT1);

    MAP_Interrupt_enableInterrupt(INT_ADC14);
    MAP_Interrupt_enableSleepOnIsrExit();

    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    MAP_ADC14_enableConversion();
    MAP_ADC14_toggleConversionTrigger();
}

// initialize the DMA for ping-pong mode
void init_DMA(void)
{
    /* Configuring DMA module */
    MAP_DMA_enableModule();
    MAP_DMA_setControlBase(controlTable);

    MAP_DMA_disableChannelAttribute(DMA_CH1_EUSCIA0RX, UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST | UDMA_ATTR_HIGH_PRIORITY | UDMA_ATTR_REQMASK);

    MAP_DMA_setChannelControl(UDMA_PRI_SELECT | DMA_CH1_EUSCIA0RX, UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);
    MAP_DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH1_EUSCIA0RX, UDMA_MODE_PINGPONG, (void*)MAP_UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE), RXData_ping, TOTAL_LCD_BYTES);

    MAP_DMA_setChannelControl(UDMA_ALT_SELECT | DMA_CH1_EUSCIA0RX, UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);
    MAP_DMA_setChannelTransfer(UDMA_ALT_SELECT | DMA_CH1_EUSCIA0RX, UDMA_MODE_PINGPONG, (void*)MAP_UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE), RXData_pong, TOTAL_LCD_BYTES);

    MAP_DMA_assignChannel(DMA_CH1_EUSCIA0RX);
    MAP_DMA_enableChannel(DMA_CHANNEL_1);
    MAP_DMA_assignInterrupt(DMA_INT1, DMA_CHANNEL_1);
    MAP_DMA_enableInterrupt(INT_DMA_INT1);

    MAP_Interrupt_enableInterrupt(INT_DMA_INT1);
    MAP_Interrupt_enableSleepOnIsrExit();
}

// process the data received from serial port using DMA in DMA interrupt sub-routine
void DMA_INT1_IRQHandler(void)
{
    sendsync = true;
    if(MAP_DMA_getChannelAttribute(DMA_CHANNEL_1) & UDMA_ATTR_ALTSELECT)
    {
        MAP_DMA_setChannelControl(UDMA_PRI_SELECT | DMA_CH1_EUSCIA0RX, UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH1_EUSCIA0RX, UDMA_MODE_PINGPONG, (void*)MAP_UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE), RXData_ping, TOTAL_LCD_BYTES);
        process_ReceivedData(true);
    }
    else
    {
        MAP_DMA_setChannelControl(UDMA_ALT_SELECT | DMA_CH1_EUSCIA0RX, UDMA_SIZE_8 | UDMA_SRC_INC_NONE | UDMA_DST_INC_8 | UDMA_ARB_1);
        MAP_DMA_setChannelTransfer(UDMA_ALT_SELECT | DMA_CH1_EUSCIA0RX, UDMA_MODE_PINGPONG, (void*)MAP_UART_getReceiveBufferAddressForDMA(EUSCI_A0_BASE), RXData_pong, TOTAL_LCD_BYTES);
        process_ReceivedData(false);
    }

    printf("data comming  %d\n", count);
 }

// Process the ADC data and execute the state machines' tick function
void ADC14_IRQHandler(void)
{
    uint64_t status;
    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);
    if(status & ADC_INT1)
    {
        resultsBuffer_JOY[0] = ADC14_getResult(ADC_MEM0);
        resultsBuffer_JOY[1] = ADC14_getResult(ADC_MEM1);
        ar_state = TickFct_Arrow(ar_state);
        btn_state = TickFct_Button(btn_state);
    }
    if(!sendsync)
    {
        send_Command("SEND_IMAGE\n");
    }
}

// copy the image data to a global variable and update the LCD
void process_ReceivedData(bool ping_or_pong)
{
    int i;
    count++;
    if(count==PALLET_CHUNK)
    {
        for(i=0;i<TOTAL_LCD_BYTES && palletindex<256;)
        {
            if(ping_or_pong)
            {
                pallet[palletindex++] = ((uint32_t) RXData_ping[i]) << 24 | ((uint32_t) RXData_ping[i+1]) << 16 | ((uint32_t) RXData_ping[i+2]) << 8 | ((uint32_t) RXData_ping[i+3]);
            }
            else
            {
                pallet[palletindex++] = ((uint32_t) RXData_pong[i]) << 24 | ((uint32_t) RXData_pong[i+1]) << 16 | ((uint32_t) RXData_pong[i+2]) << 8 | ((uint32_t) RXData_pong[i+3]);
            }
            i += 4;
        }
        palletindex = 0;
        imageindex = 0;
        count = 0;
        Graphics_drawImage(&g_sContext, &bitmap, 0, 0);
    }
    else
    {
        for(i=0;i<TOTAL_LCD_BYTES;i++)
        {
            if(ping_or_pong)
            {
                image[imageindex++] = RXData_ping[i];
            }
            else
            {
                image[imageindex++] = RXData_pong[i];
            }
        }
    }
}

// send a command string to the host PC using serial port
void send_Command(char *cmd)
{
    while(*cmd != 0)
    {
        MAP_UART_transmitData(EUSCI_A0_BASE, *cmd++);
    }
}

// button state tick function
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
                send_Command("Z\n");
                state = BTN_PRESS_Z;
            }
            else if(GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN1) == GPIO_INPUT_PIN_LOW)
            {
                send_Command("SPACE_BAR\n");
                state = BTN_PRESS_SPACE;
            }
            else if(GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN1) == GPIO_INPUT_PIN_LOW)
            {
                send_Command("X\n");
                state = BTN_PRESS_X;
            }
            break;
        case BTN_PRESS_Z:
            if(GPIO_getInputPinValue(GPIO_PORT_P3, GPIO_PIN5) != GPIO_INPUT_PIN_LOW)
            {
                send_Command("Z_UP\n");
                state = BTN_IDEAL;
            }
            break;
        case BTN_PRESS_SPACE:
            if(GPIO_getInputPinValue(GPIO_PORT_P5, GPIO_PIN1) != GPIO_INPUT_PIN_LOW)
            {
                send_Command("SPACE_BAR_UP\n");
                state = BTN_IDEAL;
            }
            break;
        case BTN_PRESS_X:
            if(GPIO_getInputPinValue(GPIO_PORT_P4, GPIO_PIN1) != GPIO_INPUT_PIN_LOW)
            {
                send_Command("X_UP\n");
                state = BTN_IDEAL;
            }
            break;
        default:
            state = BTN_SMStart;
            break;
    }
    return state;
}

// Joystick state tick function
enum AR_States TickFct_Arrow(enum AR_States state) {
  switch(state)
  {
     case AR_SMStart:
        state = AR_IDEAL;
        break;
     case AR_IDEAL:
         if(resultsBuffer_JOY[0] > 15000)
         {
            send_Command("RIGHT_ARROW\n");
            state = AR_PRESS_LR;
         }
         else if(resultsBuffer_JOY[0] < 50)
         {
           send_Command("LEFT_ARROW\n");
           state = AR_PRESS_LR;
         }
         else if(resultsBuffer_JOY[1] > 15000)
         {
           send_Command("UP_ARROW\n");
           state = AR_PRESS_UD;
         }
         else if(resultsBuffer_JOY[1] < 50)
         {
           send_Command("DOWN_ARROW\n");
           state = AR_PRESS_UD;
         }
        break;
     case AR_PRESS_LR:
         if(resultsBuffer_JOY[0]>6500 && resultsBuffer_JOY[0]<8000)
         {
             send_Command("LR_ARROW_UP\n");
             state = AR_IDEAL;
         }
         break;
     case AR_PRESS_UD:
         if(resultsBuffer_JOY[1]>7500 && resultsBuffer_JOY[1]<9000)
         {
             send_Command("UD_ARROW_UP\n");
             state = AR_IDEAL;
         }
        break;
     default:
        state = AR_SMStart;
        break;
   }
  return state;
}

/**************************** END Firmware Code ****************************/
