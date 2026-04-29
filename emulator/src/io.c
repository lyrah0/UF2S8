#include "io.h"
#include "vm.h"
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int get_keypress_nonblocking()
{
	// Save current file flags
	int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	// Apply flags with non-blocking added
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	int character = getchar();

	// Restore blocking behaviour
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	return character;
}

void interrupt_pushtostack(struct VirtualMachine *viM)
{
	uint16_t stackp = viM->csr[7] << 8 | viM->csr[6];

	viM->memory[stackp--] = viM->pc >> 8;
	viM->memory[stackp--] = viM->pc;
	viM->memory[stackp--] = viM->csr[0];

	viM->csr[7] = stackp >> 8;
	viM->csr[6] = stackp;
	viM->csr[0] &= 0x7F;
}

void interrupt_timer(struct VirtualMachine *viM, const unsigned int *timer)
{
	uint16_t timer_mult = viM->memory[0xFEFE] << 8 | 0xFF;
	if (!(*timer % timer_mult == 0 && (viM->csr[0] >> 7) == 1)) { return; }

	uint8_t hardware_control = viM->memory[0xFEFF];
	bool timer_enabled = (hardware_control & 0x1) != 0;

	if (timer_enabled) {
		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF06;
		viM->pc = viM->memory[vector_addr] |
			viM->memory[vector_addr + 1] << 8;
	}
}

void interrupt_input(struct VirtualMachine *viM, const unsigned int *timer)
{
	if (!(*timer % 1000 == 0 && (viM->csr[0] >> 7) == 1)) { return; }

	int character = -1;

	// Check SDL buffer first
	if (viM->sdl_buf_head != viM->sdl_buf_tail) {
		character = viM->sdl_input_buffer[viM->sdl_buf_head];
		viM->sdl_buf_head = (viM->sdl_buf_head + 1) % 16;
	} else {
		// Fallback to terminal
		character = get_keypress_nonblocking();
	}

	if (character != -1) {
		viM->memory[HW_KEYBOARD_DATA] = (uint8_t)character;

		interrupt_pushtostack(viM);

		uint16_t vector_addr = 0xFF06;
		viM->pc = viM->memory[vector_addr] |
			viM->memory[vector_addr + 1] << 8;
	}
}
