#include "memory.h"
#include "vm.h"
#include "graphics.h"
#include <stdint.h>
#include <stdio.h>

static void terminal_write(uint8_t value)
{
	putchar(value);
	(void)fflush(stdout);
}

uint16_t fetch_instruction(struct VirtualMachine *viM)
{
	uint16_t instruction =
		(memory_read(viM, (uint16_t)(viM->pc + 1)) << 8) |
		memory_read(viM, viM->pc);

	viM->pc += 2;

	return instruction;
}

void memory_dump(struct VirtualMachine *viM)
{
	if (viM->memory_dump) {
		for (int i = 0; i < MAX_MEMORY; i++) {
			printf("0x%04hx: %02hhx", (uint16_t)i,
				viM->memory[(uint16_t)i]);
			i++;
			while (i % 16 != 0) {
				printf(" %02hhx",
					viM->memory[(uint16_t)(i++)]);
			}
			printf("  ");
			while (i % 32 != 31) {
				printf(" %02hhx",
					viM->memory[(uint16_t)(i++)]);
			}
			printf(" %02hhx\n", viM->memory[(uint16_t)i]);
		}
	}
}

void memory_write(struct VirtualMachine *viM, uint16_t address, uint8_t value)
{
	if (address == HW_UART_DATA) {
		terminal_write(value);
	} else if (address == HW_BLIT_CMD) {
		execute_blit(viM, value);
	} else if (address == HW_GFX_ADDR_L) {
		viM->vram_ptr = (viM->vram_ptr & 0xFF00) | value;
	} else if (address == HW_GFX_ADDR_H) {
		viM->vram_ptr = (viM->vram_ptr & 0x00FF) |
			((uint16_t)value << 8);
	} else if (address == HW_GFX_DATA) {
		viM->vram[viM->vram_ptr] = value;
		if (viM->memory[HW_GFX_CTRL] & 0x4) { viM->vram_ptr++; }
	} else if (address >= HW_BANK_SEL_0 && address <= HW_BANK_SEL_7) {
		viM->bank_sel[address - HW_BANK_SEL_0] = value;
	} else if (address >= 0xFE00 && address <= 0xFEFF) {
		viM->memory[address] = value;
	} else {
		uint8_t win = address / BANK_SIZE;
		uint8_t bank = viM->bank_sel[win];
		uint16_t offset = address % BANK_SIZE;
		if (bank < 128) {
			// ROM: Read-only, ignore write
			return;
		}
		if (bank < 248) {
			viM->ram[((bank - 128) * BANK_SIZE) + offset] = value;
			return;
		}
		viM->vram[((bank - 248) * BANK_SIZE) + offset] = value;
	}
}

uint8_t memory_read(struct VirtualMachine *viM, uint16_t address)
{
	if (address == HW_KBD_DATA) {
		if (viM->key_head != viM->key_tail) {
			uint16_t event = viM->key_buffer[viM->key_head];
			viM->key_head = (viM->key_head + 1) % 64;
			return (uint8_t)(event & 0xFF);
		}
		return 0;
	}
	if (address == HW_KBD_STATUS) {
		uint8_t status = 0;
		if (viM->key_head != viM->key_tail) {
			status |= 0x01; // Ready
			uint16_t event = viM->key_buffer[viM->key_head];
			if ((event >> 8) & 1) {
				status |= 0x02; // Release
			}
		}
		return status;
	}
	if (address == HW_UART_DATA) {
		if (viM->uart_head != viM->uart_tail) {
			uint8_t data = viM->uart_buffer[viM->uart_head];
			viM->uart_head = (viM->uart_head + 1) % 64;
			return data;
		}
		return 0;
	}
	if (address == HW_UART_STATUS) {
		uint8_t status = 0x02; // TX Empty always set
		if (viM->uart_head != viM->uart_tail) {
			status |= 0x01; // RX Ready
		}
		return status;
	}
	if (address == HW_GFX_ADDR_L) {
		return (uint8_t)(viM->vram_ptr & 0xFF);
	}
	if (address == HW_GFX_ADDR_H) { return (uint8_t)(viM->vram_ptr >> 8); }
	if (address == HW_GFX_DATA) {
		uint8_t value = viM->vram[viM->vram_ptr];
		if (viM->memory[HW_GFX_CTRL] & 0x8) { viM->vram_ptr++; }
		return value;
	}
	if (address >= HW_BANK_SEL_0 && address <= HW_BANK_SEL_7) {
		return viM->bank_sel[address - HW_BANK_SEL_0];
	}
	if (address >= 0xFE00 && address <= 0xFEFF) {
		return viM->memory[address];
	}
	uint8_t win = address / BANK_SIZE;
	uint8_t bank = viM->bank_sel[win];
	uint16_t offset = address % BANK_SIZE;
	if (bank < 128) { return viM->rom[(bank * BANK_SIZE) + offset]; }
	if (bank < 248) {
		return viM->ram[((bank - 128) * BANK_SIZE) + offset];
	}
	return viM->vram[((bank - 248) * BANK_SIZE) + offset];
}