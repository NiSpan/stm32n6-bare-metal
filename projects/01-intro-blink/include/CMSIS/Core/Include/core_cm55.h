/*
 * Copyright (c) 2018-2024 Arm Limited. All rights reserved.
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
 * Modified: trimmed from CMSIS 6 core_cm55.h to __IO, __IM, __IOM and the SCB up to VTOR.
 */

#ifndef __CORE_CM55_H
#define __CORE_CM55_H

#include <stdint.h>

#include "cmsis_compiler.h"

/* Register access qualifier, used by the device header (core_cm55.h:307) */
#define __IO     volatile            /* read / write */

/* Register access qualifiers for structure members */
#define __IM     volatile const      /* read only */
#define __IOM    volatile            /* read / write */

/*
 * System Control Block, only up to VTOR. CPUID and ICSR are kept so that
 * VTOR lands at offset 0x008.
 */
typedef struct {
    __IM  uint32_t CPUID;                  /*!< Offset: 0x000 (R/ )  CPUID Base Register */
    __IOM uint32_t ICSR;                   /*!< Offset: 0x004 (R/W)  Interrupt Control and State Register */
    __IOM uint32_t VTOR;                   /*!< Offset: 0x008 (R/W)  Vector Table Offset Register */
} SCB_Type;

#define SCS_BASE        (0xE000E000UL)          /* System Control Space Base Address */
#define SCB_BASE        (SCS_BASE + 0x0D00UL)   /* System Control Block Base Address */
#define SCB             ((SCB_Type *) SCB_BASE) /* SCB Configuration struct */

#endif /* __CORE_CM55_H */
