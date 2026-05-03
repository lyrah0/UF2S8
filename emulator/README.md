# UF2S8 Emulator

> **SDL3-based hardware emulator for the UF2S8 architecture.**

[![Language: C](https://img.shields.io/badge/Language-C23-orange.svg)](../README.md)
[![Library: SDL3](https://img.shields.io/badge/Library-SDL3-blue.svg)](https://www.libsdl.org/)

The UF2S8 Emulator provides a complete virtual environment for executing software compiled for the UF2S8 architecture. It emulates the CPU, memory subsystem (including banking), and various hardware peripherals like the graphics blitter and keyboard.

## Features

- **CPU Core** — Full emulation of the UF2S8 instruction set, registers, and flags.
- **Hardware Blitter** — SDL3-accelerated 2D graphics engine supporting fills, blits, transparency, and clipping.
- **Banked Memory** — Sophisticated memory controller supporting up to 656 KB of addressable space via bank switching.
- **Integrated Debugger** — CLI-based debugger for stepping through code, inspecting registers, and memory.
- **I/O Emulation** — Realistic interrupt-driven keyboard and timer support.

## Source Structure

| File | Purpose |
|------|---------|
| `src/main.c` | SDL3 initialization and main execution loop. |
| `src/cpu.c` | Instruction fetch-decode-execute cycle and ALU logic. |
| `src/graphics.c` | Hardware blitter implementation and SDL3 rendering. |
| `src/memory.c` | Memory map management and bank switching logic. |
| `src/io.c` | Peripheral emulation (keyboard, timer) and interrupts. |
| `src/debugger.c` | Built-in CLI debugger and state inspection. |

## Building

The emulator requires SDL3 and a modern C compiler (GCC 14+).

```sh
make
```

### Dependencies

- **SDL3**: Graphics and input handling.

## Usage

Run a compiled binary with optional graphics support:

```sh
./bin/emulator [-g] program.bin
```

### Options

- `-g`: Enable graphical output window.
- `-d`: Start in debugger mode.

## Architecture

For a detailed breakdown of the hardware registers, memory map, and interrupt vectors, see the [Hardware Architecture Documentation](arch.md).

## License

This component is part of the UF2S8 project and is licensed under the [GNU General Public License v3.0](../LICENSE).
