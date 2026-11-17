So far I have only talked to the board through the debugger, with the boot ROM as the only code running. Now I write my own program: the smallest one I can think of.

## main.c

```c
int main(void) {
    volatile int counter = 0;

    while (1) {
        counter++;
    }
}
```

It does nothing visible, but with the debugger I can halt the core and see `counter` grow, which is enough to know my code is running.

`volatile` tells the compiler that the variable can change or be read outside what it sees in the code, so every access must really happen in memory. Without it, the compiler sees that `counter` is never used and is free to remove it, leaving only an empty `while (1)`. With optimisation on, there would be nothing to look at in the debugger.

`main` never returns. On a desktop, `main` returns to the operating system; here there is nothing to return to.

## How does it run?

On a PC, the operating system loads a program and calls `main`. On the MCU there is no operating system, so the questions are:

- Where in memory does the code go?
- How does the core know where to start?
- What has to happen before `main`?

### From source to a binary

Building happens in two steps:

1. **Compiling**: each `.c` file becomes an object file (`.o`) with machine code and data, but **no final addresses yet**. Code that calls a function in another file only leaves a note: "address of `main` goes here".
2. **Linking**: the linker combines all object files into one **ELF** file. It decides where every piece goes in memory, fills in the addresses, and checks that nothing is missing.

The linker groups what the compiler produced into **sections**:

| Section | Contents                                                         |
| ------- | ---------------------------------------------------------------- |
| `.text` | machine code (and constants, in `.rodata`)                       |
| `.data` | global variables with an initial value, e.g. `int x = 5;`        |
| `.bss`  | global variables that start at 0, e.g. `int y;`                  |
| stack   | local variables and return addresses; grows down from its top    |

On a PC the linker already knows the memory layout. For an MCU it does not: I have to tell it which memories exist and where each section goes. That is the job of the **linker script**.

### What runs first

After a reset, a Cortex-M core does not start at `main`. It reads a table at the start of the program, the **vector table** (the boot ROM has one too, see [03](<03 - Exploring with STM32CubeProgrammer.md#Reading the ROM>)):

- word 0: the **initial stack pointer**, loaded into `SP`
- word 1: the address of the **Reset_Handler**, loaded into `PC`
- then the addresses of the fault and interrupt handlers

The Reset_Handler is the first code of mine that runs. Before calling `main` it has to prepare what C expects to be ready:

- copy the initial values of `.data` to RAM
- fill `.bss` with zeros
- optionally set up the system (clocks, FPU, …), in `SystemInit`
- call `main`

In C, `int y;` at global scope is guaranteed to be 0 and `int x = 5;` to be 5. On the MCU nothing does that for free; the startup code does.

### Where it runs on this board

In dev boot the boot ROM does not start anything (see [02](<02 - The boot process.md>)). The debugger loads the ELF straight into internal RAM and points the core at my vector table, so the program never touches flash.

## What I need

To get from `main.c` to a program running on the board:

- **device headers** (CMSIS): register definitions and core functions for the Cortex-M55 and the STM32N6
- **startup code**: the vector table and the Reset_Handler
- **linker script**: the memory map and where each section goes
- a way to **build** it (the toolchain in Docker) and **load** it (GDB + OpenOCD)
