#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memory.h"
#include "vm.h"

static void test_basic_memory()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// Test basic RAM in Window 7 (0xE000 - 0xFFEF)
	memory_write(viM, 0xF000, 0xAA);
	assert(memory_read(viM, 0xF000) == 0xAA);

	memory_write(viM, 0xFFEE, 0x55);
	assert(memory_read(viM, 0xFFEE) == 0x55);

	free(viM);
	(void)printf("test_basic_memory passed\n");
}

static void test_banking()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// Test default Window 0 mapped to Bank 0 (ROM bank 0)
	// ROM is read-only: writes should be ignored
	memory_write(viM, 0x1000, 0x55);
	assert(memory_read(viM, 0x1000) == 0x00);

	// Test Window 0 (0x0000 - 0x1FFF) mapped to RAM Bank 128
	memory_write(viM, HW_BANK_SEL_0, 128);
	memory_write(viM, 0x1000, 0x01);
	assert(memory_read(viM, 0x1000) == 0x01);

	// Switch Window 0 to RAM Bank 129
	memory_write(viM, HW_BANK_SEL_0, 129);
	assert(memory_read(viM, 0x1000) == 0x00);
	memory_write(viM, 0x1000, 0x02);
	assert(memory_read(viM, 0x1000) == 0x02);

	// Switch back to Bank 128
	memory_write(viM, HW_BANK_SEL_0, 128);
	assert(memory_read(viM, 0x1000) == 0x01);

	// Test Window 4 (0x8000 - 0x9FFF) mapped to RAM Bank 130
	memory_write(viM, HW_BANK_SEL_4, 130);
	memory_write(viM, 0x8500, 0x42);
	assert(memory_read(viM, 0x8500) == 0x42);

	// Test Window 7 (0xE000 - 0xFFEF) mapped to Bank 247
	memory_write(viM, HW_BANK_SEL_7, 247);
	memory_write(viM, 0xE500, 0x99);
	assert(memory_read(viM, 0xE500) == 0x99);

	// Test Window 1 mapped to Bank 247 seeing same data as Window 7
	memory_write(viM, HW_BANK_SEL_1, 247);
	assert(memory_read(viM, 0x2500) == 0x99);

	// Switch Window 7 to RAM Bank 131
	memory_write(viM, HW_BANK_SEL_7, 131);
	assert(memory_read(viM, 0xE500) == 0x00);
	memory_write(viM, 0xE500, 0x33);
	assert(memory_read(viM, 0xE500) == 0x33);

	// Switch Window 7 back to Bank 247
	memory_write(viM, HW_BANK_SEL_7, 247);
	assert(memory_read(viM, 0xE500) == 0x99);

	// Test Window 2 mapped to Bank 248 (VRAM Bank 0)
	memory_write(viM, HW_BANK_SEL_2, 248);
	memory_write(viM, 0x4000, 0x77);
	assert(viM->vram[0] == 0x77);
	assert(memory_read(viM, 0x4000) == 0x77);

	free(viM);
	(void)printf("test_banking passed\n");
}

static void test_hw_registers()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);
	memory_init(viM);

	// Test TIMER_HZ register (0xFFF0)
	memory_write(viM, HW_TIMER_HZ, 60);
	assert(memory_read(viM, HW_TIMER_HZ) == 60);

	// Test HW_CTRL register (0xFFF1)
	memory_write(viM, HW_HW_CTRL, 0xC1);
	assert(memory_read(viM, HW_HW_CTRL) == 0xC1);

	// Test HW_STATUS (0xFFF2): UART TX empty always 1 (bit 3 = 0x08)
	uint8_t status = memory_read(viM, HW_HW_STATUS);
	assert((status & 0x08) != 0);

	// Test BANK_SEL registers (0xFFF8 - 0xFFFF)
	for (int i = 0; i < 8; i++) {
		memory_write(viM, (uint16_t)(HW_BANK_SEL_0 + i), 100 + i);
		assert(memory_read(viM, (uint16_t)(HW_BANK_SEL_0 + i)) ==
			100 + i);
		assert(viM->bank_sel[i] == 100 + i);
	}

	free(viM);
	(void)printf("test_hw_registers passed\n");
}

int main()
{
	test_basic_memory();
	test_banking();
	test_hw_registers();
	return 0;
}
