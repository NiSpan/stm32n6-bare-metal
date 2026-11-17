In [09](<09 - Cross compiling.md>) I built the program by hand, one `arm-none-eabi-gcc` command per file. That works for three files, but the commands are long, the flags repeat, and I have to remember which files changed. A build tool fixes this.

**Make** reads a `Makefile`: a list of **rules**. Each rule names a target (a file to produce), its prerequisites (the files it is made from) and the commands that produce it. Make compares file timestamps and rebuilds only the targets whose prerequisites are newer.

**CMake** is one level higher. It does not build anything itself. I describe the project in `CMakeLists.txt` (sources, include paths, flags, target), and CMake **generates** the build files for a real build tool (Makefiles or Ninja). It also handles things that are tedious in plain Make: header dependencies, separate build directories, and toolchain selection for cross compiling.

I start with Make, because CMake generates Makefiles and it helps to know what it produces. Then I move the same build to CMake.

Both are in the Docker container ([00.1 - Docker](<00.1 - Docker.md>)):

```
developer@b3f2d57db87d:/workspace$ make --version
GNU Make 4.4.1
developer@b3f2d57db87d:/workspace$ cmake --version
cmake version 3.31.6
```

At this point the project was still mounted at `/workspace` in the container. Later in this step it moves to the same path as on the host ([00.1 - Docker](<00.1 - Docker.md>)).

References: [GNU make manual](https://www.gnu.org/software/make/manual/make.html), [CMake documentation](https://cmake.org/cmake/help/v3.31/).

## A first Makefile

My first `Makefile` is the commands from [09](<09 - Cross compiling.md>), copied into rules. Nothing clever, just enough to build and clean the demo with one word.

```make
all: build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o build/app.elf

build/main.o: src/main.c | build
	arm-none-eabi-gcc -mcpu=cortex-m55 -mthumb -O0 -g -c src/main.c -o build/main.o

build/startup_stm32n657xx.o: src/startup_stm32n657xx.c | build
	arm-none-eabi-gcc -mcpu=cortex-m55 \
		-mthumb	\
		-O0	\
		-Iinclude/CMSIS/Core/Include \
		-Iinclude/CMSIS/Device/ST/STM32N6xx/Include \
		-g \
		-c src/startup_stm32n657xx.c \
		-o build/startup_stm32n657xx.o

build/system_stm32n6xx.o: src/system_stm32n6xx.c | build
	arm-none-eabi-gcc -mcpu=cortex-m55 \
		-mthumb \
		-O0 \
		-Iinclude/CMSIS/Core/Include \
		-Iinclude/CMSIS/Device/ST/STM32N6xx/Include \
		-g \
		-c src/system_stm32n6xx.c \
		-o build/system_stm32n6xx.o

build/app.elf: build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o
	arm-none-eabi-gcc -mcpu=cortex-m55 \
		-mthumb \
		-nostdlib \
		-T linker/STM32N657XX.ld \
		build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o \
		-o build/app.elf

build:
	mkdir -p build

clean:
	rm -rf build
```

Every rule has the same shape:

```make
target: prerequisites
	command
```

- **target**: the file the rule produces, e.g. `build/main.o`.
- **prerequisites**: the files it is made from, e.g. `src/main.c`. If one of them is itself a target, Make builds it first.
- **command**: the shell command that produces the target. It must start with a **tab**, not spaces. A `\` at the end of a line continues the command on the next line.

There are two kinds of rules here:

- **File rules** (`build/*.o`, `build/app.elf`): one per compile step and one for the link step, the same commands as in 09.
- **Name rules** (`all`, `clean`): no file with that name is ever created, so Make always runs them. `all` only lists what I want built. `clean` deletes the whole `build/` folder.

The `build/` folder itself has a rule too: `mkdir -p build` creates it if it is missing, so the build works after a `make clean`. The objects depend on it with `| build`. Anything after `|` is an **order-only** prerequisite: Make makes sure it exists before the command runs, but does not rebuild the target when it changes. That matters for a folder, because its timestamp changes every time a file is written into it, and a normal prerequisite would cause needless rebuilds.

`make` with no argument runs the first rule in the file, which is why `all` comes first. So the build is:

```sh
make          # same as: make all
make clean
```

Make decides what to rebuild from timestamps. If `build/main.o` is older than `src/main.c`, it recompiles it, and because `build/app.elf` depends on `build/main.o`, it relinks too. The startup and system objects are left alone. A second `make` with no changes does nothing.

## Variables

The same flags were repeated in every rule. I moved them into variables at the top, using the names Make itself uses in its built-in rules:

```make
CC = arm-none-eabi-gcc

CPUFLAGS = -mcpu=cortex-m55 -mthumb
CPPFLAGS = -Iinclude/CMSIS/Core/Include -Iinclude/CMSIS/Device/ST/STM32N6xx/Include
CFLAGS = $(CPUFLAGS) -O0 -g
LDFLAGS = $(CPUFLAGS) -nostdlib -T linker/STM32N657XX.ld
```

- `CC`: the C compiler.
- `CPPFLAGS`: **preprocessor** flags (`-I`, `-D`). The name means C PreProcessor, not C++.
- `CFLAGS`: compiler flags (optimisation, debug info, warnings).
- `LDFLAGS`: linker flags.
- `CPUFLAGS`: my own name, not a standard one. The compile and the link both need the CPU flags, so they live in one place.

`$(NAME)` expands a variable. The rules become short and all the same:

```make
build/main.o: src/main.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c src/main.c -o build/main.o

build/app.elf: build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o
	$(CC) $(LDFLAGS) build/main.o build/startup_stm32n657xx.o build/system_stm32n6xx.o -o build/app.elf
```

Other common names I will meet: `CXX`/`CXXFLAGS` (C++), `AS`/`ASFLAGS` (assembler), `LDLIBS` (libraries, placed after the objects), `OBJCOPY`, `OBJDUMP`, `SIZE` (binutils).

## Moving to CMake

The Makefile works, but I keep it here only as a record. The project now builds with CMake, and the project folder has a `CMakeLists.txt` instead of a `Makefile`:

```
projects/01-intro-blink/
├── CMakeLists.txt
├── cmake/
│   └── arm-none-eabi.cmake
├── include/
├── linker/
└── src/
```

### The toolchain file

By default CMake looks for the host compiler and test-builds a small program with it. For the board I tell it which compiler to use and that there is no OS. This lives in its own file, `cmake/arm-none-eabi.cmake`:

```cmake
# Bare-metal target: no OS, cross compiler from the container
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)

# The compiler test cannot link a full program without our startup and linker script
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
```

- `CMAKE_SYSTEM_NAME Generic`: bare metal, no operating system.
- `CMAKE_C_COMPILER`: the cross compiler, same as `CC` in the Makefile.
- `CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY`: CMake checks the compiler by building a test program. Linking it would fail without my startup code and linker script, so it builds a library instead.

### CMakeLists.txt

```cmake
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
```

- `CMAKE_TOOLCHAIN_FILE`: loads the toolchain file. It must come before `project()`, because `project()` is where CMake finds the compiler. It is only read on the first configure of a build folder.
- `project(stm32n6-demo C)`: project name, C only.
- `CMAKE_EXPORT_COMPILE_COMMANDS`: writes `build/compile_commands.json` for clangd.
- `CMAKE_EXECUTABLE_SUFFIX .elf`: the output is `app.elf` (a Generic target has no suffix by default).
- `add_executable(app ...)`: the target and its sources. CMake writes the compile and link rules for me.
- `target_include_directories`: the `-I` paths, like `CPPFLAGS`.
- `target_compile_options`: like `CFLAGS`.
- `target_link_options`: like `LDFLAGS`.

`PRIVATE` means the setting is for this target only. It matters later, when there are libraries that pass settings on to the targets that use them.

### Configure and build

Inside the container, two steps:

```sh
cmake -S . -B build      # configure: generate build/Makefile
cmake --build build      # build: runs make in build/
```

- `-S .`: the source folder, where `CMakeLists.txt` is.
- `-B build`: the build folder. CMake puts everything it generates there, including the objects and `build/app.elf`.

Configure is only needed once, or after `CMakeLists.txt` changes (`cmake --build` reruns it by itself when needed). To start clean, I delete the build folder:

```sh
rm -rf build
```

To see the full compiler commands, as with the Makefile:

```sh
cmake --build build --verbose
```

The commands are the same as in the Makefile, plus `-MD`: gcc writes a list of the headers each source uses, so a change in a header now triggers a rebuild too.
