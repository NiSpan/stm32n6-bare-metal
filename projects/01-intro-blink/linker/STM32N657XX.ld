/*
 * ENTRY(symbol)
 * -------------
 * Standard GNU ld command (not ST or STM32N6 specific). It writes the
 * symbol's address into the ELF header field "e_entry" (the entry point).
 * Check it with:  arm-none-eabi-readelf -h <file>.elf  -> "Entry point address"
 * The address is odd (bit 0 = 1) because Cortex-M code is Thumb.
 *
 * The Cortex-M core never reads it: the ELF header exists only in the .elf
 * file on the PC, not on the chip. At reset a Cortex-M takes SP/PC from the
 * vector table. The entry point is used by the tools that read the ELF:
 *
 *  1. Debugger (matters in dev boot): GDB "load" copies the sections into
 *     RAM and sets PC to the entry point. This is how Reset_Handler gets
 *     started in dev boot, since the boot ROM is not involved.
 *
 *  2. Garbage collection (--gc-sections): ld removes every section that
 *     nothing references. The entry symbol is a root, so it and everything
 *     reachable from it survive (Reset_Handler -> main). The vector table
 *     is not called by anything, so it also needs KEEP() in SECTIONS.
 *
 *  3. Default: without ENTRY (or -e on the command line) ld looks for
 *     "_start" (provided by newlib, not linked here), and otherwise warns
 *     "cannot find entry symbol _start; defaulting to ..." and uses the
 *     start of .text.
 *
 * Placement in the script does not matter to ld; putting it at the top is a
 * convention. Usual order: ENTRY, constants (stack/heap size), MEMORY, SECTIONS.
 */
ENTRY(Reset_Handler)

/*
 * MEMORY: one region, RAM, in AXISRAM1 (secure alias), after the 400 KB
 * FLEXRAM. Code, data, bss and stack all live here.
 *   0x34064000  start of RAM (vector table goes first)
 *   0x34083800  _sstack: bottom of stack -> MSPLIM
 *   0x34084000  _estack: top of stack    -> initial MSP (stack grows down)
 *
 * _estack/_sstack are symbols: names for an address, no memory behind them.
 * In C:  extern uint32_t _estack;   then use &_estack (the address),
 * never _estack itself (that would read whatever is stored there).
 */
MEMORY
{
    RAM (xrw) : ORIGIN = 0x34064000, LENGTH = 0x20000  /* 128 KB */
}

_Min_Stack_Size = 0x800;

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - _Min_Stack_Size;

/*
 * SECTIONS: tells ld how to build the output sections of the .elf from the
 * input sections of the .o files, and where to put them in MEMORY.
 *
 *   .text : { *(.text*) } > RAM
 *   |         |  |          '- place this output section in region RAM
 *   |         |  '- input sections named .text, .text.main, ...
 *   |         '- from every input file (*)
 *   '- name of the output section in the .elf
 *
 * Minimum for a simple main.c (in this order, at the start of RAM):
 *   .isr_vector  vector table, first so it sits at ORIGIN(RAM) (VTOR aligned)
 *   .text        code
 *   .rodata      constants and string literals (const)
 *   .data        initialized globals/statics   (int x = 5;)
 *   .bss         zero-initialized globals/statics (int y;). Type NOBITS:
 *                takes RAM but no bytes in the .elf; startup must zero it
 *
 * .   the location counter: a cursor holding the address where ld puts the
 *     next byte. Placing an input section moves it forward by its size.
 *     ". = X;" is an assignment: you move the cursor yourself.
 *
 * ALIGN(n) is a function: returns "." rounded UP to a multiple of n
 *     (unchanged if already a multiple). So ". = ALIGN(4);" means "move
 *     the cursor forward to the next 4-byte boundary"; skipped bytes are
 *     padding. Example with a 7-byte input section:
 *
 *       . = ALIGN(4);          0x34064000 -> already aligned, no change
 *       KEEP(*(.isr_vector))   placed 0x34064000..06, cursor at 0x34064007
 *       . = ALIGN(4);          0x34064007 -> 0x34064008 (1 byte padding)
 *                              next section starts at 0x34064008
 *
 *     Why two: the first aligns the START of the contents (whatever came
 *     before may have left the cursor unaligned). The second aligns the END,
 *     so the section size is a multiple of 4, the next section starts
 *     aligned, and a symbol defined there (_edata = .;) is word aligned.
 *     That matters for .data/.bss: startup copies/zeroes them one 4-byte
 *     word at a time from start to end symbol.
 *     For .isr_vector both are redundant (ORIGIN is aligned, the table is
 *     16 x 4 bytes) but kept as a habit, like ST does on every section.
 *     Not the same as the VTOR rule: VTOR needs the table START aligned to
 *     the table size (>= 128 bytes, 1024 for the full N6 table). Placing it
 *     first at ORIGIN(RAM) gives that.
 *
 * KEEP(...)  never remove these input sections in --gc-sections. Needed
 *     for the vector table: no code calls or references it.
 *
 * _sdata = .;  defines a symbol at the cursor's current address (no memory
 *     is reserved). Startup uses it as &_sdata.
 *
 * With -ffunction-sections/-fdata-sections each function/variable gets its
 * own input section (.text.main, .bss.counter), which is why *(.text*) uses
 * the trailing * and why --gc-sections can drop unused ones one by one.
 * Input sections not listed here ("orphans") are still placed by ld
 * somewhere it chooses; --orphan-handling=warn shows them.
 */
SECTIONS
{
    .isr_vector : 
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > RAM

    .text : 
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > RAM

    .rodata : 
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > RAM

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

    .bss : 
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    ASSERT(_ebss <= _sstack, "Data section flowed into stack!")
}
