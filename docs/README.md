This is my workspace for the **STM32 NUCLEO-N657X0-Q**: a collection of the steps I took to get this board up and running and to build applications for it, starting from nothing. No IDE and no vendor HAL at first; vendor files only as reference, so I understand what each piece does.

Each step below is a note on what I did, why, and what I ran into. They are in the order I did them, even the small ones.

The code for the notes is in (the link does not work on local Obsidian view) [`projects/`](../projects). Paths like `src/main.c` or `build/` in the notes are relative to the project folder, e.g. `projects/01-intro-blink/`.

## Steps

| Step | Note |
| ---- | ---- |
| 00 | [Development environment](<00 - Development environment.md>) |
| 00.1 | [Docker](<00.1 - Docker.md>) |
| 00.2 | [Board access](<00.2 - Board access.md>) |
| 01 | [Getting to know the board](<01 - Getting to know the board.md>) |
| 02 | [The boot process](<02 - The boot process.md>) |
| 03 | [Exploring with STM32CubeProgrammer](<03 - Exploring with STM32CubeProgrammer.md>) |
| 04 | [Exploring with GDB](<04 - Exploring with GDB.md>) |
| 05 | [A first program](<05 - A first program.md>) |
| 06 | [CMSIS](<06 - CMSIS.md>) |
| 07 | [Linker script](<07 - Linker script.md>) |
| 08 | [Startup code](<08 - Startup code.md>) |
| 09 | [Cross compiling](<09 - Cross compiling.md>) |
| 10 | [Loading and debugging](<10 - Loading and debugging.md>) |
| 11 | [Make and CMake](<11 - Make and CMake.md>) |
| 12 | [Blinky program](<12 - Blinky Program.md>) |

## My setup

| What        | Details                                                                 |
| ----------- | ----------------------------------------------------------------------- |
| OS          | Debian Linux 13                                                         |
| Editor      | Neovim (clangd, Telescope), fish shell                                  |
| Build env   | Docker image: `gcc-arm-none-eabi`, binutils, CMake, make, gdb-multiarch, OpenOCD (built from upstream) |
| Board       | NUCLEO-N657X0-Q (MB1940): STM32N657X0, Cortex-M55, on-board STLINK-V3   |
| Connection  | USB-C to the on-board ST-LINK                                           |

## Resources

The [Resources](<Resources.md>) document lists useful links such as Board and Reference manuals, that I needed along the way, to find answers during development.  
