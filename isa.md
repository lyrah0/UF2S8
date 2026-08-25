# Unnamed fixed 2-byte simple 8 Version 2
# Design principles

* 16 GPRs
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

* Interrupts jump to the address at VBR + (interrupt ID << 1)
	- Interrupts push PC and flags to the stack.
256 interrupt IDs

|ID  |Name	|Description|
|----|----------|-----------|
|0x00|Halt	|Halt execution
|0x01|Breakpoint|Breakpoint
|0x02|illegal instruction|Illegal instruction
|0x03-0x0F|Reserved|

## Registers

|Name       |Description|
|-----------|-----------|
|r0-15		|Registers
|a0-7		|Address registers

address registers are virtual and mapped to the normal registers in pairs

Control&status registers(i.e csr):

|Name	|Number	|Description|
|-------|-------|-----------|
|flags	|0x0	|flags register
|vbr    |0x1    |Vector branch register(7 upper bits, lowest bit hardwired to 0)
|rng    |0x2    |Hardware random number generator (R: next 8-bit random value, W: seed)
|cycl   |0xC    |Cycle counter low byte (16-bit cycle counter low)
|cych   |0xD    |Cycle counter high byte (16-bit cycle counter high)
|spl	|0xE	|Stack register low
|sph	|0xF	|Stack register high

* flags format:
    - 7: Interrupt
    - 6-5: Reserved
    - 4: Auxiliary carry
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
|VS      |6     |v==1         |Overflow set|
|VC      |7     |v==0         |Overflow clear|
|HI      |8     |(c==1)&(z==0)|Unsigned higher|
|LS      |9     |(c==0)|(z==1)|Unsigned lower or same|
|GE      |10    |n==v         |Signed greater or equal|
|LT      |11    |n!=v         |Signed less|
|GT      |12    |(z==0)&(n==v)|Signed greater|
|LE      |13    |(z==1)|(n!=v)|Signed less or equal|
|AS      |14    |a==1         |Auxiliary carry set|
|AC      |15    |a==0         |Auxiliary carry clear|

* Note: Codes 0-5 and AL cover 90% of cases.
    - Opcodes 0-5 are used 70.66% of the time if AL is excluded.
        - Coverage rises to 90.73% if LS,HI are added
    - Opcodes 0-5 can be used to create all other conditions.
    - Of those, EQ,NE,CS,CC cover 99%+.
    - PL is the least used condition

## instructions

legend:
s - source
m - modifier(second source)
d - destination
b - base
i - immediate
o - offset
c - condition

|Mnemonic|OPCODE              |Description|
|--------|--------------------|-----------|
|NOP     |0000_0000_0000_0000|No operation
|RET     |0000_0000_0001_0000|Return
|WFI     |0000_0000_0010_0000|Wait for interrupt
|RETI    |0000_0000_0011_0000|Return from interrupt
|SWI     |0001_0001_ssss_0000|Software interrupt from register
|PUSH    |0001_0010_ssss_0000|push register to stack
|POP     |0001_0011_dddd_0000|pop register from stack
|B       |0001_1101_bbb0_0000|Branch register
|BL      |0001_1101_bbb1_0000|Branch register and push return address to stack
|BST     |0001_1110_iii0_0000|Bit set flags
|BIC     |0001_1110_iii1_0000|Bit clear flags
|BTS     |0001_1111_iii0_0000|Bit test flags
|B       |0010_bbb0_cccc_0000|Branch register (COND)
|BL      |0010_bbb1_cccc_0000|Branch register and push return address to stack (COND)
|MOV     |0011_ssss_dddd_0000|Move register to register
|BST     |0100_iii0_dddd_0000|Bit set immediate
|BIC     |0100_iii1_dddd_0000|Bit clear immediate
|NOT     |0110_ssss_dddd_0000|Not
|NEG     |0111_ssss_dddd_0000|Negate
|INC     |1000_ssss_dddd_0000|Increment
|DEC     |1001_ssss_dddd_0000|Decrement
|MOV     |1010_ssss_dddd_0000|Move csr to register
|MOV     |1011_ssss_dddd_0000|Move register to csr
|INCC    |1100_ssss_dddd_0000|Increment Carry
|DECB    |1101_ssss_dddd_0000|Decrement Borrow
|CMP     |1110_ssss_ssss_0000|Compare subtraction
|CMA     |1111_ssss_ssss_0000|Compare And
|SUB     |0000_mmmm_dddd_0001|Subtract
|SBB     |0001_mmmm_dddd_0001|Subtract with borrow
|ADD     |0010_mmmm_dddd_0001|Add
|ADC     |0011_mmmm_dddd_0001|Add with carry
|AND     |0100_mmmm_dddd_0001|And
|ANDN    |0101_mmmm_dddd_0001|And Not
|OR      |0110_mmmm_dddd_0001|Or
|ORN     |0111_mmmm_dddd_0001|Or Not
|NOR     |1000_mmmm_dddd_0001|Not Or
|XOR     |1001_mmmm_dddd_0001|Exclusive or
|SLL     |1010_mmmm_dddd_0001|Shift Left Logical
|SRL     |1011_mmmm_dddd_0001|Shift Right Logical
|SRA     |1100_mmmm_dddd_0001|Shift Right Arithmetic
|SLL     |1101_iii0_dddd_0001|Shift Left Logical immediate
|SRL     |1110_iii1_dddd_0001|Shift Right Logical immediate
|SRA     |1111_iii0_dddd_0001|Shift Right Arithmetic immediate
|BTS     |1111_iii1_ssss_0001|Bit test immediate
|BST     |0000_mmmm_dddd_0010|Bit set
|BIC     |0001_mmmm_dddd_0010|Bit clear
|BTS     |0010_mmmm_ssss_0010|Bit test
|ADD     |iiii_iiii_dddd_1000|Add immediate
|LI      |iiii_iiii_dddd_1001|Load Immediate
|SB      |oooo_bbbo_ssss_1010|Store byte with offset
|LB      |oooo_bbbo_dddd_1011|Load byte with offset
|B       |oooo_oooo_cccc_1100|Branch relative (COND)
|BL      |oooo_oooo_cccc_1101|Branch relative and push return address to stack (COND)
|B       |oooo_oooo_oooo_1110|Branch relative
|BL      |oooo_oooo_oooo_1111|Branch relative and push return address to stack

ADD/LI/B.C/BL.C immediate format: imm[7:4]:imm[2:0]:imm[3]
SB/LB immediate format: imm[4:1]:imm[0]
B/BL immediate format: imm[11:4]:imm[2:0]:imm[3]

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

#### SWI - Software interrupt from register
Software interrupt with ID from register

#### PUSH - Push register to stack
Push register src to stack

#### POP - Pop register from stack
Pop from stack into register dst

C: x Z: m N: m V: x

#### B - Branch register
Branch to address in register pair base

#### BL - Branch register and push return address to stack
Push return address to stack and branch to address in register pair base

#### BST - Bit set flags
Set bit imm in flags register

C: m Z: m N: m V: m

#### BIC - Bit clear flags
Clear bit imm in flags register

C: m Z: m N: m V: m

#### BTS - Bit test flags
Test bit imm in flags register

C: x Z: m N: m V: x

#### B - Branch register (COND)
Branch register (COND)

#### BL - Branch register and push return address to stack (COND)
Branch register and push return address to stack (COND)

#### MOV - Move register to register
Move register src to register dst

C: x Z: m N: m V: x

#### BST - Bit set immediate
Set bit imm in register dst

C: x Z: m N: m V: x

#### BIC - Bit clear immediate
Clear bit imm in register dst

C: x Z: m N: m V: x

#### NOT - Not
Bitwise NOT src and store in dst

C: x Z: m N: m V: x

#### NEG - Negate
Two's complement negate src and store in dst

C: m Z: m N: m V: m

#### INC - Increment
Increment src by 1 and store in dst

C: m Z: m N: m V: m

#### DEC - Decrement
Decrement src by 1 and store in dst

C: m Z: m N: m V: m

#### MOV - Move csr to register
Move csr to register

C: x Z: m N: m V: x

#### MOV - Move register to csr
Move register to csr

#### INCC - Increment Carry
Increment src with Carry flag and store in dst

C: m Z: m N: m V: m

#### DECB - Decrement Borrow
Decrement src with Borrow flag and store in dst

C: m Z: m N: m V: m

#### CMP - Compare subtraction
Compare subtraction src1 and src2

C: m Z: m N: m V: m

#### CMA - Compare And
Compare And src1 and src2

C: x Z: m N: m V: x

#### SUB - Subtract
Subtract mod from dst and store at dst

C: m Z: m N: m V: m

#### SBB - Subtract with borrow
Subtract mod with borrow from dst and store at dst

C: m Z: m N: m V: m

#### ADD - Add
Add mod to dst and store at dst

C: m Z: m N: m V: m

#### ADC - Add with carry
Add mod with carry to dst and store at dst

C: m Z: m N: m V: m

#### AND - And
And mod and dst and store at dst

C: x Z: m N: m V: x

#### ANDN - And Not
And dst with bitwise NOT of mod and store at dst

C: x Z: m N: m V: x

#### OR - Or
Or mod and dst and store at dst

C: x Z: m N: m V: x

#### ORN - Or Not
Or dst with bitwise NOT of mod and store at dst

C: x Z: m N: m V: x

#### NOR - Not Or
Not Or mod and dst and store at dst

C: x Z: m N: m V: x

#### XOR - Exclusive or
Exclusive or mod and dst and store at dst

C: x Z: m N: m V: x

#### SLL - Shift Left Logical
Shift Left Logical dst by mod and store at dst

C: x Z: m N: m V: x

#### SRL - Shift Right Logical
Shift Right Logical dst by mod and store at dst

C: x Z: m N: m V: x

#### SRA - Shift Right Arithmetic
Shift Right Arithmetic dst by mod and store at dst

C: x Z: m N: m V: x

#### SLL - Shift Left Logical immediate
Shift Left Logical dst with immediate and store at dst

C: x Z: m N: m V: x

#### SRL - Shift Right Logical immediate
Shift Right Logical dst with immediate and store at dst

C: x Z: m N: m V: x

#### SRA - Shift Right Arithmetic immediate
Shift Right Arithmetic dst with immediate and store at dst

C: x Z: m N: m V: x

#### BTS - Bit test immediate
Test bit imm in register src

C: x Z: m N: m V: x

#### BST - Bit set
Set bit mod in register dst

C: x Z: m N: m V: x

#### BIC - Bit clear
Clear bit mod in register dst

C: x Z: m N: m V: x

#### BTS - Bit test
Test bit mod in register src

C: x Z: m N: m V: x

#### ADD - Add immediate
Add signed immediate to dst and store at dst

C: m Z: m N: m V: m

#### LI - Load Immediate
Load Immediate into dst

C: x Z: m N: m V: x

#### SB - Store byte with offset
Store src with base + offset in memory

#### LB - Load byte with offset
Load byte from base + offset in memory to dst

C: x Z: m N: m V: x

#### B - Branch relative (COND)
Branch relative (COND)

#### BL - Branch relative and push return address to stack (COND)
Branch relative and push return address to stack (COND)

#### B - Branch relative
Branch relative

#### BL - Branch relative and push return address to stack
Branch relative and push return address to stack
