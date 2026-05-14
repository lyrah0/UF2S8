#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "lexer.h"
#include "structures.h"
#include <strings.h>

void test_simple_tokens()
{
	const char *test_file = "test_simple.s";
	FILE *file_ptr = fopen(test_file, "w");
	assert(file_ptr);
	if (fprintf(file_ptr, "MOV r0, r1\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "NOP\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, "label: ADD r2, r3, r4\n") < 0) {
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
		(void)fprintf(stderr, "Lexer failed on %s\n", test_file);
		exit(1);
	}

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

	assert(tokens->tokens[0].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tokens->tokens[0].str, "MOV") == 0);

	assert(tokens->tokens[1].type == TOKEN_REGISTER);
	assert(tokens->tokens[1].num_value == 0); // r0

	assert(tokens->tokens[2].type == TOKEN_COMMA);

	assert(tokens->tokens[3].type == TOKEN_REGISTER);
	assert(tokens->tokens[3].num_value == 1); // r1

	assert(tokens->tokens[4].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tokens->tokens[4].str, "NOP") == 0);

	assert(tokens->tokens[5].type == TOKEN_SYMBOL);
	assert(strcmp(tokens->tokens[5].str, "label") == 0);

	assert(tokens->tokens[6].type == TOKEN_COLON);

	assert(tokens->tokens[7].type == TOKEN_INSTRUCTION);
	assert(strcasecmp(tokens->tokens[7].str, "ADD") == 0);

	free(tokens);
	if (remove(test_file) != 0) {
		perror("remove");
		exit(1);
	}
	if (printf("test_simple_tokens passed\n") < 0) {
		perror("printf");
		exit(1);
	}
}

void test_numbers_and_strings()
{
	const char *test_file = "test_data.s";
	FILE *file_ptr = fopen(test_file, "w");
	assert(file_ptr);
	if (fprintf(file_ptr, "LI r0, 42\n") < 0) {
		perror("fprintf");
		exit(1);
	}
	if (fprintf(file_ptr, ".asciz \"Hello\"\n") < 0) {
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
		(void)fprintf(stderr, "Lexer failed on %s\n", test_file);
		exit(1);
	}

	// Expected:
	// LI (INSTR)
	// r0 (REG)
	// , (COMMA)
	// 42 (NUMBER)
	// . (PERIOD)
	// asciz (SYMBOL)
	// "Hello" (STRING)

	assert(tokens->tokens[3].type == TOKEN_NUMBER);
	assert(tokens->tokens[3].num_value == 42);

	assert(tokens->tokens[4].type == TOKEN_PERIOD);
	assert(tokens->tokens[5].type == TOKEN_SYMBOL);
	assert(strcmp(tokens->tokens[5].str, "asciz") == 0);
	assert(tokens->tokens[6].type == TOKEN_STRING);
	assert(strcmp(tokens->tokens[6].str, "Hello") == 0);

	free(tokens);
	if (remove(test_file) != 0) {
		perror("remove");
		exit(1);
	}
	if (printf("test_numbers_and_strings passed\n") < 0) {
		perror("printf");
		exit(1);
	}
}

int main()
{
	test_simple_tokens();
	test_numbers_and_strings();
	return 0;
}
