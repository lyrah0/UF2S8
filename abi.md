# UF2S8 ABI Specification

This document defines the formal Application Binary Interface (ABI) for the UF2S8 architecture (ISA Version 2).

## Registers

The UF2S8 has 16 General Purpose Registers (`r0-r15`) and 8 Address Registers (`a0-a7`) which are virtual mappings of GPR pairs (`a0 = r1:r0`, `a1 = r3:r2`, etc.).
The hardware Stack Pointer (`spl`, `sph`) is stored in Control/Status Registers (`csr[14]`, `csr[15]`).

|Register   |Mapping  |Role                  |Preservation|
|-----------|---------|----------------------|------------|
|**a0**     |`r1:r0`  |Arg 1 / Return Value  |Caller-saved (Volatile)|
|**a1**     |`r3:r2`  |Arg 2 / Scratch 1     |Caller-saved (Volatile)|
|**a2**     |`r5:r4`  |Arg 3 / Scratch 2     |Caller-saved (Volatile)|
|**a3**     |`r7:r6`  |Arg 4 / Scratch 3     |Caller-saved (Volatile)|
|**a4**     |`r9:r8`  |Preserved 1 / Local 1 |**Callee-saved (Non-volatile)**|
|**a5**     |`r11:r10`|Preserved 2 / Local 2 |**Callee-saved (Non-volatile)**|
|**a6**     |`r13:r12`|Preserved 3 / Local 3 |**Callee-saved (Non-volatile)**|
|**a7**     |`r15:r14`|**Frame Pointer (FP)**|**Callee-saved (Non-volatile)**|
|**spl/sph**|`csr[14/15]`|Hardware Stack Pointer|N/A|

## Calling Convention (Fastcall)

### Argument Passing
Arguments are allocated from left-to-right into the argument register pool (`r0–r7` / `a0–a3`):

1. **8-bit Arguments**: Allocated to the next available general purpose register (`r0` through `r7`).
2. **16-bit Arguments / Pointers**: Allocated to the next available even-aligned address pair (`a0 = r1:r0`, `a1 = r3:r2`, `a2 = r5:r4`, `a3 = r7:r6`). If the current free register is odd, it is skipped (padded) to maintain 16-bit alignment.
3. **Stack Arguments**: Any argument that cannot fit in the remaining argument registers (`r0–r7`) is pushed onto the stack in reverse order (right-to-left).

#### Example Allocations
- `func(u8 a, u8 b, u8 c, u8 d, u8 e)` $\to$ `r0, r1, r2, r3, r4` (all in registers).
- `func(u8 a, u16 b, u8 c, u16 d)` $\to$ `r0, a1 (r3:r2), r4, a3 (r7:r6)` (`r1` and `r5` skipped for alignment).
- `func(u16 a, u16 b, u16 c, u16 d)` $\to$ `a0, a1, a2, a3`.

### Return Values
- **8-bit**: Returned in `r0`.
- **16-bit**: Returned in `a0` (`r0` low, `r1` high).

### Preservation Rules
- `a0` through `a3` (`r0-r7`) are **volatile (caller-saved)**. The caller must save them if their values are needed after a function call.
- `a4` through `a7` (`r8-r15`) are **non-volatile (callee-saved)**. The callee must save and restore them if modified.
- **Stack Arguments**: Arguments passed on the stack are owned by the callee and are considered volatile. The callee is permitted to overwrite incoming stack argument slots (e.g. `[a7 + 5]`) as local scratch space; the caller cannot assume their contents are preserved after a call. The caller remains responsible for cleaning up stack arguments upon return.

## Stack Frame Layout

The Frame Pointer (`a7`) points to the base of the current stack frame.

|Address|Content                  |Access    |
|-------|-------------------------|----------|
|High   |Argument 5               |`[a7 + 5]`|
|...    |Return Address High      |`[a7 + 4]`|
|...    |Return Address Low       |`[a7 + 3]`|
|...    |Saved `r15` (Old FP High)|`[a7 + 2]`|
|...    |Saved `r14` (Old FP Low) |`[a7 + 1]`|
|**FP** |Hardware SP High at entry|`r15`     |
|**FP** |Hardware SP Low at entry |`r14`     |
|...    |Local Variable 1         |`[a7]`|
|Low    |Local Variable 2         |`[a7 - 1]`|

## Function Lifecycle

### Prologue
```unixassembly
function_name:
	; 1. Save old Frame Pointer
	PUSH	r15
	PUSH	r14
	; 2. Set new Frame Pointer to current SP
	MOV	r14, spl
	MOV	r15, sph
	; 3. Save any used callee-saved registers (a4-a6 / r8-r13)
	; PUSH r9 / PUSH r8 etc.
```

### Epilogue
```unixassembly
	; 1. Restore any used callee-saved registers (a4-a6 / r8-r13)
	; POP r8 / POP r9 etc.
	; 2. Restore SP from FP (deallocates locals)
	MOV	spl, r14
	MOV	sph, r15
	; 3. Restore old Frame Pointer
	POP	r14
	POP	r15
	RET
```
