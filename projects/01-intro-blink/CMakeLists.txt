cmake_minimum_required(VERSION 3.25)

# Cross compile with arm-none-eabi-gcc. Must be set before project()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake)

project(stm32n6-demo C)

# Write build/compile_commands.json with the exact command for every source (for clangd)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_EXECUTABLE_SUFFIX .elf)

add_executable(app
    src/main.c
    src/startup_stm32n657xx.c
    src/system_stm32n6xx.c
)

target_include_directories(app PRIVATE
    include/CMSIS/Core/Include
    include/CMSIS/Device/ST/STM32N6xx/Include
)

target_compile_options(app PRIVATE -mcpu=cortex-m55 -mthumb -O0 -g)

target_link_options(app PRIVATE
    -mcpu=cortex-m55 -mthumb
    -nostdlib
    -T ${CMAKE_SOURCE_DIR}/linker/STM32N657XX.ld
)
