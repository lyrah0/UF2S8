#include "graphics.h"
#include "vm.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

void handle_graphics_events(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) { viM->running = false; }
		if (event.type == SDL_EVENT_TEXT_INPUT) {
			for (const char *p = event.text.text; *p != '\0';
				p++) {
				int next = (viM->sdl_buf_tail + 1) % 16;
				if (next != viM->sdl_buf_head) {
					viM->sdl_input_buffer[viM
							->sdl_buf_tail] =
						(uint8_t)*p;
					viM->sdl_buf_tail = next;
				}
			}
		} else if (event.type == SDL_EVENT_KEY_DOWN) {
			uint8_t c = 0;
			switch (event.key.key) {
			case SDLK_RETURN:
				c = '\n';
				break;
			case SDLK_BACKSPACE:
				c = '\b';
				break;
			case SDLK_TAB:
				c = '\t';
				break;
			case SDLK_ESCAPE:
				c = '\x1b';
				break;
			}
			if (c != 0) {
				int next = (viM->sdl_buf_tail + 1) % 16;
				if (next != viM->sdl_buf_head) {
					viM->sdl_input_buffer[viM
							->sdl_buf_tail] = c;
					viM->sdl_buf_tail = next;
				}
			}
		}
	}
}

void render_graphics_frame(struct VirtualMachine *viM)
{
	if (!viM->graphics) { return; }

	uint8_t mode = viM->memory[HW_GFX_CONTROL] & 0x03;
	uint8_t *source_data = viM->memory + VRAM_START;

	SDL_Color colors[256];
	if (mode < 3) {
		for (int i = 0; i < 16; i++) {
			uint8_t val = viM->memory[HW_PALETTE_START + i];
			colors[i].r = (uint8_t)((val >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((val >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((val & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, 16);
	} else {
		for (int i = 0; i < 256; i++) {
			colors[i].r = (uint8_t)((i >> 5) * 255 / 7);
			colors[i].g = (uint8_t)(((i >> 2) & 0x07) * 255 / 7);
			colors[i].b = (uint8_t)((i & 0x03) * 255 / 3);
			colors[i].a = 255;
		}
		SDL_SetPaletteColors(viM->sdl_palette, colors, 0, 256);
	}

	if (mode == 0) {
		for (int i = 0; i < 2048; i++) {
			uint8_t b = source_data[i];
			for (int j = 0; j < 8; j++) {
				viM->processed_vram[(i * 8) + j] =
					(b >> (7 - j)) & 1;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode == 1) {
		for (int i = 0; i < 4096; i++) {
			uint8_t b = source_data[i];
			for (int j = 0; j < 4; j++) {
				viM->processed_vram[(i * 4) + j] =
					(b >> ((3 - j) * 2)) & 3;
			}
		}
		source_data = viM->processed_vram;
	} else if (mode == 2) {
		for (int i = 0; i < 8192; i++) {
			uint8_t b = source_data[i];
			viM->processed_vram[(ptrdiff_t)(i * 2)] = (b >> 4) & 0x0F;
			viM->processed_vram[(i * 2) + 1] = b & 0x0F;
		}
		source_data = viM->processed_vram;
	}

	SDL_UpdateTexture(viM->texture, nullptr, source_data, 128);
	SDL_RenderClear(viM->renderer);
	SDL_RenderTexture(viM->renderer, viM->texture, nullptr, nullptr);
	SDL_RenderPresent(viM->renderer);
}

bool init_sdl(struct VirtualMachine *viM) // NOLINT
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		printf("SDL_Init Error: %s\n", SDL_GetError());
		return true;
	}

	viM->window = SDL_CreateWindow(
		"UF2S8 Emulator", SCREEN_WIDTH * 8, SCREEN_HEIGHT * 8, 0);
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

	viM->texture = SDL_CreateTexture(viM->renderer, SDL_PIXELFORMAT_INDEX8,
		SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!viM->texture) {
		printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	if (!SDL_SetTexturePalette(viM->texture, viM->sdl_palette)) {
		printf("SDL_SetTexturePalette Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	if (!SDL_SetTextureScaleMode(viM->texture, SDL_SCALEMODE_NEAREST)) {
		printf("Warning: failed to set nearest neighbor scaling\n");
	}

	SDL_StartTextInput(viM->window);
	return false;
}