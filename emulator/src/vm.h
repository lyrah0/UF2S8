#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

enum {
	VRAM_START = 0xC000,
	VRAM_SIZE = 0x2000,
	VRAM_END = VRAM_START + VRAM_SIZE - 1,
	MODE0_RES = 256, // 1-bpp
	MODE1_RES = 180, // 2-bpp
	MODE2_RES = 128, // 4-bpp
	MODE3_RES = 90, // 8-bpp (RGB332)
	DISPLAY_SIZE = 768,
	HW_PALETTE_START = 0xFEE0,
	HW_TERMINAL_OUT = 0xFEF0,
	HW_KEYBOARD_DATA = 0xFEF1,
	HW_KEYBOARD_STATUS = 0xFEF2,
	HW_GFX_CONTROL = 0xFEFD,
	HW_TIMER_MULT = 0xFEFE,
	HW_HARDWARE_CONTROL = 0xFEFF
};

enum { MAX_MEMORY = 1 << 16, MAX_BREAKPOINTS = 1 << 4 };

struct VirtualMachine {
	uint8_t memory[MAX_MEMORY];
	uint8_t gpr[8];
	uint8_t csr[8];
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
	SDL_Texture *textures[4];
	SDL_Palette *sdl_palette;
	uint8_t processed_vram[MODE0_RES * MODE0_RES];
	uint16_t key_buffer[64];
	int key_head;
	int key_tail;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
