#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "structures.h"

void test_simple_tokens()
{
	const char *test_file = "test_simple.s";
	FILE *f = fopen(test_file, "w");
	fprintf(f, "MOV r0, r1\n");
	fprintf(f, "NOP\n");
	fprintf(f, "label: ADD r2, r3, r4\n");
	fclose(f);

	struct TokenList *tl = calloc(1, sizeof(struct TokenList));
	tl->count = 0;
	bool err = lexer(test_file, tl);
	assert(!err);
	(void)err;

	// Expected tokens:
	// MOV (INSTR)
	// r0 (REG)
	// , (COMMA)
	// r1 (REG)
	// NOP (INSTR)
	// label (SYMBOL)
	// : (COLON)
	// ADD (INSTR)
	// r2 (REG)
	// , (COMMA)
	// r3 (REG)
	// , (COMMA)
	// r4 (REG)

	assert(tl->tokens[0].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tl->tokens[0].str, "MOV") == 0);

	assert(tl->tokens[1].type == TOKEN_REGISTER);
	assert(tl->tokens[1].num_value == 0); // r0

	assert(tl->tokens[2].type == TOKEN_COMMA);

	assert(tl->tokens[3].type == TOKEN_REGISTER);
	assert(tl->tokens[3].num_value == 1); // r1

	assert(tl->tokens[4].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tl->tokens[4].str, "NOP") == 0);

	assert(tl->tokens[5].type == TOKEN_SYMBOL);
	assert(strcmp(tl->tokens[5].str, "label") == 0);

	assert(tl->tokens[6].type == TOKEN_COLON);

	assert(tl->tokens[7].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tl->tokens[7].str, "ADD") == 0);

	free(tl);
	remove(test_file);
	printf("test_simple_tokens passed\n");
}

void test_numbers_and_strings()
{
	const char *test_file = "test_data.s";
	FILE *f = fopen(test_file, "w");
	fprintf(f, "LI r0, 42\n");
	fprintf(f, ".asciz \"Hello\"\n");
	fclose(f);

	struct TokenList *tl = calloc(1, sizeof(struct TokenList));
	tl->count = 0;
	bool err = lexer(test_file, tl);
	assert(!err);
	(void)err;

	// Expected:
	// LI (INSTR)
	// r0 (REG)
	// , (COMMA)
	// 42 (NUMBER)
	// . (PERIOD)
	// asciz (SYMBOL)
	// "Hello" (STRING)

	assert(tl->tokens[3].type == TOKEN_NUMBER);
	assert(tl->tokens[3].num_value == 42);

	assert(tl->tokens[4].type == TOKEN_PERIOD);
	assert(tl->tokens[5].type == TOKEN_SYMBOL);
	assert(strcmp(tl->tokens[5].str, "asciz") == 0);
	assert(tl->tokens[6].type == TOKEN_STRING);
	assert(strcmp(tl->tokens[6].str, "Hello") == 0);

	free(tl);
	remove(test_file);
	printf("test_numbers_and_strings passed\n");
}

int main()
{
	test_simple_tokens();
	test_numbers_and_strings();
	return 0;
}
