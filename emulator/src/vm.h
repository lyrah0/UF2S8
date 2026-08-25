#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

enum {
	HW_TIMER_HZ = 0xFFF0,
	HW_HW_CTRL = 0xFFF1,
	HW_HW_STATUS = 0xFFF2,
	HW_KBD_DATA = 0xFFF3,
	HW_UART_STATUS = 0xFFF4,
	HW_UART_DATA = 0xFFF5,
	HW_BANK_SEL_0 = 0xFFF8,
	HW_BANK_SEL_1 = 0xFFF9,
	HW_BANK_SEL_2 = 0xFFFA,
	HW_BANK_SEL_3 = 0xFFFB,
	HW_BANK_SEL_4 = 0xFFFC,
	HW_BANK_SEL_5 = 0xFFFD,
	HW_BANK_SEL_6 = 0xFFFE,
	HW_BANK_SEL_7 = 0xFFFF
};

enum {
	MAX_MEMORY = 1 << 16,
	MAX_BREAKPOINTS = 1 << 4,
	BANK_SIZE = 0x2000,
	NUM_WINDOWS = 8,
	NUM_ROM_BANKS = 128,
	NUM_RAM_BANKS = 120,
	NUM_VRAM_BANKS = 8,
	TOTAL_BANKS = 256,
	ROM_SIZE = NUM_ROM_BANKS * BANK_SIZE, // 1024KB
	RAM_SIZE = NUM_RAM_BANKS * BANK_SIZE, // 960KB
	VRAM_SIZE = 1 << 16, // 64KB (8 banks of 8KB)
};

struct VirtualMachine {
	uint8_t hw_regs[16];
	uint8_t rom[ROM_SIZE];
	uint8_t ram[RAM_SIZE];
	uint8_t bank_sel[NUM_WINDOWS];
	const uint8_t *bank_read_ptr[NUM_WINDOWS];
	uint8_t *bank_write_ptr[NUM_WINDOWS];
	uint8_t gpr[16];
	uint8_t csr[16];
	uint16_t pc;
	uint16_t breakpoint[MAX_BREAKPOINTS];
	int bp_count;
	volatile bool running;
	bool debug_mode;
	bool memory_dump;
	bool graphics;
	bool print_stats;
	bool wait_for_interrupt;
	SDL_Thread *cpu_thread;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *dynamic_texture;
	int current_res_w;
	int current_res_h;
	SDL_Palette *sdl_palette;
	uint8_t vram[VRAM_SIZE];
	uint8_t processed_vram[896 * 704];
	uint16_t key_buffer[64];
	int key_head;
	int key_tail;
	uint8_t uart_buffer[64];
	int uart_head;
	int uart_tail;
	volatile bool vsync_pending;
	uint16_t lfsr;
	int current_scanline;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
