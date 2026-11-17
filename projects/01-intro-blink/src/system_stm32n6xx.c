#include "stm32n6xx.h"
#include "system_stm32n6xx.h"

void SystemInit(void) {
    SCB->VTOR = (uint32_t)(&__Vectors[0]);
}
