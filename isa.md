# Unnamed fixed 2-byte simple 8
# Design principles

* 8 GPRs
* Load/store
* 16-bit/2-byte fixed instructions
* 8-bit data
* 16-bit addresses in register pairs
* Memory is byte addressable
* Little-endian
* Interrupt support
* Flags
* Stack grows downwards

## interrupts

* Interrupts jump to the address at 0xFF00 + (interrupt ID << 1)
	- Interrupts push PC and flags to the stack.
128 interrupt IDs

|ID  |Name	|Description|
|----|----------|-----------|
|0x00|Halt	|Halt execution
|0x01|Breakpoint|Breakpoint
|0x02|illegal instruction|Illegal instruction
|0x03-0x0F|Reserved|

## Registers

|Name		|Description|
|---------------|-----------|
|r0-7		|Registers
|a0-3		|Address registers

address registers are virtual and mapped to the normal registers in pairs

Control&status registers(i.e csr):

|Name	|Number	|Description|
|-------|-------|-----------|
|flags	|0x0	|flags register
|spl	|0x6	|Stack register low
|sph	|0x7	|Stack register high

* flags format:
    - 7: Interrupt
    - 6-4: Reserved
    - 3: signed oVerflow
    - 2: Negative
    - 1: Zero
    - 0: Carry

Conditions:

|Mnemonic|Opcode|Flags        |Description|
|--------|------|-------------|-----------|
|ZS/EQ   |0     |z==1         |Equal, zero|
|ZC/NE   |1     |z==0         |Not equal, not zero|
|CS/HS   |2     |c==1         |Carry set, unsigned higher or same|
|CC/LO   |3     |c==0         |Carry clear, unsigned lower|
|NS/MI   |4     |n==1         |Negative set|
|NC/PL   |5     |n==0         |Negative clear|
|VS      |6	|v==1         |Overflow set|
|AL      |7	|none         |Always|

## instructions

|Mnemonic|OPCODE             |Description|
|--------|-------------------|-----------|
|NOP     |000_000_000_000_0000|No operation
|RET     |001_000_000_000_0000|Return
|WFI     |010_000_000_000_0000|Wait for interrupt
|RETI    |000_001_000_000_0000|Return from interrupt
|INCC    |ddd_010_000_000_0000|Increment Carry
|DECB    |ddd_011_000_000_0000|Decrement Borrow
|SWI     |sss_101_000_000_0000|Software interrupt from register
|POP     |ddd_110_000_000_0000|pop register from stack
|PUSH    |sss_111_000_000_0000|push register to stack
|MOV     |ddd_sss_000_001_0000|Move csr to register
|MOV     |ddd_sss_001_001_0000|Move register to csr
|CMP     |sss_sss_010_001_0000|Compare subtraction
|CMA     |sss_sss_011_001_0000|Compare And
|B       |ccc_bb0_000_111_0000|Branch register (COND)
|BL      |ccc_bb1_000_111_0000|Branch register and push return address to stack (COND)
|SUB     |ddd_sss_sss_000_0001|Subtract
|SBB     |ddd_sss_sss_001_0001|Subtract with borrow
|ADD     |ddd_sss_sss_010_0001|Add
|ADC     |ddd_sss_sss_011_0001|Add with carry
|AND     |ddd_sss_sss_100_0001|And
|MOV     |ddd_sss_sss_100_0001|Move register to register (both source registers are the same)
|OR      |ddd_sss_sss_101_0001|Or
|NOR     |ddd_sss_sss_110_0001|Not Or
|XOR     |ddd_sss_sss_111_0001|Exclusive or
|SLL     |ddd_sss_sss_000_0010|Shift Left Logical
|SRL     |ddd_sss_sss_001_0010|Shift Right Logical
|SRA     |ddd_sss_sss_010_0010|Shift Right Arithmetic
|SLL     |ddd_sss_iii_100_0010|Shift Left Logical immediate
|SRL     |ddd_sss_iii_101_0010|Shift Right Logical immediate
|SRA     |ddd_sss_iii_110_0010|Shift Right Arithmetic immediate
|LI      |ddd_iii_iii_ii0_1010|Load Immediate
|ADD     |ddd_sss_iii_iii_1011|Add signed immediate
|SB      |sss_bbo_ooo_ooo_1100|Store byte with offset
|LB      |ddd_bbo_ooo_ooo_1101|Load byte with offset
|B       |ccc_ooo_ooo_ooo_1110|Branch relative (COND)
|BL      |ccc_ooo_ooo_ooo_1111|Branch relative and push return address to stack (COND)

### instruction details

flag legend:

x - unknown
m - modified

#### NOP - No operation
Does nothing

#### RET - Return
Pop return address from stack to PC

#### WFI - Wait for interrupt
Halt execution until an interrupt occurs

#### RETI - Return from interrupt
Pop return address and flags from stack to PC and Flags registers

#### INCC - Increment Carry
Increment dst with Carry flag

C: m Z: m N: m V: m

#### DECB - Decrement Borrow
Decrement dst with Borrow flag

C: m Z: m N: m V: m

#### SWI - Software interrupt from register
Software interrupt with ID from register

#### POP - Pop register from stack
Pop from stack into register dst

C: x Z: m N: m V: x

#### PUSH - Push register to stack
Push register src to stack

#### MOV - Move csr to register
Move csr to register

C: x Z: m N: m V: x

#### MOV - Move register to csr
Move register to csr

#### CMP - Compare subtraction
Compare subtraction src1 and src2

C: m Z: m N: m V: m

#### CMA - Compare And
Compare And src1 and src2

C: x Z: m N: m V: x

#### B - Branch register (COND)
Branch register (COND)

#### BL - Branch register and push return address to stack (COND)
Branch register and push return address to stack (COND)

#### SUB - Subtract
Subtract src2 from src1 and store at dst

C: m Z: m N: m V: m

#### SBB - Subtract with borrow
Subtract src2 with borrow from src1 and store at dst

C: m Z: m N: m V: m

#### ADD - Add
Add src1 and src2 and store at dst

C: m Z: m N: m V: m

#### ADC - Add with carry
Add src1 with carry and src2 and store at dst

C: m Z: m N: m V: m

#### AND - And
And src1 and src2 and store at dst

C: x Z: m N: m V: x

#### MOV - Move register to register (both source registers are the same)
Move register src to register dst (both source registers are the same)

C: x Z: m N: m V: x

#### OR - Or
Or src1 and src2 and store at dst

C: x Z: m N: m V: x

#### NOR - Not Or
Not Or src1 and src2 and store at dst

C: x Z: m N: m V: x

#### XOR - Exclusive or
Exclusive or src1 and src2 and store at dst

C: x Z: m N: m V: x

#### SLL - Shift Left Logical
Shift Left Logical src1 with src2 and store at dst

C: x Z: m N: m V: x

#### SRL - Shift Right Logical
Shift Right Logical src1 with src2 and store at dst

C: x Z: m N: m V: x

#### SRA - Shift Right Arithmetic
Shift Right Arithmetic src1 with src2 and store at dst

C: x Z: m N: m V: x

#### SLL - Shift Left Logical immediate
Shift Left Logical src1 with immediate and store at dst

C: x Z: m N: m V: x

#### SRL - Shift Right Logical immediate
Shift Right Logical src1 with immediate and store at dst

C: x Z: m N: m V: x

#### SRA - Shift Right Arithmetic immediate
Shift Right Arithmetic src1 with immediate and store at dst

C: x Z: m N: m V: x

#### LI - Load Immediate
Load Immediate into dst

C: x Z: m N: m V: x

#### ADD - Add signed immediate
Add src1 with signed immediate and store at dst

C: m Z: m N: m V: m

#### SB - Store byte with offset
Store src with base + offset in memory


#### LB - Load byte with offset
Load byte from base + offset in memory to dst

C: x Z: m N: m V: x

#### B - Branch relative (COND)
Branch relative (COND)

#### BL - Branch relative and push return address to stack (COND)
Branch relative and push return address to stack (COND)