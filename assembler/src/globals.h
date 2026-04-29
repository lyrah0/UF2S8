#pragma once
#include "structures.h"

const char *const instr_list[] = { "NOP", "RET", "RETI", "SWI", "POP", "PUSH",
	"MOV", "CMP", "CMN", "CMA", "SUB", "SBC", "ADD", "ADC", "AND", "OR",
	"NOR", "XOR", "SLL", "SRL", "SRA", "LI", "ADD", "SB", "LB", "B", "BL",
	"INCC", "DECB" };

const struct registers registers[] = { { "r0", 0 }, { "r1", 1 }, { "r2", 2 },
	{ "r3", 3 }, { "r4", 4 }, { "r5", 5 }, { "r6", 6 }, { "r7", 7 },
	{ "a0", 10 }, { "a1", 11 }, { "a2", 12 }, { "a3", 13 },
	{ "flags", 20 }, { "spl", 26 }, { "sph", 27 } };

const struct conditions conditions[] = { { "ZS", 0 }, { "EQ", 0 }, { "ZC", 1 },
	{ "NE", 1 }, { "CS", 2 }, { "HS", 2 }, { "CC", 3 }, { "LO", 3 },
	{ "NS", 4 }, { "MI", 4 }, { "NC", 5 }, { "PL", 5 }, { "VS", 6 },
	{ "AL", 7 } };
