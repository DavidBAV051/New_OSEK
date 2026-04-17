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

/*==================================================================*/
/* --- Additional Libs --- */
#include "os.h"

/*==================================================================*/
/* --- Definitions --- */
/* LED Definitions */
#define LED_ROJO BOARD_LED_RED_GPIO_PIN
#define LED_VERDE BOARD_LED_GREEN_GPIO_PIN
#define LED_AZUL BOARD_LED_BLUE_GPIO_PIN

/* Button Interruption */
#define BOTON_PORT PORT0
#define BOTON_GPIO GPIO0
#define BOTON_PIN  6
#define BOTON_IRQ  GPIO00_IRQn

/* Systick Time Parameter */
#define TIME_PARAM 1000U

/* Delay Time Parameter */
#define DELAY_PARAM 50000U

/*==================================================================*/
/* --- BASE Init --- */
void BOARD_InitHardware(void)
{
    /* attach FRO 12M to FLEXCOMM4 (debug console) */
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
    CLOCK_AttachClk(BOARD_DEBUG_UART_CLK_ATTACH);

    /* enable clock for GPIO*/
    CLOCK_EnableClock(kCLOCK_Gpio0);
    CLOCK_EnableClock(kCLOCK_Gpio1);

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
void Init_Boton_Interrupt(void) {
    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
    };

    GPIO_SetPinInterruptConfig(BOTON_GPIO, BOTON_PIN, kGPIO_InterruptFallingEdge);
    EnableIRQ(BOTON_IRQ);

    GPIO_PinInit(BOTON_GPIO, BOTON_PIN, &sw_config);
}
void GPIO00_IRQHandler(void) {
    activate_task_ISR(TASK_ISR_BTN_ID);

	GPIO_GpioClearInterruptFlags(BOTON_GPIO, 1U << BOTON_PIN);

}

/*==================================================================*/
int main(void) {
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };
    BOARD_InitHardware();
    Init_Boton_Interrupt();

	GPIO_PinInit(GPIO0, LED_ROJO, &led_config);
	GPIO_PinInit(GPIO0, LED_VERDE, &led_config);
	GPIO_PinInit(GPIO1, LED_AZUL, &led_config);
	LED_GREEN_OFF();
	LED_RED_OFF();
	LED_BLUE_OFF();

	SysTick_Config(SystemCoreClock / TIME_PARAM);

	task_config();
	os_init();
    while(ONE) {
    }
    return ZERO ;
}

void task_LEDON(void){
	LED_RED_ON();
}

void task_LEDOFF(void){
	LED_RED_OFF();
}

void task_ISR_BTN(void){
	delay();
	LED_GREEN_ON();
	LED_BLUE_ON();
	LED_RED_ON();
	delay();
	LED_BLUE_OFF();
	LED_GREEN_OFF();
	LED_RED_OFF();
	terminate_task_ISR();
}

