#pragma once
#include "structures.h"

const char *const instr_list[] = { "NOP", "RET", "RETI", "SWI", "POP", "PUSH",
	"MOV", "CMP", "CMA", "SUB", "SBB", "ADD", "ADC", "AND", "ANDN", "OR",
	"ORN", "NOR", "XOR", "SLL", "SRL", "SRA", "LI", "SB", "LB", "B", "BL",
	"INCC", "DECB", "WFI", "BST", "BIC", "BTS", "NOT", "NEG", "INC",
	"DEC" };

const struct registers registers[] = { { "r0", 0 }, { "r1", 1 }, { "r2", 2 },
	{ "r3", 3 }, { "r4", 4 }, { "r5", 5 }, { "r6", 6 }, { "r7", 7 },
	{ "r8", 8 }, { "r9", 9 }, { "r10", 10 }, { "r11", 11 }, { "r12", 12 },
	{ "r13", 13 }, { "r14", 14 }, { "r15", 15 }, { "a0", 20 },
	{ "a1", 21 }, { "a2", 22 }, { "a3", 23 }, { "a4", 24 }, { "a5", 25 },
	{ "a6", 26 }, { "a7", 27 }, { "flags", 40 }, { "vbr", 41 },
	{ "rng", 42 }, { "cycl", 52 }, { "cych", 53 }, { "spl", 54 },
	{ "sph", 55 } };

const struct conditions conditions[] = { { "ZS", 0 }, { "EQ", 0 }, { "ZC", 1 },
	{ "NE", 1 }, { "CS", 2 }, { "HS", 2 }, { "CC", 3 }, { "LO", 3 },
	{ "NS", 4 }, { "MI", 4 }, { "NC", 5 }, { "PL", 5 }, { "VS", 6 },
	{ "VC", 7 }, { "HI", 8 }, { "LS", 9 }, { "GE", 10 }, { "LT", 11 },
	{ "GT", 12 }, { "LE", 13 }, { "AS", 14 }, { "AC", 15 } };
