#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "memory.h"
#include "vm.h"

static void test_basic_memory()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);

	// Test basic RAM (address > 0xE000)
	memory_write(viM, 0xF000, 0xAA);
	assert(memory_read(viM, 0xF000) == 0xAA);

	memory_write(viM, 0xFFFF, 0x55);
	assert(memory_read(viM, 0xFFFF) == 0x55);

	free(viM);
	(void)printf("test_basic_memory passed\n");
}

static void test_banking()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);

	// Test Window 0 (0x0000 - 0x7FFF) with banking
	// Bank 0
	memory_write(viM, HW_BANK_SEL, 0x00);
	memory_write(viM, 0x1000, 0x01);
	assert(memory_read(viM, 0x1000) == 0x01);

	// Bank 1
	memory_write(viM, HW_BANK_SEL, 0x01);
	memory_write(viM, 0x1000, 0x02);
	assert(memory_read(viM, 0x1000) == 0x02);

	// Check Bank 0 again
	memory_write(viM, HW_BANK_SEL, 0x00);
	assert(memory_read(viM, 0x1000) == 0x01);

	// Test Window 1 (0x8000 - 0xBFFF)
	// Bank 0
	memory_write(viM, HW_BANK_SEL, 0x00);
	memory_write(viM, 0x9000, 0x10);
	assert(memory_read(viM, 0x9000) == 0x10);

	// Bank 1 (bits 4-6 of bank_select)
	memory_write(viM, HW_BANK_SEL, 0x10);
	memory_write(viM, 0x9000, 0x20);
	assert(memory_read(viM, 0x9000) == 0x20);

	memory_write(viM, HW_BANK_SEL, 0x00);
	assert(memory_read(viM, 0x9000) == 0x10);

	free(viM);
	(void)printf("test_banking passed\n");
}

static void test_vram_access()
{
	struct VirtualMachine *viM = calloc(1, sizeof(struct VirtualMachine));
	assert(viM);

	// Set VRAM address to 0x1234
	memory_write(viM, HW_GFX_ADDR_L, 0x34);
	memory_write(viM, HW_GFX_ADDR_H, 0x12);
	assert(viM->vram_ptr == 0x1234);

	// Write to VRAM via HW_GFX_DATA
	memory_write(viM, HW_GFX_DATA, 0xBE);
	assert(viM->vram[0x1234] == 0xBE);

	// Read from VRAM via HW_GFX_DATA
	assert(memory_read(viM, HW_GFX_DATA) == 0xBE);

	// Test auto-increment (needs HW_GFX_CTRL bit 2 for write, bit 3 for read)
	viM->memory[HW_GFX_CTRL] = 0x0C; // Both bits set
	memory_write(viM, HW_GFX_DATA, 0xEF);
	assert(viM->vram[0x1235] == 0xEF);
	assert(viM->vram_ptr == 0x1236);

	uint8_t val = memory_read(viM, HW_GFX_DATA); // Read from 0x1236, then increment
	assert(val == 0); // VRAM was 0 at 0x1236
	(void)val;
	assert(viM->vram_ptr == 0x1237);

	free(viM);
	(void)printf("test_vram_access passed\n");
}

int main()
{
	test_basic_memory();
	test_banking();
	test_vram_access();
	return 0;
}
