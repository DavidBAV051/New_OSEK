/*
 * Copyright 2016-2026 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file    New_OSEK.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_ctimer.h"

/*==================================================================*/
/* --- Additional Libs --- */
#include "os.h"

/*==================================================================*/
/* --- Definitions --- */
#define LED_ROJO BOARD_LED_RED_GPIO_PIN
#define LED_VERDE BOARD_LED_GREEN_GPIO_PIN
#define LED_AZUL BOARD_LED_BLUE_GPIO_PIN

/* Button Interruption */
#define BOTON_PORT PORT0
#define BOTON_GPIO GPIO0
#define BOTON_PIN  6
#define BOTON_IRQ  GPIO00_IRQn

/* UART Ports*/
#define UART_PORT GPIO1
#define UART1_PIN 17U
#define UART2_PIN 15U
#define UART3_PIN 16U

/* Systick Time Parameter */
#define TIME_PARAM 1000U
#define PRESSCALE 1000000U
/* Delay Time Parameter */
#define DELAY_PARAM 50000U

/* UART var */
volatile u8 rx_byte_uart1 = 0;
volatile u8 rx_byte_uart2 = 0;
volatile u8 rx_byte_uart3 = 0;
volatile u32 bit_ticks = 104;
SWUART_State_t uart1_state;
SWUART_State_t uart2_state;
SWUART_State_t uart3_state;
u8 received_data_1 = ONE;
u8 received_data_2 = ONE;
u8 received_data_3 = ONE;

/*==================================================================*/
/* --- BASE Init --- */
void BOARD_InitHardware(void)
{
    /* attach FRO 12M to FLEXCOMM4 (debug console) */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1u);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER0);

    CLOCK_SetClkDiv(kCLOCK_DivCtimer1Clk, 1u);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER1);

    CLOCK_SetClkDiv(kCLOCK_DivCtimer2Clk, 1u);
	CLOCK_AttachClk(kFRO_HF_to_CTIMER2);

    /* enable clock for GPIO*/
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);
    CLOCK_EnableClock(kCLOCK_Timer0);
    CLOCK_EnableClock(kCLOCK_Timer1);
    CLOCK_EnableClock(kCLOCK_Timer2);

	BOARD_InitBootPins();
	BOARD_InitBootPeripherals();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
#ifndef BOARD_INIT_DEBUG_CONSOLE_PERIPHERAL
    /* Init FSL debug console. */
    BOARD_InitDebugConsole();
#endif
}
void delay(void){
	SDK_DelayAtLeastUs(DELAY_PARAM, SystemCoreClock);
}

/*==================================================================*/
/* Function Prototypes */
void Init_UART1_Timer(void) {
    ctimer_config_t config;
    ctimer_match_config_t matchConfig;

    CTIMER_GetDefaultConfig(&config);
    config.prescale = 47;
    // Lo queremos a 1Mhz, porque 1M/9600 = 104us
    CTIMER_Init(UART1_CTIMER, &config);

    CTIMER_SetupCapture(UART1_CTIMER,
                        UART1_CAPTURE_CHANNEL,
                        kCTIMER_Capture_FallEdge,
                        true); // true = enable capture interrupt

    matchConfig.enableCounterReset = false; // No resetear timer
    matchConfig.enableCounterStop  = false; // No para timer
    matchConfig.matchValue         = 0xFFFFFFFF; // dummy
    matchConfig.outControl         = kCTIMER_Output_NoAction;
    matchConfig.outPinInitState    = false;
    matchConfig.enableInterrupt    = false; // Deshabilitada hasta el star bit

    CTIMER_SetupMatch(UART1_CTIMER, UART1_MATCH_CHANNEL, &matchConfig);

    EnableIRQ(UART1_CTIMER_IRQn);
}

void Init_UART2_Timer(void) {
    ctimer_config_t config;
    ctimer_match_config_t matchConfig;

    CTIMER_GetDefaultConfig(&config);
    config.prescale = 47;
    // Lo queremos a 1Mhz, porque 1M/9600 = 104us
    CTIMER_Init(UART2_CTIMER, &config);

    CTIMER_SetupCapture(UART2_CTIMER,
                        UART2_CAPTURE_CHANNEL,
                        kCTIMER_Capture_FallEdge,
                        true); // true = enable capture interrupt

    matchConfig.enableCounterReset = false; // No resetear timer
    matchConfig.enableCounterStop  = false; // No para timer
    matchConfig.matchValue         = 0xFFFFFFFF; // dummy
    matchConfig.outControl         = kCTIMER_Output_NoAction;
    matchConfig.outPinInitState    = false;
    matchConfig.enableInterrupt    = false; // Deshabilitada hasta el star bit

    CTIMER_SetupMatch(UART2_CTIMER, UART2_MATCH_CHANNEL, &matchConfig);

    EnableIRQ(UART2_CTIMER_IRQn);
}

void Init_UART3_Timer(void) {
    ctimer_config_t config;
    ctimer_match_config_t matchConfig;

    CTIMER_GetDefaultConfig(&config);
    config.prescale = 47;
    // Lo queremos a 1Mhz, porque 1M/9600 = 104us
    CTIMER_Init(UART3_CTIMER, &config);

    CTIMER_SetupCapture(UART3_CTIMER,
                        UART3_CAPTURE_CHANNEL,
                        kCTIMER_Capture_FallEdge,
                        true); // true = enable capture interrupt

    matchConfig.enableCounterReset = false; // No resetear timer
    matchConfig.enableCounterStop  = false; // No para timer
    matchConfig.matchValue         = 0xFFFFFFFF; // dummy
    matchConfig.outControl         = kCTIMER_Output_NoAction;
    matchConfig.outPinInitState    = false;
    matchConfig.enableInterrupt    = false; // Deshabilitada hasta el star bit

    CTIMER_SetupMatch(UART3_CTIMER, UART3_MATCH_CHANNEL, &matchConfig);

    EnableIRQ(UART3_CTIMER_IRQn);
}

void CTIMER0_IRQHandler(void) {
    uint32_t flags = CTIMER_GetStatusFlags(UART1_CTIMER);

    if (flags & kCTIMER_Capture0Flag) {
        CTIMER_ClearStatusFlags(UART1_CTIMER, kCTIMER_Capture0Flag);

        uint32_t capture_val = UART1_CTIMER->CR[UART1_CAPTURE_CHANNEL];

        UART1_CTIMER->MR[UART1_MATCH_CHANNEL] = capture_val + (bit_ticks + (bit_ticks / 2));

        UART1_CTIMER->CCR &= ~(CTIMER_CCR_CAP0I_MASK);
        UART1_CTIMER->MCR |= CTIMER_MCR_MR1I_MASK;

        uart1_state.bit_count = 0;
        uart1_state.rx_buffer = 0;
    }

    if (flags & kCTIMER_Match1Flag) {

        if (uart1_state.bit_count < 8) {
             u8 pin_state = GPIO_PinRead(UART_PORT, UART1_PIN);
             uart1_state.rx_buffer |= (pin_state << uart1_state.bit_count);

            uart1_state.bit_count++;

            UART1_CTIMER->MR[UART1_MATCH_CHANNEL] += bit_ticks;
        } else {
            rx_byte_uart1 = uart1_state.rx_buffer;

            activate_task_ISR(TASK_5_ID);

            UART1_CTIMER->MCR &= ~(CTIMER_MCR_MR1I_MASK);
            UART1_CTIMER->CCR |= CTIMER_CCR_CAP0I_MASK;
        }

        CTIMER_ClearStatusFlags(UART1_CTIMER, kCTIMER_Match1Flag);
    }
}

void CTIMER1_IRQHandler(void) {
    uint32_t flags = CTIMER_GetStatusFlags(UART2_CTIMER);

    if (flags & kCTIMER_Capture0Flag) {
        CTIMER_ClearStatusFlags(UART2_CTIMER, kCTIMER_Capture0Flag);

        uint32_t capture_val = UART2_CTIMER->CR[UART2_CAPTURE_CHANNEL];

        UART2_CTIMER->MR[UART2_MATCH_CHANNEL] = capture_val + (bit_ticks + (bit_ticks / 2));

        UART2_CTIMER->CCR &= ~(CTIMER_CCR_CAP0I_MASK);
        UART2_CTIMER->MCR |= CTIMER_MCR_MR1I_MASK;

        uart2_state.bit_count = 0;
        uart2_state.rx_buffer = 0;
    }

    if (flags & kCTIMER_Match1Flag) {

        if (uart2_state.bit_count < 8) {
             u8 pin_state = GPIO_PinRead(UART_PORT, UART2_PIN);
             uart2_state.rx_buffer |= (pin_state << uart2_state.bit_count);

            uart2_state.bit_count++;

            UART2_CTIMER->MR[UART2_MATCH_CHANNEL] += bit_ticks;
        } else {
            rx_byte_uart2 = uart2_state.rx_buffer;

            activate_task_ISR(TASK_6_ID);

            UART2_CTIMER->MCR &= ~(CTIMER_MCR_MR1I_MASK);
            UART2_CTIMER->CCR |= CTIMER_CCR_CAP0I_MASK;
        }

        CTIMER_ClearStatusFlags(UART2_CTIMER, kCTIMER_Match1Flag);
    }
}

void CTIMER2_IRQHandler(void) {
    uint32_t flags = CTIMER_GetStatusFlags(UART3_CTIMER);

    if (flags & kCTIMER_Capture0Flag) {
        CTIMER_ClearStatusFlags(UART3_CTIMER, kCTIMER_Capture0Flag);

        uint32_t capture_val = UART3_CTIMER->CR[UART3_CAPTURE_CHANNEL];

        UART3_CTIMER->MR[UART2_MATCH_CHANNEL] = capture_val + (bit_ticks + (bit_ticks / 2));

        UART3_CTIMER->CCR &= ~(CTIMER_CCR_CAP0I_MASK);
        UART3_CTIMER->MCR |= CTIMER_MCR_MR1I_MASK;

        uart3_state.bit_count = 0;
        uart3_state.rx_buffer = 0;
    }

    if (flags & kCTIMER_Match1Flag) {

        if (uart3_state.bit_count < 8) {
             u8 pin_state = GPIO_PinRead(UART_PORT, UART3_PIN);
             uart3_state.rx_buffer |= (pin_state << uart3_state.bit_count);

            uart3_state.bit_count++;

            UART3_CTIMER->MR[UART3_MATCH_CHANNEL] += bit_ticks;
        } else {
            rx_byte_uart3 = uart3_state.rx_buffer;

            activate_task_ISR(TASK_7_ID);

            UART3_CTIMER->MCR &= ~(CTIMER_MCR_MR1I_MASK);
            UART3_CTIMER->CCR |= CTIMER_CCR_CAP0I_MASK;
        }

        CTIMER_ClearStatusFlags(UART3_CTIMER, kCTIMER_Match1Flag);
    }
}

/*==================================================================*/
int main(void) {
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };
    BOARD_InitHardware();

	GPIO_PinInit(GPIO0, LED_ROJO, &led_config);
	GPIO_PinInit(GPIO0, LED_VERDE, &led_config);
	GPIO_PinInit(GPIO1, LED_AZUL, &led_config);
	LED_GREEN_OFF();
	LED_RED_OFF();
	LED_BLUE_OFF();
	Init_UART1_Timer();
	Init_UART2_Timer();
	Init_UART3_Timer();
	CTIMER_StartTimer(UART1_CTIMER);
	CTIMER_StartTimer(UART2_CTIMER);
	CTIMER_StartTimer(UART3_CTIMER);

	SysTick_Config(SystemCoreClock / TIME_PARAM);

	task_config();
	os_init();
    while(ONE) {
    }
    return ZERO ;
}

/* UARTS */
void task_UART1(void){
	received_data_1 = rx_byte_uart1;
	terminate_task_ISR();
}

void task_UART2(void){
	received_data_2 = rx_byte_uart2;
	terminate_task_ISR();
}

void task_UART3(void){
	received_data_3 = rx_byte_uart3;
	terminate_task_ISR();
}

/* PWMs */
void task_PWM1(void){
	while(ONE){
		LED_GREEN_ON();
		task_delay(received_data_1);
		LED_GREEN_OFF();
		task_delay(10-received_data_1);
	}
}

void task_PWM2(void){
	while(ONE){
		LED_RED_ON();
		task_delay(received_data_2);
		LED_RED_OFF();
		task_delay(10-received_data_2);
	}
}

void task_PWM3(void){
	while(ONE){
		LED_BLUE_ON();
		task_delay(received_data_3);
		LED_BLUE_OFF();
		task_delay(10-received_data_3);
	}
}
void task_ISR_TMR(void){
//	delay();
//	LED_GREEN_ON();
//	LED_BLUE_ON();
//	LED_RED_ON();
//	delay();
//	LED_BLUE_OFF();
//	LED_GREEN_OFF();
//	LED_RED_OFF();
//	terminate_task_ISR();
}
