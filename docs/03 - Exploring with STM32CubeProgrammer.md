With the board in dev boot and the connection working, I use the STM32CubeProgrammer CLI to explore the board by hand before writing any code: read registers, compare them with the reference manual, and change a few.

References: [STM32CubeProgrammer command lines](https://dev.st.com/stm32cube-docs/prog/2.23.0/en/docs/markup/CubeProg_Command_Lines.html), [RM0486](https://www.st.com/resource/en/reference_manual/dm00769900.pdf), [UM3417](https://www.st.com/resource/en/user_manual/um3417-stm32n6-nucleo144-board-mb1940-stmicroelectronics.pdf).

## Basic commands

Every call starts with `-c` to connect, followed by any number of commands. They run in order in one connection:

```
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -r32 <address> <bytes> -w32 <address> <value>
```

The few I use here:

| Command                    | What it does                                                                            |
| -------------------------- | --------------------------------------------------------------------------------------- |
| `-c port=SWD`              | connect through the debug port (`port=/dev/ttyACM0` for the boot ROM's UART bootloader) |
| `mode=HOTPLUG`             | attach without resetting or halting the core, so values written earlier are still there |
| `-r32 <address> <bytes>`   | read memory as 32-bit words; `4` bytes = one register                                   |
| `-w32 <address> <value>`   | write one 32-bit word; it replaces the whole register                                   |
| `-score`                   | show the core state (running, halted, …)                                                |
| `-halt` / `-run` / `-step` | stop the core, let it run, execute one instruction                                      |
| `-coreReg`                 | read the core registers (R0–R12, SP, LR, PC, xPSR); `-coreReg PC=<value>` writes one    |

> [!note]
> Each call is a new connection. With the default `mode=NORMAL` the connection can reset the chip, which undoes earlier writes; `HOTPLUG` or chaining commands in one call avoids that.

The full list is in the command-line reference above.

## First look at the core and memory

### Core state

In dev boot nothing runs, but the core is still alive. I check its state, stop it, look at its registers and let it go:

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -score
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -halt -coreReg -run
```

```
R0       = 0x15F951E1
R1       = 0x60B166E7
R2       = 0x00000000
R3       = 0x00000000
R4       = 0x00000001
R5       = 0x00000001
R6       = 0x00000001
R7       = 0x180050FD
R8       = 0x18005ABC
R9       = 0x00000000
R10      = 0x00000000
R11      = 0x00000000
R12      = 0x00000000
SP       = 0x341037A0
LR       = 0x180005F5
PC       = 0x18003A1A
XPSR     = 0x01000000
MSP      = 0x341037A0
PSP      = 0x341037F0
PRIMASK  = 0x00
FAULTMASK= 0x00
BASEPRI  = 0x00
CONTROL  = 0x00
```

What the registers are:

- **R0–R12**: general-purpose registers; they hold whatever the running code was working on.
- **SP** (stack pointer): top of the current stack. There are two, **MSP** (main) and **PSP** (process); after reset the core uses MSP.
- **LR** (link register): the address to return to when the current function ends.
- **PC** (program counter): the address of the instruction being executed.
- **XPSR**: status flags; bit 24 (`0x01000000`) is the Thumb bit, always 1 on Cortex-M.
- **PRIMASK, FAULTMASK, BASEPRI**: interrupt masking; all 0 means nothing is masked.
- **CONTROL**: which stack is in use and whether code is privileged; 0 means MSP and privileged.

`PC` and `LR` are in `0x18000000`, the boot ROM, and `SP` is in AXISRAM2, the boot ROM's own stack. So the core is in the boot ROM, which in dev boot just waits for the debugger.

### Reading a register

The FLEXMEM configuration (how much of the RAM is used as TCM) depends on `CFGITCMSZ[3:0]` and `CFGDTCMSZ[3:0]` in `SYSCFG_CM55TCMCR`. From the reference manual:

- SYSCFG base: `0x56008000` (secure)
- `SYSCFG_CM55TCMCR` offset `0x008`, reset value `0x0000 0087`

So the register is at `0x56008008`:

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -r32 0x56008008 4
```

```
Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x56008008
0x56008008 : 00000087
```

`0x87` splits into `CFGITCMSZ` = bits 3:0 = `7` and `CFGDTCMSZ` = bits 7:4 = `8`:

> [!quote] RM0486, SYSCFG_CM55TCMCR
> **Bits 7:4 CFGDTCMSZ[3:0]:** Select DTCM memory size
> 0x8: 128 Kbytes (default value)
>
> **Bits 3:0 CFGITCMSZ[3:0]:** Select ITCM memory size
> 0x7: 64 Kbytes (default value)

### Writing and reading RAM

Nothing is loaded yet, so the RAM is free to use. I pick an address in AXISRAM1, `0x34090000`, read it, write a value, and read it back:

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -r32 0x34090000 4 -w32 0x34090000 0xCAFEBABE -r32 0x34090000 4
```

```
Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x34090000
0x34090000 : 00000000

Write 32-bit data
w @0x34090000:  0xcafebabe

Downloading...
================================================== 100%
32-bit data download complete

Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x34090000
0x34090000 : CAFEBABE
```

I start with trying uart, /dev/ttyACM0. running score there i see Warning: score command is not supported with this port: /dev/ttyACM0
which means that for more debug commands i will need the swd. continue with reading. i could not read some memory area and its contents. i will switch to swd interface with debugging.

## Turning on the blue LED by hand

### Finding the pin

The board manual (UM3417) lists the user LEDs and the MCU pins they are connected to:

- Red LD5: PG10
- Green: PG0
- Blue: PG8

`PG8` is a pin name: **P**ort **G**, pin **8**. Every pin named `Pxn` belongs to a GPIO port, so the next place to look is the GPIO chapter of the reference manual. The datasheet would only matter if I wanted the pin to do something other than plain GPIO (UART, SPI, timer, …).

The manual also says how they are driven:

> [!quote] UM3417, user LEDs
> The three following user LEDs (LD5, LD6, and LD7) are OFF when the I/O is in the HIGH state, and are ON when the I/O is in the LOW state.

So the LEDs are **active-low**: each one sits between VDD and its pin, and lights when the pin pulls low. This is fixed by the board wiring; no GPIO register can invert it.

### What the GPIO port offers

From the GPIO chapter:

> [!quote] RM0486, GPIO
> Each general-purpose I/O port has four 32-bit configuration registers (GPIOx_MODER, GPIOx_OTYPER, GPIOx_OSPEEDR, and GPIOx_PUPDR), two 32-bit data registers (GPIOx_IDR and GPIOx_ODR), a 16-bit reset register (GPIOx_BRR), and a 32-bit set/reset register (GPIOx_BSRR).
> In addition, all GPIOs have a 32-bit locking register (GPIOx_LCKR), two couples of 32-bit advanced configuration registers (GPIOx_DELAYRL/H, GPIOx_ADVCFGRL/H), and two 32-bit alternate function selection registers (GPIOx_AFRH and GPIOx_AFRL).

To light an LED I need two of them:

| Register      | Offset | What I use it for                                                                                     |
| ------------- | ------ | ----------------------------------------------------------------------------------------------------- |
| `GPIOx_MODER` | `0x00` | make the pin an output: 2 bits per pin, `MODE8` = bits 17:16, `01` = output, reset value `11` (analog) |
| `GPIOx_BSRR`  | `0x18` | drive the pin: bit 8 (`BS8`) sets PG8 high, bit 24 (`BR8`) sets it low                                 |
| `GPIOx_ODR`   | `0x14` | read back the level the pin is driven to                                                              |

`ODR` can also drive the pin, but a write replaces all 16 pins of the port. `BSRR` only touches the bits written as 1, so one write changes PG8 and nothing else.

### Where GPIOG is

The memory map table in the reference manual ("Peripheral register boundary addresses") gives the GPIOG base and the bus it sits on:

- `0x56021800` (secure), `0x46021800` (non-secure)
- bus: **AHB4**

The full address of a register is base + offset, so `GPIOG_MODER` is at `0x56021800`.

### First read: the port is off

For ports other than A and B, `MODER` resets to `0xFFFFFFFF` (all pins analog). I read it expecting that value:

```sh
STM32_Programmer_CLI -c port=SWD -r32 0x56021800 4
```

```
0x56021800 : 00000000
```

It reads `0`. At reset almost every peripheral's **clock is off** to save power, and a peripheral without a clock does not respond: writes are ignored and reads return 0. The clocks are switched on in **RCC** (Reset and Clock Control).

### Enabling the clock in RCC

Each bus has an enable register in RCC with one bit per peripheral. GPIOG is on AHB4, so the bit is `GPIOGEN` in the AHB4 enable registers. On the N6 there are three of them:

| Register       | Offset   | Write behaviour                                 |
| -------------- | -------- | ----------------------------------------------- |
| `RCC_AHB4ENR`  | `0x25C`  | read/write; a write replaces all bits           |
| `RCC_AHB4ENSR` | `0xA5C`  | writing 1 turns that clock on, 0 has no effect  |
| `RCC_AHB4ENCR` | `0x125C` | writing 1 turns that clock off, 0 has no effect |

With the CLI, `-w32` on `ENR` would also switch off any clock that was already on, unless I read it first and add my bit. `ENSR` changes only the bit I write, so I write there and read `ENR` to check.

- RCC base: `0x56028000` (secure)
- `RCC_AHB4ENSR` = `0x56028A5C`, `RCC_AHB4ENR` = `0x5602825C`
- `GPIOGEN` is bit 6: `0x00000040`

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -w32 0x56028A5C 0x00000040 -r32 0x5602825C 4 -r32 0x56021800 4
```

```
Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x5602825C
0x5602825C : 00040040

Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x56021800
0x56021800 : FFDFFFFF
```

> [!note]
> The write reports `Error: Failed to download data!`, but it worked. After a write, the CLI reads the address back to check it. A set register like `ENSR` does not read back what was written, so the check fails. Reading `ENR` shows the real state.

`RCC_AHB4ENR` now has bit 6 (`GPIOGEN`) set; bit 18 (`PWREN`) was already on. With the clock on, `GPIOG_MODER` responds, but with `0xFFDFFFFF` instead of the reset value `0xFFFFFFFF`: bit 21 is 0, so `MODE10` = `01` and **PG10, the red LED, is an output**. The boot ROM sets this up in dev boot, which is why the red LED is on when I plug in the board. See [The red LED at power-up](#The%20red%20LED%20at%20power-up).

### PG8 as output

`MODE8` is bits 17:16 and must go from `11` to `01`, so only bit 17 is cleared. `MODER` has no set/clear version, so I write the whole value. I write `0xFFFDFFFF`, which also puts PG10 back to analog, so the red LED goes off (to keep it, the value would be `0xFFDDFFFF`):

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -w32 0x56021800 0xFFFDFFFF -r32 0x56021800 4
```

```
Write 32-bit data
w @0x56021800:  0xfffdffff

Downloading...
[==================================================] 100%
32-bit data download complete

Reading 32-bit memory content
  Size          : 4 Bytes
  Address:      : 0x56021800
0x56021800 : FFFDFFFF
```

`MODER` is a normal read/write register, so here the CLI check passes.

### Driving PG8

`GPIOG_BSRR` is at `0x56021818`:

- `0x00000100` (bit 8, `BS8`): PG8 high, LED **off**
- `0x01000000` (bit 24, `BR8`): PG8 low, LED **on**

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -w32 0x56021818 0x00000100 -r32 0x56021814 4
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -w32 0x56021818 0x01000000 -r32 0x56021814 4
```

`BSRR` is write-only too, so the same `Failed to download data` error appears; `ODR` at offset `0x14` shows whether bit 8 changed.

## The red LED at power-up

The red LED is on every time I power the board in dev boot, before I connect, and `GPIOG_MODER` shows PG10 as output. The only code that runs before I connect is the boot ROM, so I look at it.

### Reading the ROM

The CMSIS header gives the boot ROM at `0x18000000` (secure), 128 KB. It is readable over SWD:

```sh
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -r32 0x18000000 0x100
```

```
0x18000000 : 341037F0 180020C5 18001B19 18000D0D
0x18000010 : 18001B01 18000C35 180025ED 00000000
0x18000020 : 00000000 00000000 00000000 18002519
0x18000030 : 18000CC5 00000000 18001B31 180025D5
0x18000040 : 18000D25 18000D25 18000D25 18000D25
...
```

It starts with a vector table, the table every Cortex-M program begins with:

| Address        | Value        | Entry                                                    |
| -------------- | ------------ | -------------------------------------------------------- |
| `0x18000000`   | `0x341037F0` | initial stack pointer, in AXISRAM2                       |
| `0x18000004`   | `0x180020C5` | Reset_Handler at `0x180020C4` (last bit = Thumb bit)     |
| `0x18000008`–  | …            | NMI, HardFault and the other fault handlers              |
| `0x18000040`+  | `0x18000D25` | every peripheral interrupt goes to one default handler   |

### Disassembling it

I dump the whole ROM on the host and disassemble it in the Docker toolchain:

```sh
# host
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -u 0x18000000 0x20000 rom.bin
# container
arm-none-eabi-objdump -D -b binary -m arm -M force-thumb --adjust-vma=0x18000000 rom.bin > rom.txt
```

There are no symbols, so I search the binary for the addresses I know: `0x56021800` (GPIOG) and `0x56028000` (RCC). The compiler keeps them as constants next to the code that uses them.

### What the ROM does with PG10

The function at `0x18003548` loads `0x56021800` (GPIOG) and computes RCC as GPIOG + `0x6800` = `0x56028000`. Then:

1. reads `RCC_AHB4ENR` bit 6 (`GPIOGEN`); if it is 0, sets it and waits until it reads 1
2. `MODER`: clears bits 21:20, sets bit 20, so `MODE10` = `01` (output)
3. `OTYPER` (`0x04`): sets bit 10, PG10 **open-drain**
4. `OSPEEDR` (`0x08`): bits 21:20 = `11`, very high speed
5. `PUPDR` (`0x0C`): bits 21:20 = `00`, no pull
6. writes `0x400` to the low half of `BSRR` (`BS10`, high) or the high half (`BR10`, low), depending on its argument

The same steps I did by hand for PG8. Open-drain fits an active-low LED: the pin only ever needs to pull low.

A second routine (`0x18004d5c`) enables the clock, clears bit 10 in `GPIOG_SECCFGR` (`0x30`) so PG10 is non-secure, and then clears `GPIOGEN` in `RCC_AHB4ENR` again. A pin keeps its configuration without a clock, so the LED stays on while the registers read `0` until I enable the clock myself.

> [!note]
> This is the ROM's own behaviour, not a fault: the ROM is read-only and runs the same way on every power-up. With GDB I followed it live: in dev boot the ROM toggles PG10 on every pass of its waiting loop, too fast to see, so the LED looks on. See [Catching the ROM driving the red LED](<04 - Exploring with GDB.md#Catching the ROM driving the red LED>).

