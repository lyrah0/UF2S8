# UF2S8 Assembler

> **Two-pass assembler for the UF2S8 architecture.**

[![Language: C](https://img.shields.io/badge/Language-C23-orange.svg)](../README.md)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](../LICENSE)

The UF2S8 Assembler is a lightweight, efficient tool designed to convert assembly source code into binary machine code for the UF2S8 virtual machine. It features a two-pass architecture to support forward-referencing labels and complex symbol resolution.

## Features

- **Two-Pass Design** — Ensures all labels and symbols are resolved correctly before final encoding.
- **Label Support** — Easy-to-use label system for branching and data references.
- **Directives** — Supports standard directives like `.db` for data embedding.
- **Robust Error Handling** — Provides clear feedback for syntax errors and undefined symbols.
- **Standard Output** — Generates flat binary files compatible with the UF2S8 Emulator.

## Source Structure

| File | Purpose |
|------|---------|
| `src/main.c` | Entry point, command-line argument parsing. |
| `src/lexer.c` | Tokenizes the assembly source into a stream of tokens. |
| `src/handle.c` | Processes directives and instructions, managing the two-pass logic. |
| `src/encode.c` | Handles instruction bit-encoding and binary output generation. |
| `src/symbol.c` | Manages the symbol table for labels and constants. |

## Building

The assembler requires a modern C compiler (GCC 14+) with C23 support.

```sh
make
```

To build with debug symbols and extra warnings:

```sh
make debug=1
```

## Usage

Pass the input assembly file and the desired output binary path:

```sh
./bin/assembler input.s output.bin
```

### Command-line Arguments

- `input.s`: The source assembly file.
- `output.bin`: The destination binary file.

## License

This component is part of the UF2S8 project and is licensed under the [GNU General Public License v3.0](../LICENSE).
