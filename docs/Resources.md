
Manuals and links for the STM32N657 and the NUCLEO-N657X0-Q board.

## Chip: STM32N657

1. [Reference manual RM0486](https://www.st.com/resource/en/reference_manual/dm00769900.pdf): registers of every peripheral (RCC, GPIO, PWR, ...), memory map
2. [Datasheet STM32N6x5xx / N6x7xx](https://www.st.com/resource/en/datasheet/stm32n657a0.pdf): pinout, electrical characteristics, features
3. [Cortex-M55 programming manual PM0273](https://www.st.com/resource/en/programming_manual/pm0273-stm32-cortexm55-mcus-programming-manual-stmicroelectronics.pdf): core registers (SCB, NVIC, MPU, ...), instruction set, exceptions
4. [Boot ROM UM3234](https://www.st.com/resource/en/user_manual/um3234-how-to-proceed-with-boot-rom-on-stm32n6-mcus-stmicroelectronics.pdf): boot modes, dev boot, FSBL loading

## Board: NUCLEO-N657X0-Q (MB1940)

1. [Board user manual UM3417](https://www.st.com/resource/en/user_manual/um3417-stm32n6-nucleo144-board-mb1940-stmicroelectronics.pdf): LEDs, buttons, boot switches, connectors, ST-LINK
2. [Zephyr board files](https://github.com/zephyrproject-rtos/zephyr/tree/main/boards/st/nucleo_n657x0_q): quick check of pin assignments (LEDs: PG0 green, PG8 blue, PG10 red, active low)

## ST software and tools

1. [STM32CubeN6 on GitHub](https://github.com/STMicroelectronics/STM32CubeN6): CMSIS device files, HAL, BSP, examples
2. [CubeN6 Nucleo examples](https://github.com/STMicroelectronics/STM32CubeN6/tree/main/Projects/NUCLEO-N657X0-Q/Applications): example linker scripts and FSBL projects
3. [STM32CubeProgrammer CLI](https://dev.st.com/stm32cube-docs/prog/2.23.0/en/docs/markup/CubeProg_Command_Lines.html)

## Articles and examples

1. [ST Community](https://community.st.com/)
2. [STM32N6 FSBL explained](https://community.st.com/stm32-mcus-60/stm32n6-fsbl-explained-145831)
3. [STM32N6 boot ROM explained](https://community.st.com/stm32-mcus-60/stm32n6-boot-rom-explained-145756)
