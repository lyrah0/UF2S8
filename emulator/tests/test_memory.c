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

	// Test basic RAM in Window 7 (address > 0xE000)
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

	// Test Window 7 (0xE000 - 0xFFFF) mapped to Bank 247
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
	memory_write(viM, HW_GFX_CTRL, 0x0C); // Both bits set
	memory_write(viM, HW_GFX_DATA, 0xEF);
	assert(viM->vram[0x1235] == 0xEF);
	assert(viM->vram_ptr == 0x1236);

	uint8_t val = memory_read(
		viM, HW_GFX_DATA); // Read from 0x1236, then increment
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
