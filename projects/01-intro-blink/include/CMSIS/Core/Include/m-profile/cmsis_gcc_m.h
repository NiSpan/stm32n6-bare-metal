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
 * Modified: trimmed from CMSIS 6 m-profile/cmsis_gcc_m.h to __set_MSP and __set_MSPLIM.
 */

#ifndef __CMSIS_GCC_M_H
#define __CMSIS_GCC_M_H

/* cmsis_gcc_m.h:309 */
__STATIC_FORCEINLINE void __set_MSP(uint32_t topOfMainStack)
{
  __ASM volatile ("MSR msp, %0" : : "r" (topOfMainStack) : );
}

/* cmsis_gcc_m.h:679 (inner part) */
__STATIC_FORCEINLINE void __set_MSPLIM(uint32_t MainStackPtrLimit)
{
  __ASM volatile ("MSR msplim, %0" : : "r" (MainStackPtrLimit));
}

#endif /* __CMSIS_GCC_M_H */
