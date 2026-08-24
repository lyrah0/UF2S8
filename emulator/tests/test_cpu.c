#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cpu.h"
#include "vm.h"
#include "isa.h"
#include "memory.h"

static inline uint16_t pack_imm8_mid(uint8_t imm)
{
	return (uint16_t)((imm & 0xF0) | ((imm & 0x07) << 1) |
		((imm >> 3) & 0x01));
}

static inline uint16_t pack_imm5_mem(uint8_t imm)
{ return (uint16_t)((((imm >> 1) & 0x0F) << 12) | ((imm & 0x01) << 8)); }

static inline uint16_t pack_imm12_branch(uint16_t imm)
{
	return (uint16_t)((((imm >> 4) & 0xFF) << 8) | ((imm & 0x07) << 5) |
		(((imm >> 3) & 0x01) << 4));
}

static void test_li()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// LI r0, 42
	uint16_t inst =
		(uint16_t)(0x0009 | (0 << 4) | (pack_imm8_mid(42) << 8));
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 42);

	// LI r1, (uint8_t)-5
	inst = (uint16_t)(0x0009 | (1 << 4) |
		(pack_imm8_mid((uint8_t)-5) << 8));
	decode_execute(viM, inst);
	assert((int8_t)viM->gpr[1] == -5);

	free(viM);
	(void)printf("test_li passed\n");
}

static void test_arithmetic()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	viM->gpr[0] = 10;
	viM->gpr[1] = 20;

	// ADD r0, r1 (r0 = r0 + r1 = 30) -> 0x2101
	uint16_t inst = 0x2101;
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 30);
	assert(!(viM->csr[0] & 0x02)); // Zero flag not set

	// SUB r0, r0 (r0 = r0 - r0 = 0) -> 0x0001
	inst = 0x0001;
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 0);
	assert(viM->csr[0] & 0x02); // Zero flag set

	// ADD r0, 15 (r0 = r0 + 15 = 15) -> 0x0008 | (0 << 4) | (pack_imm8_mid(15) << 8)
	inst = (uint16_t)(0x0008 | (0 << 4) | (pack_imm8_mid(15) << 8));
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 15);

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

	// AND r0, r1 -> 0b10001000
	uint16_t inst = 0x4101;
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 0b10001000);

	// OR r0, r1 -> 0b10001000 | 0b10101010 = 0b10101010
	inst = 0x6101;
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 0b10101010);

	// XOR r0, r1 -> 0
	inst = 0x9101;
	decode_execute(viM, inst);
	assert(viM->gpr[0] == 0);

	// SLL imm: r1 = r1 << 2
	inst = (uint16_t)(0xD001 | (2 << 9) | (1 << 4));
	decode_execute(viM, inst);
	assert(viM->gpr[1] == (uint8_t)(0b10101010 << 2));

	free(viM);
	(void)printf("test_logic_shifts passed\n");
}

static void test_stack()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// Set stack pointer to 0xEFFF (in RAM Window 7)
	viM->csr[0xF] = 0xEF;
	viM->csr[0xE] = 0xFF;

	// Push r0 (value 0x77) -> 0x1200
	viM->gpr[0] = 0x77;
	uint16_t inst = 0x1200;
	decode_execute(viM, inst);

	// Pop into r1 -> 0x1310
	inst = 0x1310;
	decode_execute(viM, inst);
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

	// B +4 (unconditional relative 2 words) -> 0x000E | pack_imm12_branch(2)
	uint16_t inst = (uint16_t)(0x000E | pack_imm12_branch(2));
	decode_execute(viM, inst);
	assert(viM->pc == 0x1004);

	free(viM);
	(void)printf("test_branch passed\n");
}

static void test_v2_new_instructions()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// NOT r2, r3
	viM->gpr[3] = 0x0F;
	uint16_t inst = 0x6320;
	decode_execute(viM, inst);
	assert(viM->gpr[2] == 0xF0);

	// NEG r4, r2
	inst = 0x7240;
	decode_execute(viM, inst);
	assert(viM->gpr[4] == 0x10);

	// INC r5, r4
	inst = 0x8450;
	decode_execute(viM, inst);
	assert(viM->gpr[5] == 0x11);

	// DEC r6, r5
	inst = 0x9560;
	decode_execute(viM, inst);
	assert(viM->gpr[6] == 0x10);

	// BST r6, 3 (set bit 3)
	inst = (uint16_t)(0x4000 | (3 << 9) | (6 << 4));
	decode_execute(viM, inst);
	assert(viM->gpr[6] == 0x18);

	// BIC r6, 4 (clear bit 4)
	inst = (uint16_t)(0x4100 | (4 << 9) | (6 << 4));
	decode_execute(viM, inst);
	assert(viM->gpr[6] == 0x08);

	// LB / SB with a0
	viM->gpr[0] = 0x00; // a0 = r1:r0 = 0xE000 (RAM)
	viM->gpr[1] = 0xE0;
	viM->gpr[7] = 0xAB;
	// SB r7, [a0 + 4]
	inst = (uint16_t)(0x000A | (7 << 4) | (0 << 9) | pack_imm5_mem(4));
	decode_execute(viM, inst);
	assert(memory_read(viM, 0xE004) == 0xAB);

	// LB r8, [a0 + 4]
	inst = (uint16_t)(0x000B | (8 << 4) | (0 << 9) | pack_imm5_mem(4));
	decode_execute(viM, inst);
	assert(viM->gpr[8] == 0xAB);

	free(viM);
	(void)printf("test_v2_new_instructions passed\n");
}

int main()
{
	test_li();
	test_arithmetic();
	test_logic_shifts();
	test_stack();
	test_branch();
	test_v2_new_instructions();
	return 0;
}
