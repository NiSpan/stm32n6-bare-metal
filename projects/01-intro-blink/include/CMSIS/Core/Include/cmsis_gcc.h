/*
 * Copyright (c) 2009-2023 Arm Limited. All rights reserved.
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
 * Modified: trimmed from CMSIS 6 cmsis_gcc.h to the definitions used here.
 */

#ifndef __CMSIS_GCC_H
#define __CMSIS_GCC_H

#define __NO_RETURN __attribute__((__noreturn__))

#define __ASM                 __asm                                          /* cmsis_gcc.h:39 */
#define __STATIC_FORCEINLINE  __attribute__((always_inline)) static inline  /* cmsis_gcc.h:48 */

#include "m-profile/cmsis_gcc_m.h"

#endif /* __CMSIS_GCC_H */
