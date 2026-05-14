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

	FILE *f = fopen(asm_file, "w");
	fprintf(f, "NOP\n");
	fprintf(f, "RET\n");
	fclose(f);

	struct TokenList *tl = calloc(1, sizeof(struct TokenList));
	tl->count = 0;
	bool err = lexer(asm_file, tl);
	assert(!err);
	(void)err;

	struct SymbolTable *st = calloc(1, sizeof(struct SymbolTable));
	st->count = 0;
	err = symbol_build_table(tl, st);
	assert(!err);

	FILE *fout = fopen(bin_file, "wb");
	assert(fout);
	err = encode_and_write(tl, st, fout);
	fclose(fout);
	assert(!err);

	// Check output
	FILE *fin = fopen(bin_file, "rb");
	assert(fin);
	uint16_t buffer[2];
	size_t read = fread(buffer, 2, 2, fin);
	assert(read == 2);
	(void)read;

	// NOP = 0x0000, RET = 0x2000
	assert(buffer[0] == 0x0000);
	assert(buffer[1] == 0x2000);

	fclose(fin);
	free(tl);
	free(st);
	remove(asm_file);
	remove(bin_file);
	printf("test_integration_basic passed\n");
}

int main()
{
	test_integration_basic();
	return 0;
}
