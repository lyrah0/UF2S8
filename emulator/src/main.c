#include <SDL3/SDL_init.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <unistd.h> //NOLINT
#include <bits/getopt_core.h> //NOLINT
#define XOPEN_SOURCE 700
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <termios.h>

#include "vm.h"
#include "cpu.h"
#include "memory.h"
#include "io.h"
#include "debugger.h"
#include "graphics.h"

static bool main_loop(struct VirtualMachine *viM)
{
	viM->running = true;
	viM->pc = 0;
	viM->bp_count = 0;
	viM->memory[0xFEFF] = 0;
	for (int i = 0; i < 8; i++) {
		viM->csr[i] = 0;
	}
	uint16_t instruction = 0;
	unsigned int timer = 0;

	while (viM->running) {
		if (instruction == 0x0000 && viM->pc > 0x1f) {
			viM->running = false;
		}

		interrupt_timer(viM, &timer);
		interrupt_input(viM, &timer);

		instruction = fetch_instruction(viM);
		for (int i = 0; i < viM->bp_count; i++) {
			if (viM->pc == viM->breakpoint[i]) {
				printf("\nHit breakpoint at 0x%04x\n",
					viM->pc);
				viM->debug_mode = true;
				break;
			}
		}

		if (viM->debug_mode) { debug_prompt(viM, instruction); }

		if (decode_execute(viM, instruction)) {
			printf("ERROR: execution failed\n");
			return true;
		}
		viM->csr[0] &= 0x8F;
		for (int i = 1; i < 6; i++) {
			viM->csr[i] = 0;
		}

		if (timer % 10000 == 0) { update_graphics(viM); }

		timer++;
	}
	memory_dump(viM);
	print_state(viM, instruction);
	return false;
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
	struct VirtualMachine viM;
	viM.debug_mode = false;
	viM.memory_dump = false;
	viM.graphics = false;

	while ((opt = getopt(argc, argv, "gmdi:o:")) != -1) {
		switch (opt) {
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
			printf("Usage: %s [-i input file] [-m] [-d]\n",
				argv[0]);
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

	if (!fread(&viM.memory, MAX_MEMORY, 1, finput)) {
		// printf("ERROR: Failed to read file.\n");
	}

	if (viM.graphics) {
		if (init_sdl(&viM)) {
			printf("ERROR: failed to initialize SDL\n");
			return 1;
		}
	}

	(void)fclose(finput);

	struct termios oldt;
	struct termios newt;
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	if (main_loop(&viM)) {
		printf("ERROR: execution failed\n");
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
		return 1;
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

	if (viM.graphics) {
		SDL_DestroyTexture(viM.texture);
		SDL_DestroyRenderer(viM.renderer);
		SDL_DestroyWindow(viM.window);
		SDL_Quit();
	}

	return 0;
}
