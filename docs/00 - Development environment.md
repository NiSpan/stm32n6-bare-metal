## Host

My development machine is a PC running **Debian Linux 13**. Everything in these notes is done from a terminal: editing, building, flashing and debugging.

## Why cross-compile

The PC has an x86-64 processor; the board has an **Arm Cortex-M55**. They run different instruction sets, so code compiled for the PC cannot run on the board.

The board cannot compile for itself either: there is no operating system on it, no file system, and far too little memory for a compiler. So the compiler runs on the PC and generates code for the Arm core. This is **cross-compiling**:

| Term   | Meaning                                  | Here                   |
| ------ | ---------------------------------------- | ---------------------- |
| Host   | the machine the compiler runs on         | PC, x86-64, Debian 13  |
| Target | the machine the generated code runs on   | STM32N657, Cortex-M55  |

A cross toolchain is named after its target. For example `arm-none-eabi-gcc`:

- `arm`: target architecture
- `none`: no operating system on the target (bare metal)
- `eabi`: Embedded Application Binary Interface, the Arm calling and data conventions for embedded code

## The toolchain: GNU Arm

A toolchain is more than a compiler. The GNU one has:

| Part       | Tools                                    | Job                                              |
| ---------- | ---------------------------------------- | ------------------------------------------------ |
| Compiler   | `arm-none-eabi-gcc`                      | C to Arm machine code (object files `.o`)        |
| Binutils   | `as`, `ld`, `objcopy`, `objdump`, `nm`, `readelf`, `size` | assemble, link into an ELF, inspect and convert binaries |
| C library  | newlib (optional)                        | `printf`, `memcpy`, ... for bare metal            |
| Debugger   | `gdb-multiarch`                          | breakpoints, registers, memory on the target      |

I chose GNU because it is free and open source, anyone can install it from the Debian repositories, and it is the de-facto standard for bare-metal Arm: ST, Zephyr and most open projects build with it.

Alternatives:

| Toolchain                        | License        | Note                                              |
| -------------------------------- | -------------- | ------------------------------------------------- |
| Arm GNU Toolchain (Arm download) | free, open     | same GCC, prebuilt by Arm, often newer than Debian's |
| ST's GCC (STM32CubeCLT / IDE)    | free           | GCC build bundled with ST's tools                 |
| LLVM / Clang                     | free, open     | growing support for bare-metal Arm                |
| Arm Compiler for Embedded (AC6)  | commercial     | Clang-based, used by Keil MDK                     |
| IAR Embedded Workbench           | commercial     | own compiler and IDE                              |

Debian packages: `gcc-arm-none-eabi`, `binutils-arm-none-eabi` and `gdb-multiarch` (Debian has no separate Arm GDB; `gdb-multiarch` supports Arm).

## Debugging: OpenOCD

GDB alone cannot talk to the board. It needs a **GDB server** that translates its requests (halt, read memory, load the program) into commands for the debug probe on the board:

I chose **OpenOCD** as the GDB server: free and open source, works with many probes and chips (not only ST), and its configuration is readable scripts, so I can see how it connects to the chip. It also has a console to read and write registers directly.

The Debian package (0.12) is too old for the STM32N6, so I build OpenOCD from upstream source in the Docker image.

Alternatives:

| GDB server                       | License           | Note                             |
| -------------------------------- | ----------------- | -------------------------------- |
| ST-LINK_gdbserver (STM32CubeCLT) | free              | ST's own, ST-LINK and STM32 only |
| pyOCD                            | free, open        | Python, uses CMSIS device packs  |
| probe-rs                         | free, open        | Rust-based, growing chip support |
| SEGGER J-Link GDB Server         | free / commercial | needs a J-Link probe             |

## Setting it up

The setup is split in two notes:

1. [Docker](<00.1 - Docker.md>): installing Docker on the host, and the image with the toolchain, make, gdb-multiarch and OpenOCD.
2. [Board access](<00.2 - Board access.md>): the udev rule that lets Linux (and the container) access the ST-LINK, and STM32CubeProgrammer as an optional host tool.
