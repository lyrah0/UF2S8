#include "graphics.h"
#include "vm.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static const int MODE_RES[4] = { MODE0_RES, MODE1_RES, MODE2_RES, MODE3_RES };
static const int MODE_VRAM_BYTES[4] = {
	MODE0_RES * MODE0_RES / 8, // 1-bpp: 8192
	MODE1_RES *MODE1_RES / 4, // 2-bpp: 8100
	MODE2_RES *MODE2_RES / 2, // 4-bpp: 8192
	MODE3_RES *MODE3_RES, // 8-bpp: 8100
};

void handle_graphics_events(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			viM->running = false;
		} else if (event.type == SDL_EVENT_KEY_DOWN ||
			event.type == SDL_EVENT_KEY_UP) {
			bool released = (event.type == SDL_EVENT_KEY_UP);
			uint8_t scancode = (uint8_t)event.key.scancode;

			int next = (viM->key_tail + 1) % 64;
			if (next != viM->key_head) {
				viM->key_buffer[viM->key_tail] =
					((uint16_t)released << 8) | scancode;
				viM->key_tail = next;
			}
		}
	}
}

void render_graphics_frame(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	uint8_t mode = viM->memory[HW_GFX_CONTROL] & 0x03;
	uint8_t *source_data = viM->memory + VRAM_START;
	int res = MODE_RES[mode];
	SDL_Color colors[256];

	if (mode < 3) {
		int pal_size = 1 << (1 << mode);
		for (int i = 0; i < pal_size; i++) {
			uint8_t val = viM->memory[HW_PALETTE_START + i];
			colors[i].r = (uint8_t)((val >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((val >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((val & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, pal_size);
	} else {
		for (int i = 0; i < 256; i++) {
			colors[i].r = (uint8_t)((i >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((i >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((i & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, 256);
	}

	int total_pixels = res * res;
	int vram_bytes = MODE_VRAM_BYTES[mode];
	if (mode == 0) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 8; j++) {
				viM->processed_vram[(i * 8) + j] =
					(vbyte >> (7 - j)) & 0x01;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode == 1) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			for (int j = 0; j < 4; j++) {
				viM->processed_vram[(i * 4) + j] =
					(vbyte >> ((3 - j) * 2)) & 0x03;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode == 2) {
		for (int i = 0; i < vram_bytes; i++) {
			uint8_t vbyte = source_data[i];
			viM->processed_vram[(ptrdiff_t)(i * 2)] =
				(vbyte >> 4) & 0x0F;
			viM->processed_vram[(ptrdiff_t)(i * 2) + 1] = vbyte &
				0x0F;
		}
		source_data = viM->processed_vram;
	}
	(void)total_pixels;

	SDL_UpdateTexture(viM->textures[mode], nullptr, source_data, res);
	SDL_RenderClear(viM->renderer);
	SDL_RenderTexture(
		viM->renderer, viM->textures[mode], nullptr, nullptr);
	SDL_RenderPresent(viM->renderer);
}

bool init_sdl(struct VirtualMachine *viM) // NOLINT
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init Error: %s\n", SDL_GetError());
		return true;
	}

	viM->window = SDL_CreateWindow(
		"UF2S8 Emulator", DISPLAY_SIZE, DISPLAY_SIZE, 0);
	if (!viM->window) {
		printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
		SDL_Quit();
		return true;
	}

	viM->renderer = SDL_CreateRenderer(viM->window, nullptr);
	if (!viM->renderer) {
		printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	SDL_SetRenderVSync(viM->renderer, 1);

	viM->sdl_palette = SDL_CreatePalette(256);
	if (!viM->sdl_palette) {
		printf("SDL_CreatePalette Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	for (int i = 0; i < 4; i++) {
		int res = MODE_RES[i];
		viM->textures[i] = SDL_CreateTexture(viM->renderer,
			SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING,
			res, res);
		if (!viM->textures[i]) {
			printf("SDL_CreateTexture Error (mode %d): %s\n", i,
				SDL_GetError());
			SDL_DestroyRenderer(viM->renderer);
			SDL_DestroyWindow(viM->window);
			SDL_Quit();
			return true;
		}
		if (!SDL_SetTexturePalette(
			    viM->textures[i], viM->sdl_palette)) {
			printf("SDL_SetTexturePalette Error (mode %d): %s\n",
				i, SDL_GetError());
			return true;
		}
		if (!SDL_SetTextureScaleMode(
			    viM->textures[i], SDL_SCALEMODE_NEAREST)) {
			printf("Warning: failed to set nearest neighbor scaling" " for mode %d\n",
				i);
		}
	}

	SDL_StartTextInput(viM->window);
	return false;
}