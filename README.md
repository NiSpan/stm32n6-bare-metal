# STM32N6 bare metal

Bare-metal programming on the STM32N657 (Arm Cortex-M55) from scratch, on the NUCLEO-N657X0-Q board: my own startup code, linker script and trimmed CMSIS headers, built with arm-none-eabi-gcc and CMake in Docker.

- `docs/`: step-by-step notes, written as I go.
- `projects/`: one small project per step.

| Project | What it does |
|---|---|
| [01-intro-blink](projects/01-intro-blink) | Startup code, linker script, CMake build, blinking the three user LEDs |

## License

My own code and notes: [0BSD](LICENSE), use them for anything, no attribution needed.

The trimmed headers in `projects/*/include/CMSIS` are derived from Arm CMSIS and STMicroelectronics STM32CubeN6 and stay under [Apache-2.0](LICENSES/Apache-2.0.txt), as stated in each file.
