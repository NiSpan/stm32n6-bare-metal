CMSIS lists what an application needs around `main` (see [06](<06 - CMSIS.md>)): a startup file with the vector table and the Reset_Handler, and a system file with `SystemInit`. The startup file is the first code of mine the core runs. It uses the symbols from the linker script ([07](<07 - Linker script.md#How the symbols are used later>)) and ends by calling `main`.

References: Arm template `CMSIS/Template/Device_M/Source/startup_Device.c`, ST `startup_stm32n657xx.s` (in `Source/Templates/gcc/`). My files: `src/startup_stm32n657xx.c`, `src/system_stm32n6xx.c`.

## The two references

The **Arm template** is in C and has the parts every Cortex-M startup file has:

1. declarations of the exception handlers
2. the vector table
3. `Reset_Handler`: set the stack limit, call `SystemInit()`, then `__PROGRAM_START()`
4. a default handler for every exception nobody else handles

`__PROGRAM_START()` hands over to the C library's start code, which copies `.data`, zeroes `.bss` and calls `main`. I link with `-nostdlib` (no C library), so I do those steps myself.

**ST's startup** is in assembly but does the same, with the symbol names from the linker script:

```
Reset_Handler:
  ldr   r0, =_sstack
  msr   MSPLIM, r0
  ldr   r0, =_estack
  mov   sp, r0          /* set stack pointer */
  bl  SystemInit
  ...copy .data from _sidata to _sdata.._edata
  ...zero .bss from _sbss to _ebss
  bl __libc_init_array
  bl main
LoopForever:
  b LoopForever
```

So the steps are clear; I write them in C, following the template's layout and ST's names.

In the sections below I write each part as if I had no headers, then show which CMSIS/ST definition does the same. In the real file I use the headers, so my code looks like any other CMSIS project.

## Linker symbols

```c
extern uint32_t _estack;
extern uint32_t _sstack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
```

`extern` says "defined somewhere else": here, by the linker script. They are addresses, not variables, so I always use them as `&_estack` (see [07](<07 - Linker script.md#Stack>)).

## Handler declarations

```c
void Reset_Handler  (void) __attribute__((noreturn));
void Default_Handler(void) __attribute__((noreturn));

void NMI_Handler        (void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler  (void) __attribute__((weak));
void MemManage_Handler  (void) __attribute__((weak, alias("Default_Handler")));
...
void SysTick_Handler    (void) __attribute__((weak, alias("Default_Handler")));
```

- `noreturn` tells the compiler the function never returns, so it does not need to keep a way back. Reset_Handler has nowhere to return to; the default handler loops forever.
- `alias("Default_Handler")` makes the name point to the same code as `Default_Handler`. Every exception has a handler without me writing each one.
- `weak` means "use this unless another definition exists". If I later write my own `void SysTick_Handler(void) { ... }` in any file, the linker uses mine and drops the weak one. No need to edit the startup file.

> [!note] From the headers
> `__NO_RETURN` is `__attribute__((__noreturn__))`, defined in `cmsis_gcc.h`. I could write the attribute myself; the macro also hides the compiler: with another compiler, `cmsis_compiler.h` picks a different definition.

## Vector table

What the core expects at the start of the program: a list of 32-bit words, the first is the initial stack pointer, the rest are handler addresses (see [05](<05 - A first program.md#What runs first>)). In C that is an array of function pointers:

```c
typedef void (*VECTOR_TABLE_Type)(void);

const VECTOR_TABLE_Type __Vectors[16] __attribute__((section(".isr_vector"), used)) = {
    (VECTOR_TABLE_Type)(&_estack),       /*     Initial Stack Pointer */
    Reset_Handler,                       /*     Reset Handler */
    NMI_Handler,                         /* -14 NMI Handler */
    HardFault_Handler,                   /* -13 Hard Fault Handler */
    MemManage_Handler,                   /* -12 MPU Fault Handler */
    BusFault_Handler,                    /* -11 Bus Fault Handler */
    UsageFault_Handler,                  /* -10 Usage Fault Handler */
    SecureFault_Handler,                 /*  -9 Secure Fault Handler */
    0,                                   /*     Reserved */
    0,                                   /*     Reserved */
    0,                                   /*     Reserved */
    SVC_Handler,                         /*  -5 SVCall Handler */
    DebugMon_Handler,                    /*  -4 Debug Monitor Handler */
    0,                                   /*     Reserved */
    PendSV_Handler,                      /*  -2 PendSV Handler */
    SysTick_Handler,                     /*  -1 SysTick Handler */
};
```

- `VECTOR_TABLE_Type` = "pointer to a function that takes nothing and returns nothing". Word 0 is an address, not a function, so it needs a cast.
- `section(".isr_vector")` puts the array in the section the linker script places first in RAM ([07](<07 - Linker script.md#Sections>)).
- `used` keeps the compiler from dropping it: nothing in the code reads `__Vectors`. (`KEEP` in the linker script does the same for the linker.)
- `const`: the table is never written at run time.
- The negative numbers are the exception numbers CMSIS uses (`SysTick_IRQn = -1`, …); device interrupts start at 0.
- 16 entries: only the Cortex-M55 exceptions. The N6's peripheral interrupts would follow, but `main` uses none yet.

It is the same layout as the boot ROM's table I read in [03](<03 - Exploring with STM32CubeProgrammer.md#Reading the ROM>).

> [!note] From the headers
> `VECTOR_TABLE_Type` comes from the system header (`system_stm32n6xx.h` in my project, following the template's `system_Device.h`), together with `extern const VECTOR_TABLE_Type __Vectors[16];`, so `SystemInit` can find the table.

## Reset_Handler

```c
void Reset_Handler(void) {
    __set_MSPLIM((uint32_t)&_sstack);
    __set_MSP((uint32_t)&_estack);

    SystemInit();

    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst = *src;
        dst++;
        src++;
    }

    dst = &_sbss;
    while (dst < &_ebss) {
        *dst = 0;
        dst++;
    }

    main();
    while (1);
}
```

### Stack

After a real reset the core already loads `MSP` from word 0 of the vector table. In dev boot there is no reset into my code: GDB loads it and sets `PC` to `Reset_Handler`, while `MSP` and `MSPLIM` still have the boot ROM's values (`msp = 0x341037a8`, `msplim_s = 0x34102ff0` in [04](<04 - Exploring with GDB.md#Registers>)). So I set both myself.

**`MSPLIM` first.** The ROM's limit `0x34102ff0` is above my stack top `0x34084000`. If I moved `MSP` first, the next push would be below the limit and fault. Setting the limit to `_sstack` first makes the new stack valid.

C has no way to write a core register, so this needs one assembly instruction, `MSR`:

```c
static inline void set_msplim(uint32_t value) {
    __asm volatile ("MSR msplim, %0" : : "r" (value));
}
```

> [!note] From the headers
> `__set_MSPLIM()` and `__set_MSP()` are exactly this, from `cmsis_gcc_m.h` (included through `cmsis_compiler.h`). They are **intrinsics**: C functions that become one CPU instruction.

### SystemInit

Called before `.data` and `.bss` are ready, as in the template and ST's code, so `SystemInit` must not use global variables. Mine does one thing, in `system_stm32n6xx.c`:

```c
void SystemInit(void) {
    SCB->VTOR = (uint32_t)(&__Vectors[0]);
}
```

`VTOR` (Vector Table Offset Register) tells the core where the vector table is. In dev boot it still points to the boot ROM's table at `0x18000000`, so any exception would run a **ROM** handler. Setting it to `__Vectors` makes exceptions use my table. Without headers this is a write to a fixed core address:

```c
*(volatile uint32_t *)0xE000ED08 = (uint32_t)&__Vectors[0];
```

> [!note] From the headers
> `SCB` is a pointer to a struct laid out like the System Control Block registers, from `core_cm55.h`; `SCB->VTOR` is the register at `0xE000ED08`.

### .data and .bss

The two loops do what C promises before `main`:

- **copy** each word from `_sidata` (stored initial values) to `_sdata`..`_edata`, so `int x = 5;` is 5
- **zero** each word from `_sbss` to `_ebss`, so `int y;` is 0

Word by word, which is why the linker script aligns the start and end of both sections to 4 bytes. In dev boot `_sidata` = `_sdata`, so the copy writes each value onto itself; it is kept so the code also works when the program is stored somewhere else.

### main

`main()` is declared in the startup file (`int main(void);`) and called last. `while (1);` after it catches the case where `main` returns: there is nowhere to go back to.

## Default handlers

```c
void HardFault_Handler(void) { while(1); }
void Default_Handler(void)   { while(1); }
```

An exception nobody handles ends here and stops. With the debugger, a halted core in one of these loops says "something faulted", and the `pc` shows which one. `HardFault_Handler` has its own body (it is `weak` but not an alias), so a hard fault is easy to tell apart from the others.

## What the headers give me

| Used in my startup     | Is                                          | From                              |
| ---------------------- | ------------------------------------------- | --------------------------------- |
| `__NO_RETURN`          | `__attribute__((__noreturn__))`              | `cmsis_gcc.h`                     |
| `VECTOR_TABLE_Type`    | `void (*)(void)`                             | `system_stm32n6xx.h`              |
| `__set_MSPLIM()`       | `MSR msplim, <reg>`                          | `cmsis_gcc_m.h`                   |
| `__set_MSP()`          | `MSR msp, <reg>`                             | `cmsis_gcc_m.h`                   |
| `SystemInit()`         | declaration                                  | `system_stm32n6xx.h`              |
| `SCB->VTOR`            | register at `0xE000ED08`                     | `core_cm55.h`                     |
| `uint32_t`             | 32-bit unsigned integer                      | `<stdint.h>`, through the headers |

The startup file includes only `stm32n6xx.h` and `system_stm32n6xx.h`; everything else comes through them (`stm32n6xx.h` → `stm32n657xx.h` → `core_cm55.h` → `cmsis_compiler.h` → `cmsis_gcc.h`).
