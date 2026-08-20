#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

enum {
	HW_BLIT_SRC_X_L = 0xFEC0,
	HW_BLIT_SRC_X_H = 0xFEC1,
	HW_BLIT_SRC_Y_L = 0xFEC2,
	HW_BLIT_SRC_Y_H = 0xFEC3,
	HW_BLIT_DST_X_L = 0xFEC4,
	HW_BLIT_DST_X_H = 0xFEC5,
	HW_BLIT_DST_Y_L = 0xFEC6,
	HW_BLIT_DST_Y_H = 0xFEC7,
	HW_BLIT_CLIP_X_MIN_L = 0xFEC8,
	HW_BLIT_CLIP_X_MIN_H = 0xFEC9,
	HW_BLIT_CLIP_Y_MIN_L = 0xFECA,
	HW_BLIT_CLIP_Y_MIN_H = 0xFECB,
	HW_BLIT_CLIP_X_MAX_L = 0xFECC,
	HW_BLIT_CLIP_X_MAX_H = 0xFECD,
	HW_BLIT_CLIP_Y_MAX_L = 0xFECE,
	HW_BLIT_CLIP_Y_MAX_H = 0xFECF,
	HW_BLIT_SRC_STRIDE_L = 0xFED0,
	HW_BLIT_SRC_STRIDE_H = 0xFED1,
	HW_BLIT_DST_STRIDE_L = 0xFED2,
	HW_BLIT_DST_STRIDE_H = 0xFED3,
	HW_BLIT_WIDTH_L = 0xFED4,
	HW_BLIT_WIDTH_H = 0xFED5,
	HW_BLIT_HEIGHT_L = 0xFED6,
	HW_BLIT_HEIGHT_H = 0xFED7,
	HW_BLIT_ALPHA = 0xFEDC,
	HW_BLIT_FLAGS = 0xFEDD,
	HW_BLIT_COLOR = 0xFEDE,
	HW_BLIT_CMD = 0xFEDF,
	HW_GFX_ADDR_L = 0xFEE0,
	HW_GFX_ADDR_H = 0xFEE1,
	HW_GFX_WIDTH = 0xFEE2,
	HW_GFX_HEIGHT = 0xFEE3,
	HW_GFX_DATA = 0xFEEE,
	HW_GFX_CTRL = 0xFEEF,
	HW_PALETTE_START = 0xFFF0,
	HW_KBD_DATA = 0xFEF1,
	HW_KBD_STATUS = 0xFEF2,
	HW_UART_DATA = 0xFEF3,
	HW_UART_STATUS = 0xFEF4,
	HW_UART_CTRL = 0xFEF5,
	HW_BANK_SEL_0 = 0xFEF6,
	HW_BANK_SEL_1 = 0xFEF7,
	HW_BANK_SEL_2 = 0xFEF8,
	HW_BANK_SEL_3 = 0xFEF9,
	HW_BANK_SEL_4 = 0xFEFA,
	HW_BANK_SEL_5 = 0xFEFB,
	HW_BANK_SEL_6 = 0xFEFC,
	HW_TIMER_HZ = 0xFEFE,
	HW_HW_CTRL = 0xFEFF
};

enum {
	MAX_MEMORY = 1 << 16,
	MAX_BREAKPOINTS = 1 << 4,
	BANK_SIZE = 0x2000,
	NUM_WINDOWS = 7,
	NUM_ROM_BANKS = 128,
	NUM_RAM_BANKS = 120,
	NUM_VRAM_BANKS = 8,
	TOTAL_BANKS = 256,
	ROM_SIZE = NUM_ROM_BANKS * BANK_SIZE, // 1024KB
	RAM_SIZE = NUM_RAM_BANKS * BANK_SIZE, // 960KB
	VRAM_SIZE = 1 << 16, // 64KB (8 banks of 8KB)
};

struct VirtualMachine {
	uint8_t memory[MAX_MEMORY];
	uint8_t rom[ROM_SIZE];
	uint8_t ram[RAM_SIZE];
	uint8_t bank_sel[NUM_WINDOWS];
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
	SDL_Texture *dynamic_texture;
	int current_res_w;
	int current_res_h;
	SDL_Palette *sdl_palette;
	uint8_t vram[VRAM_SIZE];
	uint16_t vram_ptr;
	uint8_t processed_vram[912 * 570];
	uint16_t key_buffer[64];
	int key_head;
	int key_tail;
	uint8_t uart_buffer[64];
	int uart_head;
	int uart_tail;
};
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static inline int16_t sign_extend(uint16_t value, uint8_t bits)
{
	uint16_t shift = 16 - bits;
	return (int16_t)((int16_t)(value << shift) >> shift);
}
