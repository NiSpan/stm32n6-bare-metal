I have `main.c`, the startup and system files ([08](<08 - Startup code.md>)) and the linker script ([07](<07 - Linker script.md>)). Now I turn them into a program for the board. My PC is x86, the board is Arm, so I need a **cross compiler**: it runs on one machine and produces code for another.

I first build by hand, one command at a time, to see what each step produces. Then I put the same steps in a Makefile.

References: [GCC Arm options](https://gcc.gnu.org/onlinedocs/gcc/ARM-Options.html), [GNU ld manual](https://sourceware.org/binutils/docs/ld/), [GNU make manual](https://www.gnu.org/software/make/manual/make.html).

## The toolchain

The toolchain is in the Docker container ([00.1 - Docker](<00.1 - Docker.md>)):

```sh
docker compose run --rm stm32-dev
```

```
developer@c1a4fc9da190:/workspace$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (15:14.2.rel1-1) 14.2.1 20241119
```

At this point the project was mounted at `/workspace` in the container, so the outputs in this note show that path. Later it moves to the same path as on the host ([00.1 - Docker](<00.1 - Docker.md>)).

`arm-none-eabi` is the **target**:

- **arm**: the processor family
- **none**: no operating system (bare metal)
- **eabi**: Embedded Application Binary Interface, the Arm rules for calling functions and laying out data, so code from different compilers and libraries fits together

Every tool has that prefix (`ls /usr/bin/arm-none-eabi-*`). The ones I use:

| Tool              | What it does                                                                         |
| ----------------- | ------------------------------------------------------------------------------------ |
| `gcc`             | compiler **driver**: runs the preprocessor, compiler, assembler and linker as needed  |
| `as`              | assembler: assembly to machine code (called by `gcc`)                                |
| `ld`              | linker: object files + linker script to ELF (called by `gcc`)                        |
| `objdump`         | shows what is inside an object or ELF: sections, disassembly                         |
| `nm`              | lists the symbols and their addresses                                                |
| `readelf`         | shows the ELF headers (entry point, segments, …)                                     |
| `size`            | sizes of `.text`, `.data`, `.bss`                                                    |
| `objcopy`         | converts an ELF to other formats (`.bin`, `.hex`)                                    |

## Compiling one file

```sh
mkdir -p build
arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -O0 -g -c src/main.c -o build/main.o
```

| Flag               | Meaning                                                              |
| ------------------ | -------------------------------------------------------------------- |
| `-mcpu=cortex-m55` | generate code for this core (its instructions, FPU, Helium)          |
| `-mthumb`          | Thumb instruction set, the only one a Cortex-M runs                  |
| `-O0`              | no optimisation: the code follows the C source line by line          |
| `-g`               | debug information, so GDB can show source lines and variable names   |
| `-c`               | compile only, do not link: the result is an object file              |
| `-o <file>`        | output file                                                          |

### Sections in the object file

`objdump -h` lists the sections:

```
build/main.o:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00000012  00000000  00000000  00000034  2**1
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .data         00000000  00000000  00000000  00000046  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  2 .bss          00000000  00000000  00000000  00000046  2**0
                  ALLOC
  3 .debug_info   0000005d  00000000  00000000  00000046  2**0
                  CONTENTS, RELOC, READONLY, DEBUGGING, OCTETS
  4 .debug_abbrev 00000054  00000000  00000000  000000a3  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  5 .debug_aranges 00000020  00000000  00000000  000000f7  2**0
                  CONTENTS, RELOC, READONLY, DEBUGGING, OCTETS
  6 .debug_line   00000046  00000000  00000000  00000117  2**0
                  CONTENTS, RELOC, READONLY, DEBUGGING, OCTETS
  7 .debug_str    00000086  00000000  00000000  0000015d  2**0
                  CONTENTS, READONLY, DEBUGGING, OCTETS
  8 .comment      00000027  00000000  00000000  000001e3  2**0
                  CONTENTS, READONLY
  9 .debug_frame  0000002c  00000000  00000000  0000020c  2**2
                  CONTENTS, RELOC, READONLY, DEBUGGING, OCTETS
 10 .ARM.attributes 00000036  00000000  00000000  00000238  2**0
                  CONTENTS, READONLY
```

- `elf32-littlearm`: an ELF file for 32-bit Arm, little-endian. Object files are ELF too, not only the final program.
- `.text` is 18 bytes (`0x12`): the code of `main`.
- `.data` and `.bss` are empty: `counter` is a **local** variable, so it lives on the stack, not in a section.
- `.debug_*` come from `-g`: the map from machine code back to source lines and variables. They are not loaded on the chip (no `ALLOC`).
- `.comment` holds the compiler version, `.ARM.attributes` the CPU and ABI the code was built for.
- **VMA and LMA are all 0**: nothing has an address yet; that is the linker's job.

### The code

`objdump -d` disassembles `.text`:

```
00000000 <main>:
   0:    b480          push    {r7}
   2:    b083          sub    sp, #12
   4:    af00          add    r7, sp, #0
   6:    2300          movs    r3, #0
   8:    607b          str    r3, [r7, #4]
   a:    687b          ldr    r3, [r7, #4]
   c:    3301          adds    r3, #1
   e:    607b          str    r3, [r7, #4]
  10:    e7fb          b.n    a <main+0xa>
```

- `push`, `sub sp`, `add r7`: make room on the stack for `counter` and use `r7` as the frame pointer
- `movs` + `str`: `counter = 0`, stored at `r7 + 4`
- `ldr`, `adds`, `str`: `counter++`, read from memory, add 1, write back. That is `volatile` at work: every pass really goes to memory
- `b.n a`: jump back, the `while (1)`
- Most instructions are 2 bytes: Thumb mixes 16-bit and 32-bit instructions

Addresses start at 0 here too: this is a position inside `main.o`, not an address on the chip.

## Compiling the startup and system files

With the same command, the startup file fails:

```
developer@c1a4fc9da190:/workspace$ arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -O0 -g -c src/startup_stm32n657xx.c -o build/startup_stm32n657xx.o
src/startup_stm32n657xx.c:7:10: fatal error: stm32n6xx.h: No such file or directory
    7 | #include "stm32n6xx.h"
      |          ^~~~~~~~~~~~~
```

`#include "…"` is searched next to the `.c` file and in the folders given with `-I`. My headers are in `include/`, so I add both folders:

```sh
arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -O0 -g \
  -Iinclude/CMSIS/Core/Include -Iinclude/CMSIS/Device/ST/STM32N6xx/Include \
  -c src/startup_stm32n657xx.c -o build/startup_stm32n657xx.o

arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -O0 -g \
  -Iinclude/CMSIS/Core/Include -Iinclude/CMSIS/Device/ST/STM32N6xx/Include \
  -c src/system_stm32n6xx.c -o build/system_stm32n6xx.o
```

`main.c` includes nothing, so it did not need them. Each `.c` file is compiled on its own, with only what it includes.

```
build/startup_stm32n657xx.o:     file format elf32-littlearm

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00000090  00000000  00000000  00000034  2**2
                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, CODE
  1 .data         00000000  00000000  00000000  000000c4  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  2 .bss          00000000  00000000  00000000  000000c4  2**0
                  ALLOC
  3 .isr_vector   00000040  00000000  00000000  000000c4  2**2
                  CONTENTS, ALLOC, LOAD, RELOC, READONLY, DATA
  ...debug sections, .comment, .ARM.attributes as in main.o
```

- **`.isr_vector`** is new: the vector table, in the section I named with `__attribute__((section(".isr_vector")))`. `0x40` = 64 bytes = 16 entries of 4 bytes.
- **`RELOC`** on `.text` and `.isr_vector`: the section has **relocations**, places where an address must be filled in later. The table holds `&_estack`, `Reset_Handler`, … and `Reset_Handler` calls `main`, but none of those addresses is known yet.

### What the object file still needs

`nm` lists the symbols in an object file:

```
developer@c1a4fc9da190:/workspace$ arm-none-eabi-nm build/startup_stm32n657xx.o
0000008a W BusFault_Handler
0000008a W DebugMon_Handler
0000008a T Default_Handler
00000084 W HardFault_Handler
0000008a W MemManage_Handler
0000008a W NMI_Handler
0000008a W PendSV_Handler
00000000 T Reset_Handler
0000008a W SVC_Handler
0000008a W SecureFault_Handler
0000008a W SysTick_Handler
         U SystemInit
0000008a W UsageFault_Handler
00000000 R __Vectors
         U _ebss
         U _edata
         U _estack
         U _sbss
         U _sdata
         U _sidata
         U _sstack
         U main
```

| Letter | Meaning                                              |
| ------ | ---------------------------------------------------- |
| `T`    | defined here, in `.text` (code)                      |
| `R`    | defined here, read-only data                         |
| `W`    | weak: used unless another file defines the same name |
| `U`    | **undefined**: used here, defined somewhere else     |

- All the aliased handlers have the same address as `Default_Handler` (`0x8a`): `alias` really is one piece of code with many names. `HardFault_Handler` has its own body at `0x84`.
- The `U` symbols are what the linker must find: `main` (in `main.o`), `SystemInit` (in `system_stm32n6xx.o`) and the `_s…`/`_e…` symbols (in the linker script).

`objdump -r` shows the relocations, here only for the vector table (`-j .isr_vector`):

```
RELOCATION RECORDS FOR [.isr_vector]:
OFFSET   TYPE              VALUE
00000000 R_ARM_ABS32       _estack
00000004 R_ARM_ABS32       Reset_Handler
00000008 R_ARM_ABS32       NMI_Handler
0000000c R_ARM_ABS32       HardFault_Handler
00000010 R_ARM_ABS32       MemManage_Handler
00000014 R_ARM_ABS32       BusFault_Handler
00000018 R_ARM_ABS32       UsageFault_Handler
0000001c R_ARM_ABS32       SecureFault_Handler
0000002c R_ARM_ABS32       SVC_Handler
00000030 R_ARM_ABS32       DebugMon_Handler
00000038 R_ARM_ABS32       PendSV_Handler
0000003c R_ARM_ABS32       SysTick_Handler
```

Each line says: at this offset in the table, write the full 32-bit address (`R_ARM_ABS32`) of this symbol. The reserved entries (`0x20`–`0x28`, `0x34`) have no relocation: they are plain 0.

## Linking

Without `-c`, the `gcc` driver calls the linker. First without my linker script:

```sh
arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -nostdlib \
  build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o \
  -o build/app.elf
```

```
ld: warning: cannot find entry symbol _start; defaulting to 00008000
ld: build/startup_stm32n657xx.o: in function `Reset_Handler':
/workspace/src/startup_stm32n657xx.c:92:(.text+0x68): undefined reference to `_sstack'
/workspace/src/startup_stm32n657xx.c:92:(.text+0x6c): undefined reference to `_estack'
/workspace/src/startup_stm32n657xx.c:92:(.text+0x70): undefined reference to `_sidata'
/workspace/src/startup_stm32n657xx.c:92:(.text+0x74): undefined reference to `_sdata'
/workspace/src/startup_stm32n657xx.c:92:(.text+0x7c): undefined reference to `_sbss'
/workspace/src/startup_stm32n657xx.c:92:(.text+0x80): undefined reference to `_ebss'
build/startup_stm32n657xx.o:/workspace/src/startup_stm32n657xx.c:44:(.isr_vector+0x0): undefined reference to `_estack'
collect2: error: ld returned 1 exit status
```

- `-nostdlib`: no C library and no C library start code (`crt0` with `_start`). My Reset_Handler is the start.
- `main` and `SystemInit` are found: they are in the other object files.
- The `_s…`/`_e…` symbols are not: nothing defines them. `ld` used its **default linker script**, which knows nothing about my memory and puts code at `0x8000`. That script also expects an entry symbol `_start`.
- `_edata` is missing from the list: the default script happens to define a symbol with that name. It would link, but to the wrong place.

With my linker script (`-T`):

```sh
arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -nostdlib -T linker/STM32N657XX.ld \
  build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o \
  -o build/app.elf
```

No output means success: `build/app.elf` is the program.

## The linked program

### Size

```
developer@c1a4fc9da190:/workspace$ arm-none-eabi-size build/app.elf
   text       data        bss        dec        hex    filename
    256          0          0        256        100    build/app.elf
```

`text` counts everything read-only: `.isr_vector` (`0x40`) + `.text` (`0xc0`) = 256 bytes. No `.data` or `.bss`: `main.c` has no global variables.

### Sections

```
Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .isr_vector   00000040  34064000  34064000  00004000  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  1 .text         000000c0  34064040  34064040  00004040  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  2 .rodata       00000000  34064100  34064100  00004100  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  3 .data         00000000  34064100  34064100  00004100  2**0
                  CONTENTS, ALLOC, LOAD, DATA
  4 .bss          00000000  34064100  34064100  00004100  2**0
                  ALLOC
  ...debug sections, .comment, .ARM.attributes
```

The sections now have addresses, in the order of the linker script: the vector table at `0x34064000`, the start of RAM, then `.text` right after it. VMA = LMA everywhere: the program runs where it is loaded. The `.text` sections of the three object files are joined into one.

### Symbols

`nm -n` sorts by address:

```
00000800 A _Min_Stack_Size
34064000 R __Vectors
34064040 T main
34064054 T Reset_Handler
340640d8 W HardFault_Handler
340640de W BusFault_Handler
340640de W DebugMon_Handler
340640de T Default_Handler
...
340640e4 T SystemInit
34064100 B _ebss
34064100 D _edata
34064100 B _sbss
34064100 D _sdata
34064100 A _sidata
34083800 A _sstack
34084000 R _estack
```

- No `U` left: every reference is resolved.
- `main` comes first in `.text` because `main.o` was first on the command line.
- The linker script symbols have the values from [07](<07 - Linker script.md#Stack>): `_estack` = `0x34084000`, `_sstack` = `0x34083800`. `_sidata` = `_sdata`, as expected without `AT>`.
- `.data` and `.bss` are empty, so their start and end symbols are all the same address: the copy and zero loops in Reset_Handler run zero times.
- `A` = absolute: a plain number defined in the linker script, not tied to a section (`_Min_Stack_Size` is a size, not an address).

### ELF header

```
  Type:                              EXEC (Executable file)
  Machine:                           ARM
  Entry point address:               0x34064055
  ...
  Flags:                             0x5000200, Version5 EABI, soft-float ABI
```

- The entry point is `Reset_Handler` (`0x34064054`) with bit 0 set: the Thumb bit. This is what GDB will use as the start `pc` ([07](<07 - Linker script.md#Entry point>)).
- `soft-float ABI`: floating-point arguments are passed in integer registers. The M55 has an FPU; using it for that needs `-mfloat-abi=hard`. Not relevant until the code uses `float`.
