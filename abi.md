# UF2S8 ABI Specification

This document defines the formal Application Binary Interface (ABI) for the UF2S8 architecture.

## Registers

The UF2S8 has 8 General Purpose Registers (`r0-r7`) and 4 Address Registers (`a0-a3`) which are virtual mappings of the GPR pairs.
The hardware Stack Pointer (`spl`, `sph`) is a separate Control/Status Register (CSR).

|Register   |Mapping|Role                  |Preservation|
|-----------|-------|----------------------|------------|
|**a0**     |`r0:r1`|Arg 1 / Return Value  |Caller-saved|
|**a1**     |`r2:r3`|Arg 2 / Scratch 1     |Caller-saved|
|**a2**     |`r4:r5`|Scratch 2             |Caller-saved|
|**a3**     |`r6:r7`|**Frame Pointer (FP)**|**Callee-saved**|
|**spl/sph**|N/A    |Hardware Stack Pointer|N/A|

## Calling Convention (Fastcall)

### Argument Passing
1. **First Argument**: Passed in `a0` (16-bit) or `r0` (8-bit).
2. **Second Argument**: Passed in `a1` (16-bit) or `r2` (8-bit).
3. **Additional Arguments**: Pushed onto the stack in reverse order (right-to-left).

### Return Values
- **8-bit**: Returned in `r0`.
- **16-bit**: Returned in `a0` (`r0` low, `r1` high).

### Preservation Rules
- `a0`, `a1`, and `a2` (`r0-r5`) are **volatile**. The caller must save them if their values are needed after a function call.
- `a3` (`r6:r7`) is **non-volatile**. The callee must save and restore it.

## Stack Frame Layout

The Frame Pointer (`a3`) points to the base of the current stack frame.

|Address|Content                  |Access    |
|-------|-------------------------|----------|
|High   |Argument 3               |`[a3 + 5]`|
|...    |Return Address High      |`[a3 + 4]`|
|...    |Return Address Low       |`[a3 + 3]`|
|...    |Saved `r7` (Old FP High) |`[a3 + 2]`|
|...    |Saved `r6` (Old FP Low)  |`[a3 + 1]`|
|**FP** |Hardware SP High at entry|`r7`      |
|**FP** |Hardware SP Low at entry |`r6`      |
|...    |Local Variable 1         |`[a3 - 1]`|
|Low    |Local Variable 2         |`[a3 - 2]`|

## Function Lifecycle

### Prologue
```unixassembly
function_name:
    ; 1. Save old Frame Pointer
    PUSH a3
    ; 2. Set new Frame Pointer to current SP
    MOV r6, spl
    MOV r7, sph
```

### Epilogue
```unixassembly
    ; 1. Restore SP from FP (deallocates locals)
    MOV spl, r6
    MOV sph, r7
    ; 2. Restore old Frame Pointer
    POP a3
    RET
```
