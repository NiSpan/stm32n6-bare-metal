Now that we can create and load a .c program on our board, we will create a simple program to blink the 3 user LEDs simultaneously with a small delay between on and off so we can see it. 

As we discovered in lesson 03 and 04, the user LEDs are connected to the MCU through PG0, PG8 and PG10. That means that they are at Pin(P) GPIO G(G) 0, 8 and 10 respectively.

On the board the LEDs are **active low**: the LED turns on when the pin is driven to 0 ([UM3417](https://www.st.com/resource/en/user_manual/um3417-stm32n6-nucleo144-board-mb1940-stmicroelectronics.pdf), LEDs section).

To drive a pin I need three things, all from the reference manual [RM0486](https://www.st.com/resource/en/reference_manual/dm00769900.pdf):

1. Turn on the clock of GPIO port G (RCC).
2. Set PG0, PG8, PG10 as outputs (GPIOG_MODER).
3. Drive the pins high or low (GPIOG_BSRR).

## Finding the addresses

Each register address is a **base** (from the memory map) plus an **offset** (from the register description of that peripheral). I use the secure addresses (`0x5...`), because in dev boot the core runs in secure state ([02](<02 - The boot process.md>)).

| Peripheral | Base | Source |
|---|---|---|
| AHB4 peripherals | `0x5602 0000` | RM0486 memory map |
| GPIOG | `0x5602 1800` | AHB4 + `0x1800` |
| RCC | `0x5602 8000` | AHB4 + `0x8000` |

The same values are in the CMSIS header `stm32n657xx.h` (`AHB4PERIPH_BASE_S`, `GPIOG_BASE_S`, `RCC_BASE_S`), which is a quick way to check.

## Clock: RCC_AHB4ENR

After reset most peripheral clocks are off, and a GPIO port without a clock ignores writes. GPIOG sits on the AHB4 bus, so its clock bit is in `RCC_AHB4ENR`:

- Offset `0x25C`, so the address is `0x5602 8000 + 0x25C = 0x5602 825C`.
- Bit 6 is `GPIOGEN`.
- Its value after reset is `0x0004 0000`: bit 18 (`PWREN`) is already on.

I write `0x0004 0040`: the reset value plus bit 6.

## Pin mode: GPIOG_MODER

`MODER` is at offset `0x00`, so its address is `0x5602 1800`. It has 2 bits per pin, pin n at bits `2n+1:2n`:

| Value | Mode |
|---|---|
| `00` | input |
| `01` | general purpose output |
| `10` | alternate function |
| `11` | analog (reset value) |

All pins start as analog (`0xFFFF FFFF`). I clear the high bit of each LED pin so it becomes `01`:

| Pin | Bits | Value |
|---|---|---|
| PG0 | 1:0 | `01` |
| PG8 | 17:16 | `01` |
| PG10 | 21:20 | `01` |

`0xFFFF FFFF` with bits 1, 17 and 21 cleared is `0xFFDD FFFD`.

## Driving the pins: GPIOG_BSRR

`BSRR` is at offset `0x18`, so its address is `0x5602 1818`. It is write only, and only the bits written as 1 do something:

- Bits 15:0 (**set**): a 1 drives pin n **high**.
- Bits 31:16 (**reset**): a 1 at bit n+16 drives pin n **low**.

So one write changes only the pins I name, and the other pins of port G are left alone. With the LEDs active low:

| Write | Bits | Pins | LEDs |
|---|---|---|---|
| `0x0000 0501` | 0, 8, 10 | high | off |
| `0x0501 0000` | 16, 24, 26 | low | on |

## The program

```c
#define RCC_AHB4_ENABLE_REGISTER (unsigned int *)0x5602825CUL
#define GPIOG_MODE_REGISTER (unsigned int *)0x56021800
#define GPIOG_SET_RESET_REGISTER (unsigned int *)0x56021818

int counter = 0;

int main(void) {
    unsigned int *RCC = RCC_AHB4_ENABLE_REGISTER;
    unsigned int *GPIOG = GPIOG_MODE_REGISTER;
    unsigned int *PIN_SET = GPIOG_SET_RESET_REGISTER;

    *RCC = 0x00040040;        // GPIOG clock on
    *GPIOG = 0xFFDDFFFD;      // PG0, PG8, PG10 as outputs

    while (1) {
        *PIN_SET = 0x00000501;    // pins high: LEDs off
        while (counter < 50000) {
            counter++;
        }
        counter = 0;
        *PIN_SET = 0x05010000;    // pins low: LEDs on
        while (counter < 50000) {
            counter++;
        }
        counter = 0;
    }

    return 0;
}
```

The delay is a busy loop: the CPU counts to 50000 and does nothing else. It is not an exact time, it depends on the clock and the optimisation level.

I build it with CMake ([11](<11 - Make and CMake.md>)) and load it with GDB ([10](<10 - Loading and debugging.md>)). The three LEDs blink together.

## Vendor definitions

I found every address and bit by hand, but ST already provides them all in the CMSIS device header `stm32n657xx.h` (in `CMSIS/Device/ST/STM32N6xx/Include/`, from [STM32CubeN6](https://github.com/STMicroelectronics/STM32CubeN6)). Each peripheral is a struct whose fields are its registers in offset order, plus a pointer macro at the base address. So `GPIOG->BSRR` is the register at `0x5602 1800 + 0x18`.

| My value | ST definition | `stm32n657xx.h` line |
|---|---|---|
| `0x5602 0000` | `AHB4PERIPH_BASE_S` | 3350 |
| `0x5602 1800` | `GPIOG_BASE_S`, `GPIOG` | 3517, 4265 |
| `0x5602 8000` | `RCC_BASE_S`, `RCC` | 3526, 4556 |
| `0x5602 825C` | `RCC->AHB4ENR` (`RCC_TypeDef`) | 2022, 2118 |
| `0x5602 1800` | `GPIOG->MODER` (`GPIO_TypeDef`) | 1435 |
| `0x5602 1818` | `GPIOG->BSRR` | 1443 |
| bit 6 | `RCC_AHB4ENR_GPIOGEN` | 28013 |
| `01` for pins 0, 8, 10 | `GPIO_MODER_MODEx_0`, `GPIO_MODER_MODEx_1` | 20114 |
| `0x0000 0501` | `GPIO_BSRR_BS0 \| GPIO_BSRR_BS8 \| GPIO_BSRR_BS10` | 20510 |
| `0x0501 0000` | `GPIO_BSRR_BR0 \| GPIO_BSRR_BR8 \| GPIO_BSRR_BR10` | 20558 |

One catch: ST defines `GPIOG` and `RCC` as the secure pointers (`0x5...`) only when compiling with `-mcmse`. Without it they point to the non-secure ones (`0x4...`). My code runs secure, so in my header I fix them to the secure ones.

I copied only these definitions into my trimmed `include/.../stm32n657xx.h` (the same way as in [06](<06 - CMSIS.md>)), with the line numbers of the original as comments. `RCC_TypeDef` is cut after `AHB4ENR`, with the registers before it replaced by one reserved block of `0x25C / 4 = 151` words so the offset stays right. `core_cm55.h` also got `__IO`, the `volatile` qualifier the structs use.

With `#include "stm32n6xx.h"`, `main.c` no longer needs its own `#define`s.

## The program with ST definitions

The same blinky, now with the vendor names instead of raw addresses and values:

```c
#include "stm32n6xx.h"
#include <stdint.h>

int counter = 0;

int main(void) {
    // GPIOG clock on, the other bits stay as they are
    RCC->AHB4ENR |= RCC_AHB4ENR_GPIOGEN;

    // PG0, PG8, PG10 as outputs: clear the 2-bit fields, then set 01
    uint32_t moder = GPIOG->MODER;
    moder &= ~(GPIO_MODER_MODE0 | GPIO_MODER_MODE8 | GPIO_MODER_MODE10);
    moder |= (GPIO_MODER_MODE0_0 | GPIO_MODER_MODE8_0 | GPIO_MODER_MODE10_0);
    GPIOG->MODER = moder;

    // Active low: pins high = LEDs off, pins low = LEDs on
    uint32_t LEDS_OFF = (GPIO_BSRR_BS0 | GPIO_BSRR_BS8 | GPIO_BSRR_BS10);
    uint32_t LEDS_ON = (GPIO_BSRR_BR0 | GPIO_BSRR_BR8 | GPIO_BSRR_BR10);

    while (1) {
        GPIOG->BSRR = LEDS_OFF;
        while (counter < 50000) {
            counter++;
        }
        counter = 0;
        GPIOG->BSRR = LEDS_ON;
        while (counter < 50000) {
            counter++;
        }
        counter = 0;
    }

    return 0;
}
```

What changed compared to the first version:

- **`|=` instead of `=` for the clock.** A read-modify-write: read `AHB4ENR`, set bit 6, write it back. The other bits (like `PWREN`) stay as they are, so I don't need to know the reset value.
- **Clear, then set for the mode.** `|=` can only set bits. The pins start as `11` (analog), so setting `_0` alone changes nothing. First clear the whole field with the mask (`GPIO_MODER_MODEx` = `0b11`), then set `_0` to get `01`. This works whatever the field held before.
- **One read and one write of `MODER`.** The register is `volatile`, so every `GPIOG->MODER |= ...` would be a separate hardware read and write. Working in a local variable gives one of each, and the three pins change mode together.
- **Names for the BSRR values.** `BS` (set, pin high) is LEDs **off** and `BR` (reset, pin low) is LEDs **on**, because the LEDs are active low.

ST names come in three forms: `_Pos` is the bit number, `_Msk` is the bit already shifted into place, and the plain name (`RCC_AHB4ENR_GPIOGEN`) is the same as `_Msk` and is the one normally used.

### clangd: "not used directly"

`main.c` includes the family header `stm32n6xx.h`, as ST intends. clangd warned that it is "not used directly": everything `main.c` uses is declared in `stm32n657xx.h`, which `stm32n6xx.h` only includes. A comment on that include tells clangd that the family header passes the device header's symbols on:

```c
#include "stm32n657xx.h" // IWYU pragma: export
```

IWYU stands for *include what you use*, the tool these pragmas come from.
