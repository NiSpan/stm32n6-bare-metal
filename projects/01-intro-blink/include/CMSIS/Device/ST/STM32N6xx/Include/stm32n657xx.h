/*
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modified: trimmed from STM32CubeN6 stm32n657xx.h to GPIO and RCC AHB4ENR; GPIOG/RCC fixed to the secure aliases.
 */

/*
 * STM32N657 device header. ST's version also defines the interrupt numbers
 * (IRQn_Type), the core features (__FPU_PRESENT, ...) and every peripheral
 * register. Only what main.c uses is kept: GPIO and the RCC clock enable.
 */
#ifndef STM32N657xx_H
#define STM32N657xx_H

#include "core_cm55.h"

/*
 * General Purpose I/O (stm32n657xx.h:1435), cut after BSRR.
 *
 * The struct is laid over the registers in memory, nothing is stored in it.
 * Each field is 4 bytes, so its offset in the struct is the register offset
 * from the reference manual: GPIOG->BSRR = 0x56021800 + 0x18. The field order
 * must match the manual exactly, one missing field shifts all that follow.
 * __IO (volatile): every access really goes to the hardware.
 */
typedef struct
{
  __IO uint32_t MODER;         /*!< GPIO port mode register,                     Address offset: 0x00      */
  __IO uint32_t OTYPER;        /*!< GPIO port output type register,              Address offset: 0x04      */
  __IO uint32_t OSPEEDR;       /*!< GPIO port output speed register,             Address offset: 0x08      */
  __IO uint32_t PUPDR;         /*!< GPIO port pull-up/pull-down register,        Address offset: 0x0C      */
  __IO uint32_t IDR;           /*!< GPIO port input data register,               Address offset: 0x10      */
  __IO uint32_t ODR;           /*!< GPIO port output data register,              Address offset: 0x14      */
  __IO uint32_t BSRR;          /*!< GPIO port bit set/reset  register,           Address offset: 0x18      */
} GPIO_TypeDef;

/*
 * Reset and Clock Control (stm32n657xx.h:2022), only up to AHB4ENR.
 *
 * ST names every register. Here the ones before AHB4ENR (CR, SR, CFGR1, ...,
 * AHB3ENR) are one unnamed block: 0x25C / 4 = 151 words. They are still real
 * registers, only without names, so never write to RESERVED. The block only
 * pushes AHB4ENR to offset 0x25C. If the count is off by one, AHB4ENR
 * silently points at the wrong register. To use another RCC register, shrink
 * the block and add the field, or copy more of ST's struct.
 */
typedef struct
{
       uint32_t RESERVED[151];    /*!< CR ... AHB3ENR                                                 Address offset: 0x0000-0x0258 */
  __IO uint32_t AHB4ENR;          /*!< RCC AHB4 enable register                                       Address offset: 0x025C */
} RCC_TypeDef;

/* Base addresses, secure alias (stm32n657xx.h:3340, 3350, 3517, 3526) */
#define PERIPH_BASE_S                   0x50000000UL
#define AHB4PERIPH_BASE_S               (PERIPH_BASE_S + 0x06020000UL)
#define GPIOG_BASE_S                    (AHB4PERIPH_BASE_S + 0x1800UL)
#define RCC_BASE_S                      (AHB4PERIPH_BASE_S + 0x8000UL)

/*
 * Peripheral pointers (stm32n657xx.h:3916, 4011, 4265, 4556).
 * ST picks the secure ones only when compiled with -mcmse, otherwise the
 * non-secure ones (0x4...). The core runs secure in dev boot, so secure is fixed here.
 */
#define GPIOG_S                   ((GPIO_TypeDef *) GPIOG_BASE_S)
#define RCC_S                     ((RCC_TypeDef *) RCC_BASE_S)
#define GPIOG                     GPIOG_S
#define RCC                       RCC_S

/* Bit definitions for RCC_AHB4ENR (stm32n657xx.h:28013) */
#define RCC_AHB4ENR_GPIOGEN_Pos                 (6UL)
#define RCC_AHB4ENR_GPIOGEN_Msk                 (0x1UL << RCC_AHB4ENR_GPIOGEN_Pos)    /*!< 0x00000040 */
#define RCC_AHB4ENR_GPIOGEN                     RCC_AHB4ENR_GPIOGEN_Msk              /*!< GPIO G enable */

/* Bit definitions for GPIO_MODER, LED pins 0, 8, 10 (stm32n657xx.h:20114) */
#define GPIO_MODER_MODE0_Pos           (0UL)
#define GPIO_MODER_MODE0_Msk           (0x3UL << GPIO_MODER_MODE0_Pos)         /*!< 0x00000003 */
#define GPIO_MODER_MODE0               GPIO_MODER_MODE0_Msk
#define GPIO_MODER_MODE0_0             (0x1UL << GPIO_MODER_MODE0_Pos)         /*!< 0x00000001 */
#define GPIO_MODER_MODE0_1             (0x2UL << GPIO_MODER_MODE0_Pos)         /*!< 0x00000002 */
#define GPIO_MODER_MODE8_Pos           (16UL)
#define GPIO_MODER_MODE8_Msk           (0x3UL << GPIO_MODER_MODE8_Pos)         /*!< 0x00030000 */
#define GPIO_MODER_MODE8               GPIO_MODER_MODE8_Msk
#define GPIO_MODER_MODE8_0             (0x1UL << GPIO_MODER_MODE8_Pos)         /*!< 0x00010000 */
#define GPIO_MODER_MODE8_1             (0x2UL << GPIO_MODER_MODE8_Pos)         /*!< 0x00020000 */
#define GPIO_MODER_MODE10_Pos          (20UL)
#define GPIO_MODER_MODE10_Msk          (0x3UL << GPIO_MODER_MODE10_Pos)        /*!< 0x00300000 */
#define GPIO_MODER_MODE10              GPIO_MODER_MODE10_Msk
#define GPIO_MODER_MODE10_0            (0x1UL << GPIO_MODER_MODE10_Pos)        /*!< 0x00100000 */
#define GPIO_MODER_MODE10_1            (0x2UL << GPIO_MODER_MODE10_Pos)        /*!< 0x00200000 */

/* Bit definitions for GPIO_BSRR, LED pins 0, 8, 10 (stm32n657xx.h:20510) */
#define GPIO_BSRR_BS0_Pos              (0UL)
#define GPIO_BSRR_BS0_Msk              (0x1UL << GPIO_BSRR_BS0_Pos)            /*!< 0x00000001 */
#define GPIO_BSRR_BS0                  GPIO_BSRR_BS0_Msk
#define GPIO_BSRR_BS8_Pos              (8UL)
#define GPIO_BSRR_BS8_Msk              (0x1UL << GPIO_BSRR_BS8_Pos)            /*!< 0x00000100 */
#define GPIO_BSRR_BS8                  GPIO_BSRR_BS8_Msk
#define GPIO_BSRR_BS10_Pos             (10UL)
#define GPIO_BSRR_BS10_Msk             (0x1UL << GPIO_BSRR_BS10_Pos)           /*!< 0x00000400 */
#define GPIO_BSRR_BS10                 GPIO_BSRR_BS10_Msk
#define GPIO_BSRR_BR0_Pos              (16UL)
#define GPIO_BSRR_BR0_Msk              (0x1UL << GPIO_BSRR_BR0_Pos)            /*!< 0x00010000 */
#define GPIO_BSRR_BR0                  GPIO_BSRR_BR0_Msk
#define GPIO_BSRR_BR8_Pos              (24UL)
#define GPIO_BSRR_BR8_Msk              (0x1UL << GPIO_BSRR_BR8_Pos)            /*!< 0x01000000 */
#define GPIO_BSRR_BR8                  GPIO_BSRR_BR8_Msk
#define GPIO_BSRR_BR10_Pos             (26UL)
#define GPIO_BSRR_BR10_Msk             (0x1UL << GPIO_BSRR_BR10_Pos)           /*!< 0x04000000 */
#define GPIO_BSRR_BR10                 GPIO_BSRR_BR10_Msk

#endif /* STM32N657xx_H */
