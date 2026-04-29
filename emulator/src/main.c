#include <SDL3/SDL_error.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_thread.h>
#include <unistd.h> //NOLINT
#include <bits/getopt_core.h> //NOLINT
#define XOPEN_SOURCE 700
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#include "vm.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"
#include "debugger.h"
#include "graphics.h"

static int cpu_thread_worker(void *data)
{
	struct VirtualMachine *viM = (struct VirtualMachine *)data;
	viM->wait_for_interrupt = false;
	viM->pc = 0;
	viM->bp_count = 0;
	viM->memory[HW_HW_CTRL] = 0;
	// Default to RGB332 mode
	viM->memory[HW_GFX_CTRL] = 0;
	for (int i = 0; i < 8; i++) {
		viM->csr[i] = 0;
	}
	viM->bank_select = 0;
	uint16_t instruction = 0;
	uint64_t instruction_count = 0;
	uint64_t start_ticks = SDL_GetPerformanceCounter();
	uint64_t ticks_ns = 0;
	uint64_t loop_count = 0;

	while (viM->running) {
		// Get ticks only once every 1000 instructions to reduce overhead
		if (loop_count % 1000 == 0 || viM->wait_for_interrupt) {
			ticks_ns = SDL_GetTicksNS();
			interrupt_timer(viM, ticks_ns);
			interrupt_input(viM);
		}
		if (!viM->wait_for_interrupt) {
			instruction = fetch_instruction(viM);
		}
		for (int i = 0; i < viM->bp_count; i++) {
			if (viM->pc == viM->breakpoint[i]) {
				printf("\nHit breakpoint at 0x%04x\n",
					viM->pc);
				viM->debug_mode = true;
				break;
			}
		}

		if (viM->debug_mode) { debug_prompt(viM, instruction); }

		if (!viM->wait_for_interrupt) {
			if (decode_execute(viM, instruction)) {
				printf("ERROR: execution failed\n");
				viM->running = false;
				break;
			}
			viM->csr[0] &= 0x8F;
			for (int i = 1; i < 6; i++) {
				viM->csr[i] = 0;
			}
			instruction_count++;
		} else {
			// Wait for 1000000/255 microseconds
			usleep((unsigned int)(1000000 / 255) - 500);
		}
		loop_count++;
	}

	uint64_t end_ticks = SDL_GetPerformanceCounter();
	uint64_t raw_freq = SDL_GetPerformanceFrequency();
	double freq = (double)raw_freq;
	double duration = (double)(end_ticks - start_ticks) / freq;

	if (duration > 0 && instruction_count > 0 && viM->print_stats) {
		printf("\n--- Performance Statistics ---\n");
		printf("Instructions executed: %" PRIu64 "\n",
			instruction_count);
		printf("Elapsed time:          %.4f seconds\n", duration);
		printf("Average speed:         %.3f MIPS\n",
			((double)instruction_count / duration) / 1000000.0);
		printf("------------------------------\n");
	}

	memory_dump(viM);
	print_state(viM, instruction);
	return 0;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("ERROR: no argument given.\n");
		return 1;
	}

	FILE *finput = nullptr;
	char *ifilepath = nullptr;

	int opt = 0;
	static struct VirtualMachine viM;
	viM.debug_mode = false;
	viM.memory_dump = false;
	viM.graphics = false;
	viM.print_stats = false;
	viM.running = false;
	viM.key_head = 0;
	viM.key_tail = 0;

	while ((opt = getopt(argc, argv, "pgmdi:o:")) != -1) {
		switch (opt) {
		case 'p':
			viM.print_stats = true;
			break;
		case 'g':
			viM.graphics = true;
			break;
		case 'm':
			viM.memory_dump = true;
			break;
		case 'd':
			viM.debug_mode = true;
			break;
		case 'i':
			ifilepath = optarg;
			break;
		default:
			printf("Usage: %s [-i input file] [-m] [-d] [-g]\n",
				argv[0]);
			return 1;
		}
	}

	if (ifilepath) {
		finput = fopen(ifilepath, "r");
	} else {
		printf("ERROR: no input file\n");
		return 1;
	}
	if (finput == NULL) {
		printf("ERROR: failed to open input file\n");
		return 1;
	}

	// Read Window 0 banks
	(void)fread(&viM.ext_memory_w0, EXT_MEMORY_W0_SIZE, 1, finput);
	// Read Window 1 banks
	(void)fread(&viM.ext_memory_w1, EXT_MEMORY_W1_SIZE, 1, finput);
	// Read Fixed region into memory starting at 0xC000
	(void)fread(&viM.memory[0xC000], 0x4000, 1, finput);

	(void)fclose(finput);

	if (viM.graphics) {
		if (init_sdl(&viM)) {
			printf("ERROR: failed to initialize SDL\n");
			return 1;
		}
	}

	viM.running = true;
	viM.cpu_thread =
		SDL_CreateThread(cpu_thread_worker, "CPUThread", &viM);
	if (!viM.cpu_thread) {
		printf("ERROR: Failed to create CPU thread: %s\n",
			SDL_GetError());
		return 1;
	}

	while (viM.running) {
		handle_graphics_events(&viM);
		render_graphics_frame(&viM);
		SDL_Delay(1);
	}

	SDL_WaitThread(viM.cpu_thread, nullptr);

	if (viM.graphics) {
		if (viM.dynamic_texture) {
			SDL_DestroyTexture(viM.dynamic_texture);
		}
		SDL_DestroyPalette(viM.sdl_palette);
		SDL_DestroyRenderer(viM.renderer);
		SDL_DestroyWindow(viM.window);
		SDL_Quit();
	}

	return 0;
}
