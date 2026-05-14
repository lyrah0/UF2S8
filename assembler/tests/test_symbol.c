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
	FILE *file_ptr = fopen(test_file, "w");
	assert(file_ptr);
	if (fprintf(file_ptr, "start:\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "  NOP\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "  B start\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "loop:\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "  ADD r0, r1, r2\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "  B loop\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fclose(file_ptr) != 0) {
		perror("fclose");
		exit(1);
	}

	struct TokenList *tokens = calloc(1, sizeof(struct TokenList));
	assert(tokens);
	tokens->count = 0;
	if (lexer(test_file, tokens)) {
		(void)fprintf(stderr, "Lexer failed\n");
		exit(1);
	}

	struct SymbolTable *symbols = calloc(1, sizeof(struct SymbolTable));
	assert(symbols);
	symbols->count = 0;
	if (symbol_build_table(tokens, symbols)) {
		(void)fprintf(stderr, "Symbol table build failed\n");
		exit(1);
	}

	// Expected symbols:
	// start at address 0
	// loop at address 4 (NOP is 2 bytes, B start is 2 bytes)

	assert(symbols->count == 2);

	int start_idx = -1;
	int loop_idx = -1;
	for (int i = 0; i < symbols->count; i++) {
		if (strcmp(symbols->symbols[i].name, "start") == 0) {
			start_idx = i;
		}
		if (strcmp(symbols->symbols[i].name, "loop") == 0) {
			loop_idx = i;
		}
	}

	assert(start_idx != -1);
	assert(loop_idx != -1);
	(void)start_idx;
	(void)loop_idx;

	assert(symbols->symbols[start_idx].address == 0);
	assert(symbols->symbols[loop_idx].address == 4);

	free(tokens);
	free(symbols);
	if (remove(test_file) != 0) {
		perror("remove");
		exit(1);
	}
	if (printf("test_symbol_table passed\n") < 0) {
		perror("printf");
		exit(1);
	}
}

int main()
{
	test_symbol_table();
	return 0;
}
