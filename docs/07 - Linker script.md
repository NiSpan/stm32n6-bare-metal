The linker script tells the linker which memories the chip has and where each part of the program goes (see [05](<05 - A first program.md#From source to a binary>)). I write it before the startup code because the startup code uses the addresses it defines: where the stack starts, where `.data` and `.bss` are.

References: Arm template `CMSIS/Template/Device_M/Config/Device_gcc.ld`, ST template `STM32N657XX_LRUN.ld` (see [06](<06 - CMSIS.md#ST linker scripts>)), [GNU ld manual: linker scripts](https://sourceware.org/binutils/docs/ld/Scripts.html). My script: `linker/STM32N657XX.ld`.

## The Arm template

`Device_gcc.ld` is generic: a Cortex-M with flash at `0x00000000` and RAM at `0x20000000`. The addresses do not fit the N6, but the comments explain the structure, and the list at the top is the most useful part. It shows the **contract** between linker script and startup code:

- symbols the script **needs from the code**: `Reset_Handler`
- symbols the script **defines for the code**: `__data_start__`, `__bss_start__`, `__StackTop`, `__StackLimit`, `__HeapLimit`, …

The startup code never hardcodes an address: it asks the linker through these symbols.

It also shows the most important idea for code that runs from flash: `.data` is placed `> RAM AT > FLASH`. A section has two addresses:

- **VMA** (virtual memory address): where it lives while the program runs, here RAM
- **LMA** (load memory address): where its bytes are stored in the image, here flash

Flash keeps the initial values of `.data` (for `int x = 5;`, the `5`), and the startup code copies them to RAM before `main`. `.bss` has no initial values to store, so it only gets a RAM address and is zeroed by the startup code.

## The ST template

ST's `STM32N657XX_LRUN.ld` is the same idea with real N6 addresses. "LRUN" (load and run) means the program is loaded straight into internal RAM, which is what the debugger does in dev boot:

```
MEMORY
{
  ROM    (xrw)    : ORIGIN = 0x34000400,   LENGTH = 511K
  RAM    (xrw)    : ORIGIN = 0x34080000,   LENGTH = 1536K
}
```

Both regions are in **AXISRAM**; "ROM" is only a name for the part that holds code and the initial values of `.data`. The structure stays flash-like: `.data` is still `>RAM AT> ROM` and copied at startup. ST's symbols, which ST's startup file uses:

| Symbol             | Meaning                                         |
| ------------------ | ----------------------------------------------- |
| `_estack`          | top of the stack (end of RAM)                   |
| `_sidata`          | where the initial values of `.data` are stored (LMA) |
| `_sdata`, `_edata` | start and end of `.data` in RAM (VMA)           |
| `_sbss`, `_ebss`   | start and end of `.bss`                         |

The rest of the file is for things I do not use yet: C++ constructors (`.init_array`), exception tables (`.ARM.exidx`), a heap, TrustZone veneers (`.gnu.sgstubs`), a non-cacheable section.

## My linker script

I keep ST's structure and symbol names, so ST's startup code would still fit, and take only what `main.c` needs.

### Entry point

```
ENTRY(Reset_Handler)
```

`ENTRY` writes the address of `Reset_Handler` into the ELF header. The core never reads it: at reset it takes SP and PC from the vector table. The **tools** read it:

- **GDB**: `load` copies the program into RAM and sets `PC` to the entry point. In dev boot nothing else starts my code, so this is how `Reset_Handler` runs.
- **`--gc-sections`**: the linker removes code nothing refers to. The entry point is where it starts looking, so `Reset_Handler` and everything it calls (`main`) are kept.

### Memory

```
MEMORY
{
    RAM (xrw) : ORIGIN = 0x34064000, LENGTH = 0x20000  /* 128 KB */
}
```

One region in AXISRAM1 (secure alias), 128 KB: code, data and stack all go there. It starts after the first 400 KB of AXISRAM1 (FLEXRAM, which can be given to the TCMs, see `SYSCFG_CM55TCMCR` in [03](<03 - Exploring with STM32CubeProgrammer.md#Reading a register>)), so changing the TCM size later does not move my program. `xrw` = executable, readable, writable.

### Stack

```
_Min_Stack_Size = 0x800;

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - _Min_Stack_Size;
```

```
0x34064000  start of RAM: vector table, code, data, bss
     ...
0x34083800  _sstack: bottom of the stack (2 KB)
0x34084000  _estack: top of the stack, it grows down from here
```

These are **symbols**: names for an address, with no memory behind them. They will be used:

- `_estack`: the **first word of the vector table**, so the core loads it into `MSP` at reset
- `_sstack`: written to **`MSPLIM`** by the startup code, so a stack that grows too far faults instead of overwriting `.bss` (the `msplim` registers I saw in [04](<04 - Exploring with GDB.md#Registers>))

In C a symbol is declared as `extern uint32_t _estack;` and used as `&_estack`, its address. Reading `_estack` itself would read whatever is stored at that address.

### Sections

`SECTIONS` builds the ELF's **output** sections from the **input** sections in the `.o` files:

```
.text : { *(.text*) } > RAM
 |         | |           '- place it in region RAM
 |         | '- input sections named .text, .text.main, ...
 |         '- from every input file
 '- output section name
```

Mine, in order from the start of RAM:

```
.isr_vector :
{
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
} > RAM
```

The **vector table** comes first, so it is at `0x34064000`. `VTOR` (where the core looks for the table) needs the table start aligned to its size, and the start of RAM gives that. `KEEP` stops `--gc-sections` from removing it: no code calls the vector table, so the linker would think it is unused. The startup code will put the table in this section with `__attribute__((section(".isr_vector")))`.

`.` is the **location counter**: the address where the next byte goes. `ALIGN(4)` rounds it up to a multiple of 4. One at the start aligns the contents, one at the end makes the size a multiple of 4.

```
.text   : { . = ALIGN(4); *(.text) *(.text*) . = ALIGN(4); } > RAM
.rodata : { . = ALIGN(4); *(.rodata) *(.rodata*) . = ALIGN(4); } > RAM
```

Code, then constants. `*(.text*)` also catches `.text.main` and similar: with `-ffunction-sections` each function gets its own section, so unused ones can be removed one by one.

```
_sidata = LOADADDR(.data);

.data :
{
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
} > RAM
```

`_sdata = .;` defines a symbol at the current address. Unlike ST, `.data` has no `AT>`: the debugger loads everything straight into RAM, so the initial values are already in place and `_sidata` = `_sdata`. I still define `_sidata` and the startup code still copies: it costs nothing here, and it is needed once the program is stored somewhere else (external flash).

```
.bss :
{
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss*)
    . = ALIGN(4);
    _ebss = .;
} > RAM
```

`.bss` takes RAM but has no bytes in the ELF (type `NOBITS`); the startup code zeroes it from `_sbss` to `_ebss`. The `ALIGN(4)` at both ends matter here: the startup code copies and zeroes one 4-byte word at a time.

### Overflow check

```
ASSERT(_ebss <= _sstack, "Data section flowed into stack!")
```

If code and data grow into the stack area, the link fails with this message instead of producing a program that breaks at run time.

## How the symbols are used later

| Symbol                        | Used by              | For                                             |
| ----------------------------- | -------------------- | ----------------------------------------------- |
| `Reset_Handler`               | `ENTRY`, GDB `load`  | where execution starts                          |
| `.isr_vector` (section)       | startup code         | puts the vector table at the start of RAM       |
| `_estack`                     | vector table word 0  | initial `MSP`                                   |
| `_sstack`                     | Reset_Handler        | `MSPLIM`, stack overflow detection              |
| `_sidata`, `_sdata`, `_edata` | Reset_Handler        | copy `.data` initial values                     |
| `_sbss`, `_ebss`              | Reset_Handler        | zero `.bss`                                     |
