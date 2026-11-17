## The board

What I have is an ST development board: the **NUCLEO-N657X0-Q** (board code **MB1940**), from ST's Nucleo-144 family.

The first thing to do is find the board manual, to see what the board offers: [Board user manual UM3417](https://www.st.com/resource/en/user_manual/um3417-stm32n6-nucleo144-board-mb1940-stmicroelectronics.pdf).

The board has the **STM32N657X0** MCU and a USB-C connector (**CN10**) for my development PC. Behind CN10 is the on-board **ST-LINK/V3** debugger, which talks to the MCU over SWD (debug) or UART (virtual COM port).

So the first logical step is to connect the PC to CN10 with a USB-C cable and use the STM32CubeProgrammer CLI to check that I can communicate with the board, and try some operations on it.

## Listing the interfaces

With the board connected to CN10 and the boot jumpers in their default position, I first asked CubeProgrammer what it can see:

```sh
STM32_Programmer_CLI -l
```

```
=====  DFU Interface   =====
No STM32 device in DFU mode connected

===== J-Link Interface =====
No J-Link/flasher probe detected.

===== STLink Interface =====
ST-Link Probe 0 :
   ST-LINK SN  : 004A003F3234511233353533
   ST-LINK FW  : V3J15M6
   Access Port Number  : 3
   Board Name  : NUCLEO-N657X0-Q

=====  UART Interface  =====
Total number of serial ports available: 5
Port: ttyACM0   Location: /dev/ttyACM0   Description: STLINK-V3
Port: ttyS0..3  Location: /dev/ttyS0..3  Description: N/A
```

| Interface | What it found                                                                                     |
| --------- | ------------------------------------------------------------------------------------------------- |
| DFU       | nothing: an STM32 in USB bootloader (DFU) mode would appear here                                  |
| J-Link    | nothing: no SEGGER probe connected                                                                |
| ST-LINK   | the on-board ST-LINK/V3, with its serial number, firmware version and the board name              |
| UART      | `/dev/ttyACM0`: the ST-LINK's virtual COM port (USART1 of the MCU, UM3417). `ttyS0`–`ttyS3`: the PC's own serial ports, nothing to do with the board |

So the PC sees the board's ST-LINK and its serial port. This only talks to the ST-LINK; the MCU itself is not involved yet.

## First connection: it fails

`-l` shows two ways to reach the MCU: through the ST-LINK (SWD) and through the serial port (UART). They work differently:

- **SWD** connects to the core's **debug hardware**: it can read memory and registers and halt the core, without needing any software on the MCU. This is the usual first choice.
- **UART** is only a serial line: it answers only if **software on the MCU is listening** on it, for example a bootloader.

I tried SWD first:

```sh
STM32_Programmer_CLI -c port=SWD
```

```
ST-LINK SN  : 004A003F3234511233353533
ST-LINK FW  : V3J15M6
Board       : NUCLEO-N657X0-Q
Voltage     : 3.28V
Error: Unable to get core ID
Error: No STM32 target found! If your product embeds Debug Authentication, please perform a discovery using Debug Authentication
2nd connect tentative with frequency (8MHz)
ST-LINK SN  : 004A003F3234511233353533
ST-LINK FW  : V3J15M6
Board       : NUCLEO-N657X0-Q
Voltage     : 3.27V
Error: Unable to get core ID
Error: No STM32 target found! If your product embeds Debug Authentication, please perform a discovery using Debug Authentication
```

The PC reaches the **ST-LINK** (serial number, firmware, board name, voltage are read), but the ST-LINK cannot reach the **MCU core**.

Then over the serial port:

```sh
STM32_Programmer_CLI -c port=/dev/ttyACM0
```

```
Serial Port /dev/ttyACM0 is successfully opened.
Port configuration: parity = even, baudrate = 115200, data-bit = 8, stop-bit = 1.0, flow-control = off
Error: Activating device: KO. Please, verify the boot mode configuration and check the serial port configuration. Reset your device then try again...
```

The port opens, but nothing on the MCU answers. The error itself points to the **boot mode**.

Both ways fail at the MCU, not at the cable or the probe.

The reason is in how this chip boots, so that is the next step: [The boot process](<02 - The boot process.md>).
