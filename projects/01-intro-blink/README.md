# 01-intro-blink

Bring-up of the STM32N657 from nothing: startup code, linker script, trimmed CMSIS headers, a CMake build, and blinking the three user LEDs (PG0, PG8, PG10). The program runs from RAM, loaded by the debugger (dev boot).

## Build

From this folder:

```sh
docker compose build                                  # once, builds the toolchain image
docker compose run --rm stm32-dev cmake -S . -B build
docker compose run --rm stm32-dev cmake --build build
```

Output: `build/app.elf`.
