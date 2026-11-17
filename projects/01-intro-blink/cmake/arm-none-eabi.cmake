# Bare-metal target: no OS, cross compiler from the container
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)

# The compiler test cannot link a full program without our startup and linker script
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
