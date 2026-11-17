In [03 - Exploring with STM32CubeProgrammer](<03 - Exploring with STM32CubeProgrammer.md>) I talked to the board one command at a time. Here I do the same kind of exploring with GDB, which keeps one connection open and lets me stop the core, step through code and inspect memory interactively.

References: [OpenOCD user guide](https://openocd.org/doc/html/index.html), [GDB documentation](https://sourceware.org/gdb/current/onlinedocs/gdb/).

## How the pieces connect

Two programs run at the same time, both in the Docker container:

- **OpenOCD** talks to the ST-LINK over USB and opens a **GDB server** on port `3333`.
- **GDB** (`gdb-multiarch`) connects to that port; this is where I type commands.

```
GDB  ──TCP :3333──►  OpenOCD  ──USB──►  ST-LINK  ──SWD──►  STM32N6
```

Only one program can use the ST-LINK at a time, so STM32CubeProgrammer must not be connected.

## Starting OpenOCD

Terminal 1, from the project folder. `--name` gives the container a fixed name so a second terminal can join it:

```sh
sudo systemctl start docker
docker compose run --rm --name stm32-gdb stm32-dev
```

Inside the container:

```sh
openocd
```

With no arguments OpenOCD reads `openocd.cfg` in the current folder, which selects the ST-LINK and the STM32N6 target. It keeps running and waits for GDB:

```
Info : STLINK V3J15M6 (API v3) VID:PID 0483:3754
Info : Target voltage: 3.273307
Info : clock speed 8000 kHz
Info : SWD DPIDR 0x6ba02477
Info : [stm32n6x.ap0] Examination succeed
Info : [stm32n6x.cpu] Cortex-M55 r1p1 processor detected
Info : [stm32n6x.cpu] target has 8 breakpoints, 8 watchpoints
stm32n6x.cpu in Secure state
Info : [stm32n6x.cpu] Examination succeed
Info : [stm32n6x.cpu] starting gdb server on 3333
Info : Listening on port 3333 for gdb connections
```

What it found:

- the ST-LINK and the board voltage (3.27 V)
- the debug port (`DPIDR`) and the core: Cortex-M55, in **Secure** state, as after the boot ROM
- **8 hardware breakpoints and 8 watchpoints**: the limit for code in ROM or for watching memory
- the GDB server on `3333`; `4444` (telnet) and `6666` (tcl) are other ways to send OpenOCD commands

## Connecting GDB

Terminal 2 joins the same container and starts GDB:

```sh
docker exec -it stm32-gdb bash
gdb-multiarch
```

At the `(gdb)` prompt I connect to OpenOCD:

```
(gdb) target extended-remote :3333
Remote debugging using :3333
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x18003514 in ?? ()
```

OpenOCD, in terminal 1, shows the other side:

```
Info : accepting 'gdb' connection on tcp/3333
[stm32n6x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x21000000 pc: 0x18003514 msp: 0x341037a8
Info : New GDB Connection: 1, Target stm32n6x.cpu, state: halted
```

- Connecting **halts the core** (`halted due to debug-request`); it stays stopped until I tell it to continue.
- The warning and `?? ()` mean GDB has no ELF file, so it has no symbols or source: it only knows addresses. Fine for now; there is no program of mine yet.
- `pc = 0x18003514` is in the boot ROM, inside the loop at `0x1800350c`–`0x1800353e` that drives the red LED (see [The red LED at power-up](<03 - Exploring with STM32CubeProgrammer.md#The red LED at power-up>)). In dev boot, this loop is where the ROM waits.

## Looking around

Three commands do what `-coreReg` and `-r32` did in 03, without reconnecting each time.

> [!tip]
> `info registers` fills more than one screen and GDB stops with `--Type <RET> for more`. Running `set pagination off` once per session turns that off.

### Registers

```
(gdb) info registers
r0             0x2a                42
r1             0x0                 0
r2             0xc8e7c3da          -924335142
r3             0xcafedead          -889266515
r4             0x0                 0
r5             0x64                100
r6             0x0                 0
r7             0x1                 1
r8             0x1                 1
r9             0x400               1024
r10            0x34102040          873472064
r11            0x56021800          1442977792
r12            0x0                 0
sp             0x341037a8          0x341037a8
lr             0x18003521          402666785
pc             0x18003514          0x18003514
xpsr           0x21000000          553648128
fpscr          0x50440000          1346633728
msp            0x341037a8          0x341037a8
psp            0x341037f0          0x341037f0
vpr            0xc1d952            [ P0=55634 MASK01=1 MASK23=12 ]
msp_ns         0x241077f0          0x241077f0
psp_ns         0x241077f0          0x241077f0
msp_s          0x341037a8          0x341037a8
psp_s          0x341037f0          0x341037f0
primask        0x0                 0
basepri        0x0                 0
faultmask      0x0                 0
control        0x0                 0
msplim_s       0x34102ff0          0x34102ff0
psplim_s       0x34102ff0          0x34102ff0
msplim_ns      0x24106ff0          0x24106ff0
psplim_ns      0x24106ff0          0x24106ff0
primask_s      0x0                 0
basepri_s      0x0                 0
faultmask_s    0x0                 0
control_s      0x0                 0
primask_ns     0x0                 0
basepri_ns     0x0                 0
faultmask_ns   0x0                 0
control_ns     0x0                 0
p0             0xd952              -9902
```

GDB shows more than CubeProgrammer did:

- **`fpscr`**: status and control of the floating-point unit.
- **`vpr`, `p0`**: predicate registers of **Helium (MVE)**, the M55's vector extension.
- **`_s` / `_ns` pairs** (`msp`, `psp`, `control`, …): with **TrustZone** the core keeps one copy for the secure world and one for the non-secure. `msp` and `psp` show the copy in use, here the secure one (`msp` = `msp_s`).
- **`msplim`, `psplim`**: ARMv8-M **stack limits**. If the stack grows below them, the core faults instead of overwriting other memory.

The values match the LED loop in the ROM: `r11` = `0x56021800` is the GPIOG base, `r9` = `0x400` is the PG10 bit for `BSRR`, and `lr` = `0x18003521` is the return address of the `bl 0x18005984` inside the loop.

### Memory

`x` = e**x**amine. `x/4xw 0x18000000` reads 4 words, shown in hex, starting at the ROM's vector table; the same values as `-r32` in 03:

```
(gdb) x/4xw 0x18000000
0x18000000:    0x341037f0    0x180020c5    0x18001b19    0x18000d0d
```

### Instructions

`x/8i $pc` shows the next 8 instructions from the program counter. GDB disassembles straight from memory, so no `rom.txt` is needed:

```
(gdb) x/8i $pc
=> 0x18003514:    eor.w    r1, r1, #1
   0x18003518:    str    r1, [sp, #4]
   0x1800351a:    b.n    0x1800350e
   0x1800351c:    bl    0x18005984
   0x18003520:    cbz    r5, 0x1800353a
   0x18003522:    bic.w    r6, r7, r6
   0x18003526:    cmp.w    r8, #0
   0x1800352a:    mov    r0, r11
```

The core stopped in a short delay loop inside the LED loop: count `r0` down to 0, flipping a value on the stack each pass. `r0` = `42`, so 42 passes are left before the `bl` at `0x1800351c`.

## Stepping, running and stopping

`stepi` (`si`) runs **one instruction** and stops. `info registers r0 pc` shows only the registers I name:

```
0x18003514 in ?? ()
(gdb) stepi
0x18003518 in ?? ()
(gdb) info registers r0 pc
r0             0x37                55
pc             0x18003518          0x18003518
(gdb) stepi
0x1800351a in ?? ()
(gdb) si
0x1800350e in ?? ()
(gdb) si
0x18003510 in ?? ()
(gdb) info registers r0 pc
r0             0x37                55
pc             0x18003510          0x18003510
(gdb) si
0x18003512 in ?? ()
(gdb) si
0x18003514 in ?? ()
(gdb) info registers r0 pc
r0             0x36                54
pc             0x18003514          0x18003514
```

- The `b.n` at `0x1800351a` jumps back to the top of the loop at `0x1800350e`.
- `r0` drops from 55 to 54 only after the `subs r0, #1` at `0x18003512` has run. The `pc` always points at the **next** instruction to execute, not the last one.
- `r0` is 55 here, not 42 as before: the ROM runs this delay over and over, so each stop lands on a different pass.

`continue` (`c`) lets the core run. GDB waits until I press **Ctrl-C**, which halts the core again:

```
(gdb) continue
Continuing.
^C
Program received signal SIGINT, Interrupt.
0x18003514 in ?? ()
```

It stops in the same delay loop, which is where the ROM spends almost all its time.

## Writing memory and OpenOCD commands

`set {int}<address> = <value>` writes a 32-bit word, like `-w32` in 03, and `x` reads it back:

```
(gdb) set {int}0x34090000 = 0x12345678
(gdb) x/xw 0x34090000
0x34090000:    0x12345678
```

`monitor <command>` sends the rest of the line to **OpenOCD** instead of GDB. `mdw` is OpenOCD's "memory display word"; here it reads `SYSCFG_CM55TCMCR` again, with the same `0x87` as in 03:

```
(gdb) monitor mdw 0x56008008
0x56008008: 00000087
```

## Reset

`monitor reset halt` resets the chip and halts the core:

```
(gdb) monitor reset halt
[stm32n6x.cpu] halted due to debug-request, current mode: Thread
xPSR: 0x81000000 pc: 0x18003a1a msp: 0x341037a0
(gdb) info registers pc sp
pc             0x18003514          0x18003514
sp             0x341037a8          0x341037a8
(gdb) maintenance flush register-cache
Register cache flushed.
(gdb) info registers pc sp
pc             0x18003a1a          0x18003a1a
sp             0x341037a0          0x341037a0
```

- The core does **not** stop at the ROM's Reset_Handler (`0x180020c4`) but at `0x18003a1a`, the same address `-coreReg` showed in 03 after a connect with reset. In dev boot the boot ROM decides when the debugger gets access, so the first possible halt is already inside the ROM. (Not verified, but it fits: the stack has also moved from `0x341037f0`.)
- The first `info registers` still shows the old `pc`. `monitor` commands bypass GDB, so GDB does not know the core was reset and shows its **cached** registers. `maintenance flush register-cache` makes it read them again.

> [!warning]
> After a `monitor` command that changes the core (reset, halt, register write), GDB's view can be stale until the register cache is flushed.

## Catching the ROM driving the red LED

After `monitor reset halt` the red LED is **off**: the reset put GPIOG back to its reset values (PG10 analog), and the ROM has not reached the LED code yet. The core is halted, so I can let it run up to that code and watch it happen.

### Breakpoints in ROM

A normal `break` makes GDB write a breakpoint instruction into memory. ROM cannot be written, so I use **hardware breakpoints**: the core itself compares the `pc` with up to 8 addresses and halts on a match.

| Command               | What it does                                                        |
| --------------------- | ------------------------------------------------------------------- |
| `hbreak *<address>`   | hardware breakpoint at an address (`*` = address, not a function name) |
| `info breakpoints`    | list the breakpoints with their numbers                             |
| `delete [n]`          | delete breakpoint `n`, or all of them without a number              |
| `continue` (`c`)      | run until a breakpoint is hit (or Ctrl-C)                           |
| `si`                  | execute one instruction                                             |

I set one on each place in the ROM that configures PG10 (found in [03](<03 - Exploring with STM32CubeProgrammer.md#The red LED at power-up>)) and let the core run:

```
(gdb) hbreak *0x18003548
Hardware assisted breakpoint 1 at 0x18003548
(gdb) hbreak *0x180034a6
Hardware assisted breakpoint 2 at 0x180034a6
(gdb) hbreak *0x18004d5c
Hardware assisted breakpoint 3 at 0x18004d5c
(gdb) continue
Continuing.

Breakpoint 2, 0x180034a6 in ?? ()
```

Breakpoint 2 is hit first: in dev boot the ROM configures PG10 **inline** in the caller, not through the function at `0x18003548`.

### The setup code

```
(gdb) x/12i $pc
=> 0x180034a6:    add.w    r0, r4, #26624    @ 0x6800
   0x180034aa:    bl    0x1800128c
   0x180034ae:    ldr    r0, [r4, #0]
   0x180034b0:    bic.w    r0, r0, #3145728    @ 0x300000
   0x180034b4:    str    r0, [r4, #0]
   0x180034b6:    ldr    r0, [r4, #0]
   0x180034b8:    orr.w    r0, r0, #1048576    @ 0x100000
   0x180034bc:    str    r0, [r4, #0]
   0x180034be:    ldr    r0, [r4, #4]
   0x180034c0:    bic.w    r0, r0, #1024    @ 0x400
   0x180034c4:    str    r0, [r4, #4]
   0x180034c6:    ldr    r0, [r4, #4]
```

`r4` holds `0x56021800` (GPIOG), so `r4 + 0x6800` is RCC, and the `bl` turns on the GPIOG clock. Then the same register writes as in 03: `MODER`, `OTYPER`, `OSPEEDR`, `PUPDR`.

At the end of the setup, `0x180034ea` writes `0x400` to `BSRR` + 24, which is `BS10`: PG10 **high**. I stop there and step over it:

```
hbreak *0x180034ea
continue
si
```

The LED stays **off**: high = off for these active-low LEDs.

### The loop that lights it

The LED is switched in the endless loop that follows the setup. Each pass it writes the opposite half of `BSRR`:

- `0x1800352e`: `strh.w r9, [fp, #24]`, `BS10`, LED off
- `0x18003536`: `strh.w r9, [fp, #26]`, `BR10`, LED **on**

With only 8 hardware breakpoints, I delete the old ones first:

```
delete
hbreak *0x1800352e
hbreak *0x18003536
continue
si
continue
si
```

Each `continue` stops at the other address, and after each `si` the LED changes: **on** after `0x18003536`, **off** after `0x1800352e`.

So the red LED is not steadily on in dev boot: the ROM **toggles it on every pass of its waiting loop**, faster than the eye can see, so it looks on.

## Leaving

```
continue
```

Ctrl-C, then:

```
delete
detach
quit
```

- `delete` removes the breakpoints, so the core does not stop on them after GDB is gone.
- `detach` disconnects GDB; the core keeps running (that is why `continue` comes first: without it the core would stay halted).
- `quit` exits GDB.

OpenOCD stops with Ctrl-C in terminal 1, and `exit` leaves the container (`--rm` deletes it).

## Commands so far

| Command                              | What it does                                           |
| ------------------------------------ | ------------------------------------------------------ |
| `target extended-remote :3333`       | connect to OpenOCD (halts the core)                    |
| `set pagination off`                 | no `--Type <RET>` prompts                              |
| `info registers [reg …]`             | core registers, all or only the ones named             |
| `x/<n><format><size> <address>`      | examine memory, e.g. `x/4xw` words in hex, `x/8i` instructions |
| `set {int}<address> = <value>`       | write a 32-bit word                                    |
| `stepi` (`si`)                       | execute one instruction                                |
| `continue` (`c`) / Ctrl-C            | run / halt                                             |
| `hbreak *<address>`                  | hardware breakpoint (needed for ROM)                   |
| `info breakpoints` / `delete [n]`    | list / delete breakpoints                              |
| `monitor <command>`                  | send a command to OpenOCD, e.g. `mdw`, `reset halt`    |
| `maintenance flush register-cache`   | re-read registers after a `monitor` command            |
| `detach` / `quit`                    | disconnect, leaving the core running / exit GDB        |
