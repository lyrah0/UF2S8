#include "memory.h"
#include "vm.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void terminal_write(uint8_t value)
{
	putchar(value);
	(void)fflush(stdout);
}

void memory_set_bank(struct VirtualMachine *viM, uint8_t win, uint8_t bank)
{
	if (win >= NUM_WINDOWS) { return; }
	viM->bank_sel[win] = bank;
	if (bank < 128) {
		viM->bank_read_ptr[win] =
			&viM->rom[(ptrdiff_t)(bank * BANK_SIZE)];
		viM->bank_write_ptr[win] = nullptr;
	} else if (bank < 248) {
		viM->bank_read_ptr[win] =
			&viM->ram[(ptrdiff_t)((bank - 128) * BANK_SIZE)];
		viM->bank_write_ptr[win] =
			&viM->ram[(ptrdiff_t)((bank - 128) * BANK_SIZE)];
	} else {
		viM->bank_read_ptr[win] =
			&viM->vram[(ptrdiff_t)((bank - 248) * BANK_SIZE)];
		viM->bank_write_ptr[win] =
			&viM->vram[(ptrdiff_t)((bank - 248) * BANK_SIZE)];
	}
}

void memory_init(struct VirtualMachine *viM)
{
	for (uint8_t i = 0; i < 7; i++) {
		memory_set_bank(viM, i, i);
	}
	memory_set_bank(viM, 7, 247);
}

uint16_t fetch_instruction(struct VirtualMachine *viM)
{
	uint16_t pc = viM->pc;
	viM->pc = (uint16_t)(pc + 2);

	uint8_t win = pc >> 13;
	uint16_t offset = pc & 0x1FFF;
	const uint8_t *ptr = viM->bank_read_ptr[win];
	if (ptr && offset < BANK_SIZE - 1) {
		return (uint16_t)ptr[offset] |
			((uint16_t)ptr[offset + 1] << 8);
	}

	return (uint16_t)memory_read(viM, pc) |
		((uint16_t)memory_read(viM, (uint16_t)(pc + 1)) << 8);
}

void memory_dump(struct VirtualMachine *viM)
{
	if (viM->memory_dump) {
		for (int i = 0; i < MAX_MEMORY; i++) {
			printf("0x%04hx: %02hhx", (uint16_t)i,
				memory_read(viM, (uint16_t)i));
			i++;
			while (i % 16 != 0) {
				printf(" %02hhx",
					memory_read(viM, (uint16_t)(i++)));
			}
			printf("  ");
			while (i % 32 != 31) {
				printf(" %02hhx",
					memory_read(viM, (uint16_t)(i++)));
			}
			printf(" %02hhx\n", memory_read(viM, (uint16_t)i));
		}
	}
}

void memory_write(struct VirtualMachine *viM, uint16_t address, uint8_t value)
{
	if (address < 0xFFF0) {
		uint8_t win = address >> 13;
		uint16_t offset = address & 0x1FFF;
		uint8_t *ptr = viM->bank_write_ptr[win];
		if (ptr) { ptr[offset] = value; }
		return;
	}

	viM->hw_regs[address - 0xFFF0] = value;

	switch (address) {
	case HW_UART_DATA:
		terminal_write(value);
		break;
	case HW_BANK_SEL_0:
	case HW_BANK_SEL_1:
	case HW_BANK_SEL_2:
	case HW_BANK_SEL_3:
	case HW_BANK_SEL_4:
	case HW_BANK_SEL_5:
	case HW_BANK_SEL_6:
	case HW_BANK_SEL_7:
		memory_set_bank(
			viM, (uint8_t)(address - HW_BANK_SEL_0), value);
		break;
	default:
		break;
	}
}

uint8_t memory_read(struct VirtualMachine *viM, uint16_t address)
{
	if (address < 0xFFF0) {
		uint8_t win = address >> 13;
		uint16_t offset = address & 0x1FFF;
		const uint8_t *ptr = viM->bank_read_ptr[win];
		return ptr ? ptr[offset] : 0;
	}

	switch (address) {
	case HW_HW_STATUS: {
		uint8_t status = 0x08; // UART transmitter empty (Always 1)
		if (viM->uart_head != viM->uart_tail) {
			status |= 0x04; // UART receiver ready
		}
		if (viM->key_head != viM->key_tail) {
			status |= 0x01; // Keyboard ready
			uint16_t event = viM->key_buffer[viM->key_head];
			if ((event >> 8) & 1) {
				status |= 0x02; // Keyboard release
			}
		}
		return status;
	}
	case HW_KBD_DATA:
		if (viM->key_head != viM->key_tail) {
			uint16_t event = viM->key_buffer[viM->key_head];
			viM->key_head = (viM->key_head + 1) % 64;
			return (uint8_t)(event & 0xFF);
		}
		return 0;
	case HW_UART_STATUS: {
		uint8_t status = 0x02; // TX Empty always set
		if (viM->uart_head != viM->uart_tail) {
			status |= 0x01; // RX Ready
		}
		return status;
	}
	case HW_UART_DATA:
		if (viM->uart_head != viM->uart_tail) {
			uint8_t data = viM->uart_buffer[viM->uart_head];
			viM->uart_head = (viM->uart_head + 1) % 64;
			return data;
		}
		return 0;
	case HW_BANK_SEL_0:
	case HW_BANK_SEL_1:
	case HW_BANK_SEL_2:
	case HW_BANK_SEL_3:
	case HW_BANK_SEL_4:
	case HW_BANK_SEL_5:
	case HW_BANK_SEL_6:
	case HW_BANK_SEL_7:
		return viM->bank_sel[address - HW_BANK_SEL_0];
	default:
		return viM->hw_regs[address - 0xFFF0];
	}
}