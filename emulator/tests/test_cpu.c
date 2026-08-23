#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cpu.h"
#include "vm.h"
#include "isa.h"
#include "memory.h"

static void test_li()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// LI r0, 42
	Instruction inst = { 0 };
	inst.li.op = 0x0A;
	inst.li.imm = 42;
	inst.li.dst = 0;

	decode_execute(viM, inst.raw);
	assert(viM->gpr[0] == 42);

	// LI r1, -5 (8-bit immediate)
	inst.li.dst = 1;
	inst.li.imm = (uint8_t)-5;
	decode_execute(viM, inst.raw);
	assert((int8_t)viM->gpr[1] == -5);

	free(viM);
	(void)printf("test_li passed\n");
}

static void test_arithmetic()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// LI r0, 10
	// LI r1, 20
	// ADD r2, r0, r1
	viM->gpr[0] = 10;
	viM->gpr[1] = 20;

	Instruction inst = { 0 };
	inst.op = 0x21; // ADD
	inst.src = 0;
	inst.mod = 1;
	inst.dst = 2;

	decode_execute(viM, inst.raw);
	assert(viM->gpr[2] == 30);
	assert(!(viM->csr[0] & 0x02)); // Zero flag not set

	// Test zero flag
	// SUB r3, r0, r0
	inst.op = 0x01; // SUB
	inst.src = 0;
	inst.mod = 0;
	inst.dst = 3;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[3] == 0);
	assert(viM->csr[0] & 0x02); // Zero flag set

	// Test ADDI
	inst.raw = 0;
	inst.addi.op = 0x0B;
	inst.addi.src = 0;
	inst.addi.dst = 4;
	inst.addi.imm0 = 5 & 0x07;
	inst.addi.imm1 = (5 >> 3) & 0x07;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[4] == 15);

	free(viM);
	(void)printf("test_arithmetic passed\n");
}

static void test_logic_shifts()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	viM->gpr[0] = 0b11001100;
	viM->gpr[1] = 0b10101010;

	// AND r2, r0, r1 -> 0b10001000
	Instruction inst = { 0 };
	inst.op = 0x41;
	inst.src = 0;
	inst.mod = 1;
	inst.dst = 2;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[2] == 0b10001000);

	// OR r3, r0, r1 -> 0b11101110
	inst.op = 0x51;
	inst.dst = 3;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[3] == 0b11101110);

	// XOR r4, r0, r1 -> 0b01100110
	inst.op = 0x71;
	inst.dst = 4;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[4] == 0b01100110);

	// SLL imm: r5 = r0 << 2
	inst.raw = 0;
	inst.op = 0x42;
	inst.src = 0;
	inst.mod = 2;
	inst.dst = 5;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[5] == (uint8_t)(0b11001100 << 2));

	free(viM);
	(void)printf("test_logic_shifts passed\n");
}

static void test_stack()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// Set stack pointer to 0xEFFF (in RAM Window 7)
	viM->csr[7] = 0xEF;
	viM->csr[6] = 0xFF;

	// Push r0 (value 0x77)
	viM->gpr[0] = 0x77;
	Instruction inst = { 0 };
	inst.ss.op = 0x080;
	inst.ss.op1 = 0x1; // PUSH
	inst.src = 0;
	decode_execute(viM, inst.raw);

	// Pop into r1
	inst.raw = 0;
	inst.sd.op = 0x0100; // POP
	inst.dst = 1;
	decode_execute(viM, inst.raw);
	assert(viM->gpr[1] == 0x77);

	free(viM);
	(void)printf("test_stack passed\n");
}

static void test_branch()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	viM->pc = 0x1000;

	// B +4 (relative)
	Instruction inst = { 0 };
	inst.branch.op = 0x0E; // B
	inst.branch.offset = 2; // offset is in words (2 bytes)
	inst.branch.cond = 7; // AL (Always)

	decode_execute(viM, inst.raw);
	// PC should be 0x1000 + (2 * 2) = 0x1004
	assert(viM->pc == 0x1004);

	free(viM);
	(void)printf("test_branch passed\n");
}

int main()
{
	test_li();
	test_arithmetic();
	test_logic_shifts();
	test_stack();
	test_branch();
	return 0;
}
