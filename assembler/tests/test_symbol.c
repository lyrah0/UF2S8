#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "symbol.h"
#include "structures.h"

void test_symbol_table()
{
	const char *test_file = "test_symbols.s";
	FILE *f = fopen(test_file, "w");
	fprintf(f, "start:\n");
	fprintf(f, "  NOP\n");
	fprintf(f, "  B start\n");
	fprintf(f, "loop:\n");
	fprintf(f, "  ADD r0, r1, r2\n");
	fprintf(f, "  B loop\n");
	fclose(f);

	struct TokenList *tl = calloc(1, sizeof(struct TokenList));
	tl->count = 0;
	bool err = lexer(test_file, tl);
	assert(!err);

	struct SymbolTable *st = calloc(1, sizeof(struct SymbolTable));
	st->count = 0;
	err = symbol_build_table(tl, st);
	assert(!err);
	(void)err;

	// Expected symbols:
	// start at address 0
	// loop at address 4 (NOP is 2 bytes, B start is 2 bytes)

	assert(st->count == 2);

	int start_idx = -1, loop_idx = -1;
	for (int i = 0; i < st->count; i++) {
		if (strcmp(st->symbols[i].name, "start") == 0) {
			start_idx = i;
		}
		if (strcmp(st->symbols[i].name, "loop") == 0) { loop_idx = i; }
	}

	assert(start_idx != -1);
	assert(loop_idx != -1);
	(void)start_idx;
	(void)loop_idx;

	assert(st->symbols[start_idx].address == 0);
	assert(st->symbols[loop_idx].address == 4);

	free(tl);
	free(st);
	remove(test_file);
	printf("test_symbol_table passed\n");
}

int main()
{
	test_symbol_table();
	return 0;
}
