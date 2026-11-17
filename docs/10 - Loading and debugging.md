I have `build/app.elf` from [09](<09 - Cross compiling.md>). Now I load it into the board's RAM with GDB, start it from `Reset_Handler` and watch what it does: the stack, the vector table, and finally `counter` in `main`.

The setup is the same as in [04](<04 - Exploring with GDB.md>): OpenOCD in one terminal, GDB in another, both in the same container.

## Loading

Terminal 1:

```sh
docker compose run --rm --name stm32-gdb stm32-dev
openocd
```

Terminal 2, this time with the ELF file:

```sh
docker exec -it stm32-gdb bash
gdb-multiarch build/app.elf
```

```
Reading symbols from build/app.elf...
(gdb) target extended-remote :3333
Remote debugging using :3333
0x18003514 in ?? ()
(gdb) monitor reset halt
[stm32n6x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x81000000 pc: 0x18003a1a msp: 0x341037a0
(gdb) maintenance flush register-cache
Register cache flushed.
(gdb) load
Loading section .isr_vector, size 0x40 lma 0x34064000
Loading section .text, size 0xc0 lma 0x34064040
Start address 0x34064054, load size 256
Transfer rate: 2048 bits in <1 sec, 128 bytes/write.
(gdb) info registers pc sp msplim_s
pc             0x34064054          0x34064054 <Reset_Handler>
sp             0x341037a0          0x341037a0
msplim_s       0x34102ff0          0x34102ff0
```

- `Reading symbols`: GDB now knows my functions and source lines, so the `pc` shows as `<Reset_Handler>` instead of `?? ()`.
- `monitor reset halt` gives a clean chip state; the core stops in the boot ROM, as in 04. Still `?? ()` there: the ROM is not in my ELF.
- `load` writes each section with `LOAD` to its **LMA**, the same list `objdump -h` showed in 09. Debug sections are not loaded.
- `Start address` is the **entry point** from the ELF header, and `load` puts it in `pc`. GDB drops the Thumb bit from the displayed address.
- `sp` and `msplim_s` still hold the **boot ROM's** values. `load` only sets `pc`; Reset_Handler has to set the stack ([08](<08 - Startup code.md#Stack>)).

## Memory before running

### The vector table in RAM

```
(gdb) x/16xw 0x34064000
0x34064000 <__Vectors>:    0x34084000    0x34064055    0x340640df    0x340640d9
0x34064010 <__Vectors+16>:    0x340640df    0x340640df    0x340640df    0x340640df
0x34064020 <__Vectors+32>:    0x00000000    0x00000000    0x00000000    0x340640df
0x34064030 <__Vectors+48>:    0x340640df    0x00000000    0x340640df    0x340640df
```

The relocations from 09 are filled in: word 0 is `_estack` (`0x34084000`), word 1 is `Reset_Handler` + Thumb bit (`0x34064055`). The reserved entries are 0.

The `a` (address) format shows what each value points to:

```
(gdb) x/16a 0x34064000
0x34064000 <__Vectors>:    0x34084000    0x34064055 <Reset_Handler>    0x340640df <Default_Handler>    0x340640d9 <HardFault_Handler>
0x34064010 <__Vectors+16>:    0x340640df <Default_Handler>    0x340640df <Default_Handler>    0x340640df <Default_Handler>    0x340640df <Default_Handler>
0x34064020 <__Vectors+32>:    0x0    0x0    0x0    0x340640df <Default_Handler>
0x34064030 <__Vectors+48>:    0x340640df <Default_Handler>    0x0    0x340640df <Default_Handler>    0x340640df <Default_Handler>
```

Every exception except reset and hard fault goes to `Default_Handler`: the `weak, alias` declarations at work. GDB shows one name per address, so `NMI_Handler`, `SysTick_Handler`, … all appear as `Default_Handler`.

### VTOR

```
(gdb) x/xw 0xE000ED08
0xe000ed08:    0x18000000
```

`VTOR` still points to the boot ROM's table. Until `SystemInit` runs, an exception would use the ROM's handlers, not mine.

## Stepping through Reset_Handler

With debug information, GDB can step by **C line** instead of by instruction:

| Command             | What it does                                                             |
| ------------------- | ------------------------------------------------------------------------ |
| `list <function>`   | show the source around a function                                        |
| `next` (`n`)        | run one C line; a function call on that line runs completely             |
| `step` (`s`)        | run one C line, but go **into** a called function                        |

GDB prints the line it will run **next**, not the one it just ran.

```
(gdb) list Reset_Handler
63    void Reset_Handler(void) {
64        /* 1. Stack limit first: MSPLIM = bottom of stack (_sstack).
65         *    Any push below this address now raises a UsageFault (STKOF). */
66        __set_MSPLIM((uint32_t)&_sstack);
67
(gdb) next
66        __set_MSPLIM((uint32_t)&_sstack);
(gdb) info registers sp msplim_s
sp             0x34103788          0x34103788
msplim_s       0x34102ff0          0x34102ff0
(gdb) next
69        __set_MSP((uint32_t)&_estack);
(gdb) info registers sp msplim_s
sp             0x34103788          0x34103788
msplim_s       0x34083800          0x34083800
(gdb) next
71        SystemInit();
(gdb) x/xw 0xE000ED08
0xe000ed08:    0x18000000
(gdb) next
74        uint32_t *src = &_sidata;
(gdb) x/xw 0xE000ED08
0xe000ed08:    0x34064000
```

- **First `next`**: runs the function's **prologue**, the instructions the compiler adds at the start of a function. `sp` drops from `0x341037a0` to `0x34103788`: Reset_Handler made room for its local variables (`src`, `dst`) on the **ROM's stack**, before my own stack exists.
- **Second `next`**: `MSPLIM` = `0x34083800` (`_sstack`). `sp` has not changed yet.
- **Third `next`**: `MSP` = `_estack`.
- **Fourth `next`**: `SystemInit()` ran and `VTOR` = `0x34064000`, my vector table.

### Where the local variables live

`print` evaluates a C expression; `&src` is the address of the local variable `src`:

```
(gdb) info registers sp r7
sp             0x34084000          0x34084000
r7             0x34103788          873478024
(gdb) print &src
$1 = (uint32_t **) 0x34103794
(gdb) print &dst
$2 = (uint32_t **) 0x34103790
```

- `sp` = `0x34084000`: `MSP` is now my stack (`_estack`).
- `r7` is the **frame pointer**: at `-O0` the function reaches its locals as `r7` + offset. It was set in the prologue, so it still points into the ROM's stack.
- `src` and `dst` are at `0x341037..`: **still on the ROM's stack**, although `sp` has moved.

### Why r7 is still on the ROM's stack

The first instructions of `Reset_Handler` (`arm-none-eabi-objdump -d build/app.elf`):

```
34064054 <Reset_Handler>:
34064054:  b580      push  {r7, lr}       ; sp 0x341037a0 -> 0x34103798
34064056:  b084      sub   sp, #16        ; sp -> 0x34103788, room for the locals
34064058:  af00      add   r7, sp, #0     ; r7 = 0x34103788, frame pointer
3406405a:  4b18      ldr   r3, [pc, #96]  ; line 66 starts here: r3 = &_sstack
...
34064060:  f383 880a msr   MSPLIM, r3
```

1. **The prologue runs before my first line.** At `-O0` every function starts the same way: save `r7` and `lr`, make room for all its locals, point `r7` at that area. It uses whatever `sp` is at that moment: the ROM's `0x341037a0`. These are the 24 bytes the first `next` showed.
2. **Locals are reached through `r7`**: `src` is `[r7, #12]` = `0x34103794`, `dst` is `[r7, #8]` = `0x34103790`.
3. **`MSR msp` only changes `sp`.** For the compiler, `__set_MSP` is a piece of inline assembly it does not look into; it does not know the stack moved. `r7` keeps pointing at the frame on the ROM's stack. Only new pushes (the call to `SystemInit`) use my stack.

The linker script only puts the stack address in word 0 of the vector table; something still has to load it into `sp`:

- **Real reset into my program**: the hardware loads `sp` from word 0 before the first instruction of Reset_Handler. The prologue then builds the frame on my stack, and `__set_MSP` writes the same value again.
- **Dev boot with GDB `load`**: no reset. `load` sets only `pc`, so Reset_Handler starts on whatever stack the ROM left.

It works, because the boot ROM no longer runs and nothing else uses that memory. Ways to avoid it:

- In GDB, after `load`: `set $sp = &_estack`, doing by hand what a reset would do.
- In the code:
  - the first part in **assembly**, as ST does: no prologue, `sp` is set before anything uses the stack
  - `__attribute__((naked))` on Reset_Handler: no prologue, but the function may only contain inline assembly
  - Reset_Handler only sets `MSPLIM` and `MSP`, then calls a **second** C function for the rest, whose prologue runs on the new stack

> [!note] `$1`, `$2`
> GDB keeps every `print` result in a numbered value history; `$1` can be used in later expressions.

## In main

`break main` sets a breakpoint by function name. My code is in RAM, so GDB can use a **software breakpoint**: it writes a `BKPT` instruction into memory. In the ROM ([04](<04 - Exploring with GDB.md#Breakpoints in ROM>)) that was not possible and I needed `hbreak`.

```
(gdb) break main
Breakpoint 1 at 0x34064046: file src/main.c, line 2.
(gdb) continue
Continuing.

Breakpoint 1, main () at src/main.c:2
2        volatile int counter = 0;
(gdb) backtrace
#0  main () at src/main.c:2
(gdb) info registers sp r7
sp             0x34083ff0          0x34083ff0
r7             0x34083ff0          872955888
(gdb) print &counter
$3 = (volatile int *) 0x34083ff4
```

- The breakpoint is at `0x34064046`, not at `main` (`0x34064040`): GDB skips the prologue and stops at the first line of C.
- `continue` ran the rest of Reset_Handler: the copy and zero loops (zero passes) and the call to `main`.
- `main`'s prologue ran **after** the stack switch, so its frame is on **my** stack: `push {r7}` (4 bytes) + `sub sp, #12` = 16 bytes below `_estack`, so `sp` = `r7` = `0x34083ff0`.
- `counter` is at `r7 + 4` = `0x34083ff4`, the `str r3, [r7, #4]` from the disassembly in 09.

### next in an endless loop

```
(gdb) next
5            counter++;
(gdb) next
^C
Program received signal SIGINT, Interrupt.
main () at src/main.c:5
5            counter++;
(gdb) print counter
$4 = 71
```

The second `next` never returns. `next` steps until the `pc` reaches the start of a **different** line, but `while (1) { counter++; }` jumps back into line 5 forever. Ctrl-C stops it.

`counter` is only 71: `next` works by single-stepping instructions one at a time through the debugger, which is very slow. In an endless loop, `stepi` or a breakpoint is the way to move.

### Running freely

```
(gdb) continue
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x3406404c in main () at src/main.c:5
5            counter++;
(gdb) print counter
$5 = 7271331
```

Without the debugger stepping each instruction, the core runs at full speed: millions of passes in the time it took to press Ctrl-C.

### Who called main

`backtrace` (`bt`) earlier showed only `main`. The return address is there:

```
(gdb) info registers lr
lr             0x340640bb          872825019
```

`lr` points back into Reset_Handler, to the instruction after `bl main` (`0x340640ba`, plus the Thumb bit). GDB simply stops a backtrace at `main` by default: on a PC, what calls `main` is C library start code. On the MCU it is my own code:

```
(gdb) set backtrace past-main on
(gdb) backtrace
#0  0x3406404c in main () at src/main.c:5
#1  0x340640ba in Reset_Handler () at src/startup_stm32n657xx.c:91
```

## Commands so far

New in this note (the ones from [04](<04 - Exploring with GDB.md#Commands so far>) still apply):

| Command                        | What it does                                                   |
| ------------------------------ | -------------------------------------------------------------- |
| `gdb-multiarch <file>.elf`     | start GDB with my program's symbols and debug information      |
| `load`                         | write the program's sections to RAM and set `pc` to the entry point |
| `x/<n>a <address>`             | examine memory as addresses, with symbol names                 |
| `list <function>`              | show the source                                                |
| `next` (`n`) / `step` (`s`)    | run one C line, over / into function calls                     |
| `print <expression>` (`p`)     | evaluate C: a variable, `&variable`, …                         |
| `break <function>`             | software breakpoint (code in RAM)                              |
| `backtrace` (`bt`)             | the chain of calls that led here                               |
| `set backtrace past-main on`   | show the callers of `main` too                                 |
| `set $sp = <value>`            | write a register                                               |

