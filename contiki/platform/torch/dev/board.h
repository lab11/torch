/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/** \addtogroup cc2538
 * @{
 *
 * \defgroup cc2538-smartrf SmartRF06EB Peripherals
 *
 * Defines related to the SmartRF06EB
 *
 * This file provides connectivity information on LEDs, Buttons, UART and
 * other SmartRF peripherals
 *
 * Notably, PC0 is used to drive LED1 as well as the USB D+ pullup. Therefore
 * when USB is enabled, LED1 can not be driven by firmware.
 *
 * This file can be used as the basis to configure other platforms using the
 * cc2538 SoC.
 * @{
 *
 * \file
 * Header file with definitions related to the I/O connections on the TI
 * SmartRF06EB
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"
/*---------------------------------------------------------------------------*/
/** \name SmartRF LED configuration
 *
 * LEDs on the SmartRF06 (EB and BB) are connected as follows:
 * - LED1 (Red)    -> PC0
 * - LED2 (Yellow) -> PC1
 * - LED3 (Green)  -> PC2
 * - LED4 (Orange) -> PC3
 *
 * LED1 shares the same pin with the USB pullup
 * @{
 */
/*---------------------------------------------------------------------------*/
/* Some files include leds.h before us, so we need to get rid of defaults in
 * leds.h before we provide correct definitions */
#undef LEDS_GREEN
#undef LEDS_BLUE
#undef LEDS_RED
#undef LEDS_CONF_ALL

#define LEDS_BLUE                4  /**< PC2 */
#define LEDS_RED                 2  /**< PC1 */
#define LEDS_GREEN               1  /**< PC0 */
#define LEDS_CONF_ALL            7

/* Notify various examples that we have LEDs */
#define PLATFORM_HAS_LEDS        1

#define LED_BLUE_BASE            GPIO_C_BASE
#define LED_RED_BASE             GPIO_C_BASE
#define LED_GREEN_BASE           GPIO_C_BASE
#define LED_BLUE_MASK            GPIO_PIN_MASK(2)
#define LED_RED_MASK             GPIO_PIN_MASK(1)
#define LED_GREEN_MASK           GPIO_PIN_MASK(0)
/** @} */
/*---------------------------------------------------------------------------*/
/** \name nRF51822 BLE
 *
 */
#define NRF51822_INT_PORT_NUM  GPIO_B_NUM
#define NRF51822_INT_PIN       4
#define NRF51822_INT_BASE      GPIO_PORT_TO_BASE(NRF51822_INT_PORT_NUM)
#define NRF51822_INT_MASK      GPIO_PIN_MASK(NRF51822_INT_PIN)

#define NRF51822_CS_N_PORT_NUM GPIO_B_NUM
#define NRF51822_CS_N_PIN      3
/** @} */
/*---------------------------------------------------------------------------*/
/** \name RF Switch
 *
 */
#define RF_SWITCH_PORT_NUM      GPIO_B_NUM
#define RF_SWITCH_PIN           5
/** @} */
/*---------------------------------------------------------------------------*/
/** \name LED PWM
 *
 */
#define LED_PWM_PORT_NUM      GPIO_A_NUM
#define LED_PWM_PIN           7
/** @} */
/*---------------------------------------------------------------------------*/
/** \name Flash
 *
 */
#define SST25VF_CS_PORT_NUM      GPIO_C_NUM
#define SST25VF_CS_PIN           3
#define SST25VF_WP_PORT_NUM      GPIO_C_NUM
#define SST25VF_WP_PIN           4
#define SST25VF_HOLD_PORT_NUM    GPIO_C_NUM
#define SST25VF_HOLD_PIN         5
/** @} */
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is driven by PC0 and is shared with LED1
 */
#define USB_PULLUP_PORT          GPIO_C_NUM
#define USB_PULLUP_PIN           0
/** @} */
/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the SmartRF06EB, the UART (XDS back channel) is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 * - CTS: PB0 (Can only be used with UART1)
 * - RTS: PD3 (Can only be used with UART1)
 *
 * We configure the port to use UART0. To use UART1, replace UART0_* with
 * UART1_* below.
 * @{
 */
#define UART0_RX_PORT            GPIO_A_NUM
#define UART0_RX_PIN             0

#define UART0_TX_PORT            GPIO_A_NUM
#define UART0_TX_PIN             1
// unused uart1
//#define UART1_CTS_PORT           GPIO_C_NUM
//#define UART1_CTS_PIN            1

//#define UART1_RTS_PORT           GPIO_C_NUM
//#define UART1_RTS_PIN            2
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name RV-3049-C3 configuration
 *
 * These values configure which CC2538 pins to use for the RTC chip.
 * @{
 */
#define RV3049_INT_N_PORT_NUM    GPIO_C_NUM
#define RV3049_INT_N_PIN         7
#define RV3049_CS_PORT_NUM       GPIO_B_NUM
#define RV3049_CS_PIN            0
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name ADC configuration
 *
 * These values configure which CC2538 pins and ADC channels to use for the ADC
 * inputs.
 *
 * ADC inputs can only be on port A.
 * @{
 */
#define ADC_ALS_PWR_PORT         GPIO_A_NUM /**< ALS power GPIO control port */
#define ADC_ALS_PWR_PIN          7 /**< ALS power GPIO control pin */
#define ADC_ALS_OUT_PIN          6 /**< ALS output ADC input pin on port A */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI configuration
 *
 * These values configure which CC2538 pins to use for the SPI lines.
 * @{
 */
#define SPI_CLK_PORT             GPIO_A_NUM
#define SPI_CLK_PIN              5
#define SPI_MOSI_PORT            GPIO_A_NUM
#define SPI_MOSI_PIN             4
#define SPI_MISO_PORT            GPIO_A_NUM
#define SPI_MISO_PIN             3
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "Torch"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */
