## After reset: the boot ROM

To find out how the chip starts, I went through the documents in this order:

1. **Datasheet**, feature list under *Memories* (section ??): SRAM and external memory interfaces (XSPI, FMC), but **no internal flash**. Most microcontrollers have internal flash, and after reset the core reads its first instructions from it. So how does this one boot?
2. **Reference manual RM0486**, memory map in *System and memory overview* (section ??): there is no flash region, but there is a **boot ROM** at `0x18000000` (Secure alias).
3. **RM0486**, boot chapter (section ??): the boot modes and the BOOT0/BOOT1 pins.
4. **Boot ROM manual UM3234**: the details of what the boot ROM does.
5. **Board manual UM3417** (section ??): where the BOOT0/BOOT1 jumpers are on the board.

What I learned: after reset, the core always starts in the **boot ROM**, ST code in read-only memory inside the chip. The boot ROM:

- reads the **boot pins** to choose a boot mode
- in normal operation, loads an image from an external memory into internal RAM, checks it, and starts it
- supports secure boot: image authentication and optional decryption

Details: [How to proceed with boot ROM on STM32N6 MCUs (UM3234)](https://www.st.com/resource/en/user_manual/um3234-how-to-proceed-with-boot-rom-on-stm32n6-mcus-stmicroelectronics.pdf), and the [STM32N6 boot ROM explained](https://community.st.com/stm32-mcus-60/stm32n6-boot-rom-explained-145756) article.

## The FSBL

The image the boot ROM loads is the **FSBL** (First Stage Boot Loader): the first program I write that runs on the chip in normal boot.

- It is stored in the **external flash** on the board (XSPI NOR, at `0x70000000`).
- It needs an **ST header** in front of it, added with ST's signing tool (`STM32_SigningTool_CLI`). Without a valid header the boot ROM refuses it.
- The boot ROM copies it into **internal SRAM** (AXISRAM2) and runs it from there.
- The FSBL then initializes the system (clocks, external memories, ...) and either is the whole application, or loads and starts a bigger application.

More: [STM32N6 FSBL explained](https://community.st.com/stm32-mcus-60/stm32n6-fsbl-explained-145831).

This is why my first connections failed: in the default boot mode the boot ROM looks for an FSBL in external flash and finds none. The debug access to the core stays closed (SWD fails), and nothing listens on the serial port (UART fails). According to ST, the boot ROM signals the failure on pin PG10 (BOOTFAILED), which on this board is the red LED.

## Boot modes

The boot pins are set with the **BOOT0** and **BOOT1** jumpers on the board (UM3417). The boot ROM reads them **at reset** (RM0486 boot chapter, UM3234, [STM32N6 boot ROM explained](https://community.st.com/stm32-mcus-60/stm32n6-boot-rom-explained-145756)):

| BOOT1 | BOOT0 | Mode                 | What the boot ROM does                                       |
| ----- | ----- | -------------------- | ------------------------------------------------------------ |
| 0     | 0     | Flash boot           | loads the FSBL from external flash and runs it               |
| 0     | 1     | Serial boot          | runs a bootloader that waits for commands over UART or USB   |
| 1     | x     | **Development boot** | starts nothing and leaves the debug port open                |

> [!warning]
> After moving a jumper, press **reset**. The pins are only read at reset, so without it the old mode stays active.

In **serial boot** there is software on the MCU that answers: the boot ROM's own bootloader. It listens on USART1, USART2, UART4 and USB DFU; on the Nucleo, USART1 is the ST-LINK's virtual COM port (`/dev/ttyACM0`).

## Next

Flash boot needs a signed program in external flash, which I don't have, and the debugger cannot connect without one. The other two modes let me talk to the MCU: **serial boot** through the boot ROM's bootloader, and **development boot** through the debug port. I try both in [03 - Exploring with STM32CubeProgrammer](<03 - Exploring with STM32CubeProgrammer.md>).
