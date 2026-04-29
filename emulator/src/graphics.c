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

	SDL_UpdateTexture(
		viM->texture, nullptr, viM->memory + VRAM_START, SCREEN_WIDTH);
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

	viM->texture = SDL_CreateTexture(viM->renderer, SDL_PIXELFORMAT_RGB332,
		SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	if (!viM->texture) {
		printf("SDL_CreateTexture Error: %s\n", SDL_GetError());
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	if (!SDL_SetTextureScaleMode(viM->texture, SDL_SCALEMODE_NEAREST)) {
		printf("SDL_SetTextureScaleMode Error: %s\n", SDL_GetError());
		SDL_DestroyTexture(viM->texture);
		SDL_DestroyRenderer(viM->renderer);
		SDL_DestroyWindow(viM->window);
		SDL_Quit();
		return true;
	}

	SDL_StartTextInput(viM->window);
	return false;
}