// Copyright 2018 Inca Roads LLC.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.

#include <driverlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

// MSP430FR2355 UCB0SDA and UCB0SCL
// http://www.ti.com/lit/ug/slau680/slau680.pdf
#define I2C_PORT            GPIO_PORT_P1
#define I2C_PIN_SDA         GPIO_PIN2
#define I2C_PIN_SCL         GPIO_PIN3

// Clock frequencies initialized by INIT_CS()
#define DCOCLK_FREQUENCY    24000000
#define MCLK_FREQUENCY      DCOCLK_FREQUENCY
#define SMCLK_FREQUENCY     DCOCLK_FREQUENCY
#define ACLK_FREQUENCY      32768
#define I2C_FREQUENCY       EUSCI_B_I2C_SET_DATA_RATE_100KBPS

// Data for Notecard I/O functions
#if !NOTECARD_USE_I2C
static size_t serialOverruns = 0;
static volatile size_t serialFillIndex = 0;
static volatile size_t serialDrainIndex = 0;
static char serialBuffer[512];
#endif

// I2C parameters
#if NOTECARD_USE_I2C
static EUSCI_B_I2C_initMasterParam i2cConfig = {
    EUSCI_B_I2C_CLOCKSOURCE_SMCLK,
    SMCLK_FREQUENCY,
    I2C_FREQUENCY,
    0,
    EUSCI_B_I2C_SEND_STOP_AUTOMATICALLY_ON_BYTECOUNT_THRESHOLD
};
static char *i2cBufferNext;
static uint32_t i2cBufferLeft = 0;
#endif

// Clock timer
static long unsigned int ticksMs = 0;

// Forwards
void init_GPIO(void);
void init_CS(void);

// Main entry point
int main(void) {

    // Initialize MSP430 peripherals as needed
    init_CS();
    init_GPIO();

    // Simulate an Arduino-like setup/loop interface
    setup();
    while (true)
        loop();

}

// Initialize all GPIO to output low for minimal LPM power consumption
void init_GPIO(void) {
    GPIO_setAsOutputPin(GPIO_PORT_PA, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PB, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PC, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PD, GPIO_PIN_ALL16);
    GPIO_setAsOutputPin(GPIO_PORT_PE, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PA, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PB, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PC, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PD, GPIO_PIN_ALL16);
    GPIO_setOutputLowOnPin(GPIO_PORT_PE, GPIO_PIN_ALL16);
    PMM_unlockLPM5();

}

// Initialize clocks/timers
void init_CS(void) {

    // Stop watchdog timer
    WDT_A_hold(WDT_A_BASE);

    // Configure two FRAM waitstate as required by the device datasheet for MCLK
    // operation at 24MHz _before_ configuring the clock system.
    FRCTL0 = FRCTLPW | NWAITS_2 ;

    P2SEL1 |= BIT6 | BIT7;                          // P2.6~P2.7: crystal pins
    do {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);              // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    } while (SFRIFG1 & OFIFG);                      // Test oscillator fault flag

    __bis_SR_register(SCG0);                        // disable FLL
    CSCTL3 |= SELREF__XT1CLK;                       // Set XT1 as FLL reference source
    CSCTL0 = 0;                                     // clear DCO and MOD registers
    CSCTL1 = DCORSEL_7;                             // Set DCO = 24MHz
    CSCTL2 = FLLD_0 + 731;                          // DCOCLKDIV = 24MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                        // enable FLL
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));      // FLL locked

    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;       // set XT1 (~32768Hz) as ACLK source
                                                    // default DCOCLKDIV as MCLK and SMCLK source

    P3DIR |= BIT4;
    P3SEL0 |= BIT4;
    P3SEL1 &= ~BIT4;

    // Set up watchdog to count at ACLK (32768Hz) and to overflow the up counter at 32, or at a ~1mS interval
    Timer_B_initUpModeParam tpB = {0};
    tpB.clockSource                 = TIMER_B_CLOCKSOURCE_ACLK;
    tpB.clockSourceDivider          = TIMER_B_CLOCKSOURCE_DIVIDER_1;
    tpB.timerInterruptEnable_TBIE   = TIMER_B_TBIE_INTERRUPT_ENABLE;
    tpB.startTimer                  = true;
    tpB.timerPeriod                 = 32;
    Timer_B_initUpMode(TIMER_B0_BASE, &tpB);
    Timer_B_startCounter(TIMER_B0_BASE, TIMER_B_UP_MODE);
    __bis_SR_register(GIE);

}

// Serial port reset procedure, called before any I/O and called again upon I/O error
#if !NOTECARD_USE_I2C
void noteSerialReset() {

    // Configure UCA1TXD and UCA1RXD
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P4, GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    // Configure UART 9600/N/8/1
    // http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 156;
    param.firstModReg = 4;
    param.secondModReg = 0;
    param.overSampling = 1;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    EUSCI_A_UART_init(EUSCI_A1_BASE, &param);

    // Enable USCI_A0 RX interrupt
    EUSCI_A_UART_enable(EUSCI_A1_BASE);
    EUSCI_A_UART_clearInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    EUSCI_A_UART_enableInterrupt(EUSCI_A1_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    __bis_SR_register(GIE);

    // Reset our buffer management
    serialFillIndex = serialDrainIndex = serialOverruns = 0;

    // Unused, but included for documentation
    ((void)(serialOverruns));

}
#endif

// Serial write data function
#if !NOTECARD_USE_I2C
void noteSerialTransmit(uint8_t *text, size_t len, bool flush) {
    while (len > 0) {
        while (EUSCI_A_UART_queryStatusFlags(EUSCI_A1_BASE, EUSCI_A_UART_BUSY));
        EUSCI_A_UART_transmitData(EUSCI_A1_BASE, *text++);
        len--;
    }
}
#endif

// Serial "is anything available" function, which does a read-ahead for data into a serial buffer
#if !NOTECARD_USE_I2C
bool noteSerialAvailable() {
    return (serialFillIndex != serialDrainIndex);
}
#endif

// Blocking serial read a byte function (generally only called if known to be available)
#if !NOTECARD_USE_I2C
char noteSerialReceive() {
    char data;
    while (!noteSerialAvailable()) ;
    if (serialDrainIndex < sizeof(serialBuffer))
        data = serialBuffer[serialDrainIndex++];
    else {
        data = serialBuffer[0];
        serialDrainIndex = 1;
    }
    return data;
}
#endif

// I2C reset procedure, called before any I/O and called again upon I/O error
#if NOTECARD_USE_I2C
void noteI2CReset(uint16_t DevAddress) {
    i2cConfig.i2cClk = CS_getSMCLK();
    GPIO_setAsPeripheralModuleFunctionInputPin( I2C_PORT, I2C_PIN_SCL | I2C_PIN_SDA,
                                                GPIO_PRIMARY_MODULE_FUNCTION );
    EUSCI_B_I2C_initMaster(EUSCI_B0_BASE, &i2cConfig);
    __bis_SR_register(GIE);
}
#endif

// Transmits in master mode an amount of data, in blocking mode.     The address
// is the actual address; the caller should have shifted it right so that the
// low bit is NOT the read/write bit. An error message is returned, else NULL if success.
#if NOTECARD_USE_I2C
const char *noteI2CTransmit(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size) {

    // Special-case Size == 0 as being equivalent to 1 because of the I2C Receive header
    size_t bufferLength = sizeof(uint8_t) + (Size == 0 ? 1 : Size);

    // Allocate the transmit buffer
    char *i2cBuffer = malloc(bufferLength);
    if (i2cBuffer == NULL)
        return "i2c: insufficient memory (write)";

    // Copy the buffer and set up for transmit
    i2cBuffer[0] = (uint8_t) Size;
    memcpy(&i2cBuffer[1], pBuffer, bufferLength-1);
    i2cBufferNext = i2cBuffer;
    i2cBufferLeft = bufferLength;

    // Set up for the transmit
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, DevAddress);
    EUSCI_B_I2C_setMode(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_MODE);
    EUSCI_B_I2C_enable(EUSCI_B0_BASE);
    EUSCI_B_I2C_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                               + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                               + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                               + EUSCI_B_I2C_NAK_INTERRUPT);
    EUSCI_B_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0);
    __bis_SR_register(GIE);

    // Initiate the multi-byte transmit, knowing that we ALWAYS send at least 2 bytes
    EUSCI_B_I2C_masterSendMultiByteStart(EUSCI_B0_BASE, *i2cBufferNext++);
    i2cBufferLeft--;

    // Wait until it has completed
    while (i2cBufferLeft > 0)  __bis_SR_register(LPM0_bits + GIE);
    while (EUSCI_B_I2C_masterIsStopSent(EUSCI_B0_BASE) != EUSCI_B_I2C_STOP_SEND_COMPLETE) ;

    // Free the transmit buffer
    free(i2cBuffer);

    return NULL;
}
#endif

// Receives in master mode an amount of data in blocking mode. An error mesage returned, else NULL if success.
#if NOTECARD_USE_I2C
const char *noteI2CReceive(uint16_t DevAddress, uint8_t* pBuffer, uint16_t Size, uint32_t *available) {

    // Transmit a signal that "about to do a receive of N bytes" to the slave, with a special-case Size == 0
    uint8_t hdr = (uint8_t) Size;
    const char *errstr = noteI2CTransmit(DevAddress, &hdr, 0);
    if (errstr != NULL)
        return errstr;

    // Allocate a return buffer
    i2cBufferLeft = Size + (sizeof(uint8_t)*2);
    char *i2cBuffer = malloc(i2cBufferLeft);
    if (i2cBuffer == NULL)
        return "i2c: insufficient memory (read)";
    i2cBufferNext = i2cBuffer;

    // Receive the reply
    EUSCI_B_I2C_setSlaveAddress(EUSCI_B0_BASE, DevAddress);
    EUSCI_B_I2C_setMode(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_MODE);
    EUSCI_B_I2C_enable(EUSCI_B0_BASE);
    EUSCI_B_I2C_clearInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_TRANSMIT_INTERRUPT0
                               + EUSCI_B_I2C_RECEIVE_INTERRUPT0
                               + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT
                               + EUSCI_B_I2C_NAK_INTERRUPT);
    EUSCI_B_I2C_enableInterrupt(EUSCI_B0_BASE, EUSCI_B_I2C_RECEIVE_INTERRUPT0 + EUSCI_B_I2C_BYTE_COUNTER_INTERRUPT);
    __bis_SR_register(GIE);

    EUSCI_B_I2C_masterReceiveStart(EUSCI_B0_BASE);

    // Wait until receive is completed
    while (i2cBufferLeft > 0) __bis_SR_register(LPM0_bits + GIE);

    // Interpret the received buffer
    uint8_t availbyte = i2cBuffer[0];
    uint8_t goodbyte = i2cBuffer[1];
    if (goodbyte != Size) {
        errstr = "i2c: incorrect amount of data";
    } else {
        *available = availbyte;
        memcpy(pBuffer, &i2cBuffer[2], Size);
    }
    free(i2cBuffer);

    // Done
    return errstr;

}
#endif

// Get the number of app milliseconds since boot (this will wrap)
long unsigned int millis() {
    return (long unsigned int) ticksMs;
}

// Delay the specified number of milliseconds
void delay(uint32_t ms) {
    long unsigned int expires = millis() + ms;
    while (millis() < expires) ;
}

// EUSCI Interrupt Service Routine
#if !NOTECARD_USE_I2C
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void)
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(USCI_A1_VECTOR))) USCI_A1_ISR (void)
#else
#error compiler not supported
#endif
{

    switch (__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG)) {

    case USCI_UART_UCRXIFG: {
        char data = UCA1RXBUF;
        if (serialFillIndex < sizeof(serialBuffer)) {
            if (serialFillIndex+1 == serialDrainIndex)
                serialOverruns++;
            else
                serialBuffer[serialFillIndex++] = data;
        } else {
            if (serialDrainIndex == 1)
                serialOverruns++;
            else {
                serialBuffer[0] = data;
                serialFillIndex = 1;
            }
        }
        break;
    }

    case USCI_NONE:
    case USCI_UART_UCTXIFG:
    case USCI_UART_UCSTTIFG:
    case USCI_UART_UCTXCPTIFG:
    default:
        break;
    }

}
#endif

// USCI B0 interrupt service routine
#if NOTECARD_USE_I2C
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(USCI_B0_VECTOR))) USCI_B0_ISR (void)
#else
#error compiler not supported
#endif
{

    switch (__even_in_range(UCB0IV, 0x1E)){

        // Receive next byte
    case 0x16: {            // Vector 24: RXIFG0
        if (i2cBufferLeft > 0) {
            *i2cBufferNext++ = EUSCI_B_I2C_masterReceiveMultiByteNext(EUSCI_B0_BASE);
            // Just before receiving last byte, generate the stop condition
            if (--i2cBufferLeft == 1)
                EUSCI_B_I2C_masterReceiveMultiByteStop(EUSCI_B0_BASE);
        }
        if (i2cBufferLeft == 0)
            __bic_SR_register_on_exit(LPM0_bits);
        break;
    }

        // Transmit next byte
    case 0x18: {            // Vector 26: TXIFG0
        if (i2cBufferLeft > 0) {
            if (--i2cBufferLeft > 0)
                EUSCI_B_I2C_masterSendMultiByteNext(EUSCI_B0_BASE, *i2cBufferNext++);
            else
                EUSCI_B_I2C_masterSendMultiByteFinish(EUSCI_B0_BASE, *i2cBufferNext++);
        }
        if (i2cBufferLeft == 0)
            __bic_SR_register_on_exit(LPM0_bits);
        break;
    }

    case 0x00:      // Vector 0: No interrupts
        break;
    case 0x02:      // Vector 2: No interrupts
        break;
    case 0x04:      // Vector 4: NACKIFG
        break;
    case 0x06:      // Vector 6: STT IFG
        break;
    case 0x08:      // Vector 8: STPIFG
        break;
    case 0x0a:      // Vector 10: RXIFG3
        break;
    case 0x0c:      // Vector 14: TXIFG3
        break;
    case 0x0e:      // Vector 16: RXIFG2
        break;
    case 0x10:      // Vector 18: TXIFG2
        break;
    case 0x12:      // Vector 20: RXIFG1
        break;
    case 0x14:      // Vector 22: TXIFG1
        break;
    case 0x1a:      // Vector 28: BCNTIFG
        break;
    case 0x1c:      // Vector 30: clock low timeout
        break;
    case 0x1e:      // Vector 32: 9th bit
        break;
    default:
        break;
    }
}
#endif

// Timer B interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void)
#elif defined(__GNUC__)
    void __attribute__ ((interrupt(TIMER0_B1_VECTOR))) TIMER0_B1_ISR (void)
#else
#error compiler not supported
#endif
{
    ticksMs++;
    Timer_B_clearTimerInterrupt(TIMER_B0_BASE);
}

