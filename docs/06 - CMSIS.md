[05](<05 - A first program.md>) ended with a list: device headers, startup code, linker script. I do not invent these from nothing: Arm defines what they should look like (CMSIS), and ST provides versions for the STM32N6. Here I look at both and decide how I use them.

References: [CMSIS documentation](https://arm-software.github.io/CMSIS_6/latest/General/index.html), [CMSIS_6 on GitHub](https://github.com/ARM-software/CMSIS_6), [STM32CubeN6](https://github.com/STMicroelectronics/STM32CubeN6), [cmsis-device-n6](https://github.com/STMicroelectronics/cmsis-device-n6).

## What CMSIS is

**CMSIS** (Common Microcontroller Software Interface Standard) is Arm's set of standard interfaces and software for Cortex-M devices. It has many parts (DSP, NN, RTOS2, drivers, debug tools, …); the one I need is **CMSIS-Core**: the basic run-time system for a Cortex-M device. It gives:

- definitions for the **core registers** (NVIC, SysTick, SCB, MPU, FPU, …) and functions to use them
- standard **exception names**, so every vendor's vector table looks the same
- **intrinsic functions** for CPU instructions C has no syntax for (`__NOP()`, `__WFI()`, `__DSB()`, …)
- a standard way to **start the device**: `SystemInit()`, the `SystemCoreClock` variable, the startup file
- **naming rules** for the files, so every Cortex-M project is organised the same way

## Two kinds of files

| Kind               | Written by        | What they cover                                    | Do I change them?          |
| ------------------ | ----------------- | -------------------------------------------------- | -------------------------- |
| **Standard files** | Arm               | the processor (here Cortex-M55) and the compiler   | no                         |
| **Device files**   | the vendor (ST)   | the chip: peripherals, interrupts, memory, startup | some, for my application   |

### Standard files

Starting from the processor header for my core and following its `#include`s gives the set I need:

| Category             | File                          | Why                                                     |
| -------------------- | ----------------------------- | ------------------------------------------------------- |
| Processor            | `core_cm55.h`                 | Cortex-M55 core registers and functions                 |
| Processor            | `cmsis_version.h`             | CMSIS version                                           |
| Compiler             | `cmsis_compiler.h`            | detects the compiler and includes its file              |
| Compiler             | `cmsis_gcc.h`                 | GCC macros and intrinsics                               |
| Compiler             | `m-profile/cmsis_gcc_m.h`     | GCC parts specific to M-profile cores                   |
| Architecture feature | `m-profile/armv8m_mpu.h`      | Memory Protection Unit                                  |
| Architecture feature | `m-profile/armv8m_pmu.h`      | Performance Monitoring Unit                             |
| Architecture feature | `m-profile/armv7m_cachel1.h`  | L1 instruction/data cache                               |

```
core_cm55.h
├── cmsis_version.h
├── cmsis_compiler.h
│   └── cmsis_gcc.h
│       └── m-profile/cmsis_gcc_m.h
├── m-profile/armv8m_mpu.h        (if __MPU_PRESENT)
├── m-profile/armv8m_pmu.h        (if __PMU_PRESENT)
└── m-profile/armv7m_cachel1.h    (if __ICACHE_PRESENT / __DCACHE_PRESENT)
```

The feature headers are included only if the device header says the chip has that feature.

### Device files

CMSIS-Core defines which device files a vendor provides, and gives **templates** for them, with `ToDo:` comments and placeholders like `<Device>`. ST filled them in for the STM32N6:

| Arm template         | STM32N6 file                                           | Purpose                                                 |
| -------------------- | ------------------------------------------------------ | ------------------------------------------------------- |
| `Device.h`           | `stm32n6xx.h`, `stm32n657xx.h`                         | peripherals, registers, interrupt numbers, core features |
| `system_Device.h`    | `system_stm32n6xx.h`                                   | `SystemInit()`, `SystemCoreClock`                       |
| `system_Device.c`    | `system_stm32n6xx_s.c` / `_ns.c` / `_fsbl.c`           | system initialisation (secure, non-secure, FSBL)        |
| `startup_Device.c`   | `startup_stm32n657xx.s`, `startup_stm32n657xx_fsbl.s`  | vector table and Reset_Handler (ST uses assembly)       |
| `Device_gcc.ld`      | `STM32N657XX_*.ld`                                     | memory layout and section placement                     |
| `partition_Device.h` | `partition_stm32n6xx.h`, `partition_stm32n657xx.h`     | TrustZone configuration                                 |

`stm32n6xx.h` is the family header: it picks the right device header (`stm32n657xx.h`) from a define such as `STM32N657xx`. `stm32n657xx.h` is the big one: every peripheral register, base address and interrupt number. I used it in 03 to check addresses.

## Where to find them

- **Arm templates and standard files**: [CMSIS_6](https://github.com/ARM-software/CMSIS_6), in `CMSIS/Core/Include/` (standard files) and `CMSIS/Core/Template/Device_M/` (device templates).
- **ST device files**: [STM32CubeN6](https://github.com/STMicroelectronics/STM32CubeN6), under `Drivers/CMSIS/Device/ST/STM32N6xx/`, which is the separate repository [cmsis-device-n6](https://github.com/STMicroelectronics/cmsis-device-n6). Headers are in `Include/`, the `system_*.c`, startup files and linker scripts in `Source/Templates/`.

## ST linker scripts

ST provides one linker script per way of running a program:

| Linker script                         | Code runs from                                                   |
| ------------------------------------- | ---------------------------------------------------------------- |
| `STM32N657XX_AXISRAM2_fsbl.ld`        | AXISRAM2, as a First Stage Bootloader loaded by the boot ROM     |
| `STM32N657XX_LRUN.ld`                 | internal SRAM ("load and run"): code and data both in RAM        |
| `STM32N657XX_LRUN_s.ld` / `_ns.ld`    | same, for a TrustZone secure / non-secure application            |
| `STM32N657XX_LRUN_RAMxspi1.ld` / `2`  | internal SRAM, plus external RAM through XSPI1 / XSPI2           |
| `STM32N657XX_XIP.ld`                  | external flash, executed in place; data in internal SRAM         |
| `STM32N657XX_ROMxspi*_RAMxspi*.ld`    | external flash through one XSPI, external RAM through another    |

In dev boot the debugger loads my program into internal RAM, so the **LRUN** scripts are the ones that match.

## How I use them

I do not copy the files and build with them. I write my own, small versions and use three references, each for a different question:

| Reference                        | Question it answers                                         |
| -------------------------------- | ----------------------------------------------------------- |
| Arm templates (`Device_M/`)      | **what** is needed: which parts a startup or system file has |
| ST device files                  | **how** ST adapted it for the STM32N6: vectors, names, values |
| ST linker scripts                | **where** things go in the N6 memory map                     |

I keep the same file names as ST (`startup_stm32n657xx.c`, `system_stm32n6xx.c`, `stm32n657xx.h`, …), so later I can swap in ST's full files without changing the structure.

In the project:

- `CMSIS/`: the original Arm and ST files, untouched, for reference
- `include/CMSIS/`: my own versions of the headers, with only what I need
- `src/`: my `startup_stm32n657xx.c` and `system_stm32n6xx.c`, written in C (ST's startup is assembly)
- `linker/`: my linker script
