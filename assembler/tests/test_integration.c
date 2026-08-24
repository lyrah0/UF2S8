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

	// NOP = 0x0000, RET = 0x0010 in ISA V2
	assert(buffer[0] == 0x0000);
	assert(buffer[1] == 0x0010);

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

void test_integration_v2()
{
	const char *asm_file = "test_v2.s";
	const char *bin_file = "test_v2.bin";

	FILE *file_ptr = fopen(asm_file, "w");
	assert(file_ptr);
	fprintf(file_ptr, "MOV r1, r2\n");
	fprintf(file_ptr, "MOV r0, spl\n");
	fprintf(file_ptr, "MOV spl, r0\n");
	fprintf(file_ptr, "ADD r3, r4\n");
	fprintf(file_ptr, "LI r5, 0x12\n");
	fprintf(file_ptr, "NOT r6, r7\n");
	fprintf(file_ptr, "INC r8\n");
	fprintf(file_ptr, "BST r9, 3\n");
	fprintf(file_ptr, "BST flags, 4\n");
	fprintf(file_ptr, "B a1\n");
	fprintf(file_ptr, "SUB r1, r2\n");
	fprintf(file_ptr, "ANDN r3, r4\n");
	fprintf(file_ptr, "ORN r5, r6\n");
	fprintf(file_ptr, "SLL r7, 2\n");
	fprintf(file_ptr, "SRL r8, 3\n");
	fprintf(file_ptr, "SRA r9, 1\n");
	fprintf(file_ptr, "BTS r10, 5\n");
	fprintf(file_ptr, "BIC r11, r12\n");
	fprintf(file_ptr, "LB r13, [a3 + 4]\n");
	fprintf(file_ptr, "SB r14, [a7]\n");
	fprintf(file_ptr, "B EQ, a2\n");
	fprintf(file_ptr, "BL NE, [a4]\n");
	fclose(file_ptr);

	struct TokenList *tokens = calloc(1, sizeof(struct TokenList));
	assert(tokens);
	tokens->count = 0;
	assert(lexer(asm_file, tokens) == 0);

	struct SymbolTable *symbols = calloc(1, sizeof(struct SymbolTable));
	assert(symbols);
	symbols->count = 0;
	assert(symbol_build_table(tokens, symbols) == 0);

	FILE *output_file = fopen(bin_file, "wb");
	assert(output_file);
	assert(encode_and_write(tokens, symbols, output_file) == 0);
	fclose(output_file);

	FILE *input_file = fopen(bin_file, "rb");
	assert(input_file);
	uint16_t buffer[22];
	size_t bytes_read = fread(buffer, 2, 22, input_file);
	assert(bytes_read == 22);
	(void)bytes_read;

	assert(buffer[0] == 0x3210); // MOV r1, r2
	assert(buffer[1] == 0xAE00); // MOV r0, spl
	assert(buffer[2] == 0xB0E0); // MOV spl, r0
	assert(buffer[3] == 0x2431); // ADD r3, r4
	assert(buffer[4] == 0x1459); // LI r5, 0x12
	assert(buffer[5] == 0x6760); // NOT r6, r7
	assert(buffer[6] == 0x8880); // INC r8
	assert(buffer[7] == 0x4690); // BST r9, 3
	assert(buffer[8] == 0x1E80); // BST flags, 4
	assert(buffer[9] == 0x1D20); // B a1
	assert(buffer[10] == 0x0211); // SUB r1, r2
	assert(buffer[11] == 0x5431); // ANDN r3, r4
	assert(buffer[12] == 0x7651); // ORN r5, r6
	assert(buffer[13] == 0xD471); // SLL r7, 2
	assert(buffer[14] == 0xE691); // SRL r8, 3
	assert(buffer[15] == 0xF291); // SRA r9, 1
	assert(buffer[16] == 0xFAB1); // BTS r10, 5
	assert(buffer[17] == 0x1CB2); // BIC r11, r12
	assert(buffer[18] == 0x26DB); // LB r13, [a3 + 4]
	assert(buffer[19] == 0x0EEA); // SB r14, [a7]
	assert(buffer[20] == 0x2400); // B EQ, a2
	assert(buffer[21] == 0x2910); // BL NE, [a4]

	fclose(input_file);
	free(tokens);
	free(symbols);
	remove(asm_file);
	remove(bin_file);
	printf("test_integration_v2 passed\n");
}

int main()
{
	test_integration_basic();
	test_integration_v2();
	return 0;
}
