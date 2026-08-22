# UF2S8 Pipelined Core

> **5-Stage Pipelined SystemVerilog Hardware Implementation of the UF2S8 CPU.**

[![Language: SystemVerilog](https://img.shields.io/badge/Language-SystemVerilog-33E)](../../README.md)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](../../LICENSE)

This directory contains the synthesizable, cycle-accurate 5-stage pipelined implementation of the UF2S8 architecture. It supports the full UF2S8 instruction set including arithmetic, logic, conditional branching, stack operations, multi-cycle subroutine calls, software interrupts, hardware interrupts with vector table dispatch, and low-power sleep modes.

---

## Features

- **5-Stage Pipeline Architecture**:
  - **IF (Instruction Fetch)**: 16-bit word-addressed program counter with NPC generation and delay-slot flush support.
  - **ID (Instruction Decode)**: Immediate generation, register file read, CSR file access, condition evaluation, and control signal generation.
  - **EX (Execute)**: 8-bit ALU computation, branch target calculation, and address calculation.
  - **MEM (Memory Access)**: Byte-level memory read/write operations and address pair handling.
  - **WB (Writeback)**: GPR commit, CSR commit (flags, SP), and target PC updates.
- **Comprehensive Hazard Handling**:
  - **Data Forwarding Unit**: Forwarding paths from EX, MEM, and WB stages to Decode for general-purpose registers (`r0`–`r7`), 16-bit address register pairs (`a0`–`a3`), Stack Pointer (`SP`), and condition flags.
  - **Load-Use Hazard Detection**: Automatic single-cycle stall insertion when an instruction depends on data loaded by an immediately preceding memory read.
  - **Branch & Interrupt Delay Flushing**: Automatic IF/ID flushing to `NOP` (`0x0000`) during multi-cycle branch and interrupt delay slots to prevent stale instruction execution.
- **Multi-Cycle & Control Transfer Engine**:
  - **Subroutine Branch & Link**: `BL rel` and `BL reg` with 2-stage stack push of 16-bit return address.
  - **Subroutine Return**: `RET` with 2-stage stack pop of 16-bit target PC.
  - **Software Interrupts**: `SWI` with multi-cycle push of PC and flags, followed by vector lookup at `0xFF00 + (ID << 1)`.
  - **Hardware Interrupts**: Asynchronous interrupt interface with acknowledgement handshake (`o_int_ack`), priority gating via `flags.I` (bit 7), vector dispatch, and return address save.
  - **Power Management**: `WFI` (Wait For Interrupt) sleep mode with hardware wake-up triggering.
  - **Return from Interrupt**: `RETI` with 3-stage stack pop restoring `flags`, `PC[7:0]`, and `PC[15:8]`.

---

## Directory & File Structure

| File | Module | Description |
|------|--------|-------------|
| `cpu.sv` | `cpu` | Top-level 5-stage pipelined CPU interconnecting all pipeline registers. |
| `stage_fetch.sv` | `stage_fetch` | Instruction fetch stage logic and program counter register. |
| `stage_decode.sv` | `stage_decode` | Decode stage combining register file, CSRs, control unit, and operand multiplexers. |
| `control_unit.sv` | `control_unit` | Main decoder, condition tester, multi-cycle state machines for interrupts and branches. |
| `forwarding_unit.sv` | `forwarding_unit` | Operand forwarding unit and load-use stall detector. |
| `alu.sv` | `alu` | 8-bit Arithmetic Logic Unit with carry, zero, negative, and overflow computation. |
| `immediate_gen.sv` | `immediate_gen` | Immediate extraction and sign/zero extension unit. |
| `register_file.sv` | `register_file` | 8 general-purpose 8-bit registers (`r0`–`r7`) with dual read ports and single write port. |
| `csr_file.sv` | `csr_file` | Control & Status Registers (flags and 16-bit hardware stack pointer `SP`). |
| `ram.sv` | `ram` | Synchronous 64 KB memory model for instructions and data. |
| `tb_cpu.sv` | `tb_cpu` | Testbench verifying full pipeline execution, interrupt dispatch, and `WFI` wake-up. |
| `test.s` | — | Verification assembly suite testing arithmetic, branches, stack, SWI, WFI, and hardware interrupts. |

---

## Pipeline Structure

```
+---------------+     +----------------+     +------------+     +------------+     +----------------+
|   IF Stage    | --> |    ID Stage    | --> |  EX Stage  | --> | MEM Stage  | --> |    WB Stage    |
| (stage_fetch) |     | (stage_decode) |     |   (ALU)    |     |  (Memory)  |     | (Register/CSR) |
+---------------+     +----------------+     +------------+     +------------+     +----------------+
        ^                      |                    |                 |                    |
        |                      +<-- EX Forward -----+                 |                    |
        |                      +<-- MEM Forward ----------------------+                    |
        |                      +<-- WB Forward --------------------------------------------+
        |                                                                                  |
        +---------------------------- Multi-cycle PC Update -------------------------------+
```

---

## Simulation & Verification

The pipelined core can be verified using **Icarus Verilog** alongside the custom UF2S8 two-pass assembler.

### 1. Assemble the Test Suite

```bash
../../assembler/bin/assembler test.s test.bin
```

### 2. Compile and Run the Testbench

```bash
iverilog -g2012 -o tb_cpu *.sv
vvp tb_cpu
```

### 3. Expected Test Output

```
================ SIMULATION FINISHED ================
Final Registers:
r0 = 0x03
r1 = 0x11
r2 = 0x22
r3 = 0xBE
r4 = 0xEF
r5 = 0xAA
r6 = 0x77
r7 = 0x88
mem[0x0200] = 0xDE
mem[0x0204] = 0xAD
=====================================================
```

Waveforms are dumped to `tb_cpu.vcd` and can be inspected using GTKWave:

```bash
gtkwave tb_cpu.vcd
```

---

## License

This component is part of the UF2S8 project and is licensed under the [GNU General Public License v3.0](../../LICENSE).
