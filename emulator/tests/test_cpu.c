#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cpu.h"
#include "vm.h"
#include "isa.h"

static void test_li()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);

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

	free(viM);
	(void)printf("test_arithmetic passed\n");
}

static void test_branch()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);

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
	test_branch();
	return 0;
}
