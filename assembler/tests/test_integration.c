#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "symbol.h"
#include "encode.h"
#include "structures.h"

void test_integration_basic()
{
	const char *asm_file = "test_integration.s";
	const char *bin_file = "test_integration.bin";

	FILE *file_ptr = fopen(asm_file, "w");
	assert(file_ptr);
	if (fprintf(file_ptr, "NOP\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "RET\n") < 0) {
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
	if (lexer(asm_file, tokens)) {
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

	FILE *output_file = fopen(bin_file, "wb");
	assert(output_file);
	if (encode_and_write(tokens, symbols, output_file)) {
		(void)fprintf(stderr, "Encoding failed\n");
		exit(1);
	}
	if (fclose(output_file) != 0) {
		perror("fclose");
		exit(1);
	}

	// Check output
	FILE *input_file = fopen(bin_file, "rb");
	assert(input_file);
	uint16_t buffer[2];
	size_t bytes_read = fread(buffer, 2, 2, input_file);
	assert(bytes_read == 2);
	(void)bytes_read;

	// NOP = 0x0000, RET = 0x2000
	assert(buffer[0] == 0x0000);
	assert(buffer[1] == 0x2000);

	if (fclose(input_file) != 0) {
		perror("fclose");
		exit(1);
	}
	free(tokens);
	free(symbols);
	if (remove(asm_file) != 0) {
		perror("remove");
		exit(1);
	}
	if (remove(bin_file) != 0) {
		perror("remove");
		exit(1);
	}
	if (printf("test_integration_basic passed\n") < 0) {
		perror("printf");
		exit(1);
	}
}

int main()
{
	test_integration_basic();
	return 0;
}
