#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

enum {
	VRAM_START = 0x8000,
	VRAM_SIZE = 0x4000,
	VRAM_END = VRAM_START + VRAM_SIZE - 1,
	SCREEN_WIDTH = 128,
	SCREEN_HEIGHT = 128
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
	SDL_Thread *cpu_thread;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	uint8_t sdl_input_buffer[16];
	int sdl_buf_head;
	int sdl_buf_tail;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
