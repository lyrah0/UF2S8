#include "structures.h"
#include "handle.h"
#include <stdint.h>
#include <stdio.h>
#include <strings.h>

// NOLINTBEGIN(readability-function-cognitive-complexity)
static bool encode_instructions(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, FILE *foutput, int *current_address,
	int *current_token)
{
	struct Token *token = &tokenList->tokens[*current_token];
	uint16_t machine_code = 0;
	if (!strcasecmp(token->str, "NOP")) {
		machine_code = 0x0000;
	} else if (!strcasecmp(token->str, "RET")) {
		machine_code = 0x2000;
	} else if (!strcasecmp(token->str, "RETI")) {
		machine_code = 0x0400;
	} else if (!strcasecmp(token->str, "SWI")) {
		if (handle_spp(
			    tokenList, current_token, &machine_code, 0x1400)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "INCC")) {
		if (handle_spp(
			    tokenList, current_token, &machine_code, 0x0800)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "DECB")) {
		if (handle_spp(
			    tokenList, current_token, &machine_code, 0x0C00)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "POP")) {
		if (handle_spp(
			    tokenList, current_token, &machine_code, 0x1800)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "PUSH")) {
		if (handle_spp(
			    tokenList, current_token, &machine_code, 0x1C00)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "MOV")) {
		if (handle_mov(tokenList, current_token, &machine_code)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "CMP")) {
		if (handle_crr(
			    tokenList, current_token, &machine_code, 0x0010)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "CMN")) {
		if (handle_crr(
			    tokenList, current_token, &machine_code, 0x0090)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "CMA")) {
		if (handle_crr(
			    tokenList, current_token, &machine_code, 0x0110)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SUB")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0001)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SBC")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0011)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "ADD")) {
		if (handle_add(tokenList, symbolTable, current_token,
			    &machine_code)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "ADC")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0031)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "AND")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0041)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "OR")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0051)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "NOR")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0061)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "XOR")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0071)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SLL")) {
		if (handle_shift(
			    tokenList, current_token, &machine_code, 0x0002)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SRL")) {
		if (handle_shift(
			    tokenList, current_token, &machine_code, 0x0012)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SRA")) {
		if (handle_shift(
			    tokenList, current_token, &machine_code, 0x0022)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "LI")) {
		if (handle_li(tokenList, symbolTable, current_token,
			    &machine_code)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SB")) {
		if (handle_loadstore(tokenList, symbolTable, current_token,
			    &machine_code, 0x000A)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "LB")) {
		if (handle_loadstore(tokenList, symbolTable, current_token,
			    &machine_code, 0x000B)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "B")) {
		if (handle_branch_cond(tokenList, symbolTable, current_token,
			    &machine_code, 0x000C, *current_address)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "BL")) {
		if (handle_branch_cond(tokenList, symbolTable, current_token,
			    &machine_code, 0x000E, *current_address)) {
			goto error;
		}
	} else {
		printf("ERROR: %d: unknown instruction %s\n", token->line,
			token->str);
		return true;
	}
	if (*current_address % 2) {
		char empty = 0;
		if (!fwrite(&empty, 1, 1, foutput)) {
			printf("ERROR: Failed to write output\n");
			return true;
		}
	}
	if (!fwrite(&machine_code, 2, 1, foutput)) {
		printf("ERROR: Failed to write output\n");
		return true;
	}
	*current_address += 2;
	return false;
error:
	printf("ERROR: %d: failed to encode %s\n", token->line, token->str);
	return true;
} // NOLINTEND(readability-function-cognitive-complexity)

static bool encode_directive_asciz(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			(*current_address) += token->len + 1;
			for (int i = 0; i < token->len + 1; i++) {
				if (!fwrite(&token->str[i], 1, 1, foutput)) {
					printf("ERROR: failed to write "
					       "output\n");
					return true;
				}
			}
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool encode_directive_ascii(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			(*current_address) += token->len;
			for (int i = 0; i < token->len; i++) {
				if (!fwrite(&token->str[i], 1, 1, foutput)) {
					printf("ERROR: failed to write "
					       "output\n");
					return true;
				}
			}
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool encode_directive_dsize(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token, int size)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_NUMBER) {
			(*current_address) += size;
			if (token->num_value > (1LL << (8 * size)) - 1) {
				printf("Warning: %d: value %lld truncated",
					token->line, token->num_value);
			}
			if (!fwrite(&token->num_value, size, 1, foutput)) {
				printf("ERROR: failed to write output\n");
				return true;
			}
		} else if (token->type == TOKEN_STRING) {
			(*current_address) += token->len;
			for (int i = 0; i < token->len; i++) {
				if (!fwrite(&token->str[i], 1, 1, foutput)) {
					printf("ERROR: failed to write "
					       "output\n");
					return true;
				}
			}
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool encode_directive_align(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token)
{
	*current_token += 2;
	if (*current_token >= tokenList->count) { return false; }
	struct Token *token = &tokenList->tokens[*current_token];
	if (token->type != TOKEN_NUMBER) {
		printf("ERROR: %d: align expects a number\n", token->line);
		return true;
	}
	int align = (int)token->num_value;
	if (align <= 0) {
		printf("ERROR: %d: align must be positive\n", token->line);
		return true;
	}
	if (align % 2) {
		printf("ERROR: %d: align must be a power of 2\n", token->line);
		return true;
	}
	int pad = (align - (*current_address % align)) % align;
	for (int i = 0; i < pad; i++) {
		const int empty = 0;
		if (!fwrite(&empty, 1, 1, foutput)) {
			printf("ERROR: failed to write output\n");
			return true;
		}
	}
	*current_address += pad;
	return false;
}

static bool encode_directive_origin(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token)
{
	if (*current_token + 2 >= tokenList->count) { return false; }
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	if (next2->type != TOKEN_NUMBER) { return false; }
	const int empty = 0;
	for (; *current_address < next2->num_value; (*current_address)++) {
		if (!fwrite(&empty, 1, 1, foutput)) {
			printf("ERROR: failed to write output\n");
			return true;
		}
	}
	*current_token += 2;

	return false;
}

static bool encode_directives(struct TokenList *tokenList, FILE *foutput,
	int *current_address, int *current_token)
{
	struct Token *next = &tokenList->tokens[*current_token + 1];
	if (next->type != TOKEN_SYMBOL) {
		printf("ERROR: %d: directive expects a symbol\n", next->line);
		return true;
	}
	if (strcasecmp(next->str, "origin") == 0) {
		return encode_directive_origin(
			tokenList, foutput, current_address, current_token);
	}
	if (strcasecmp(next->str, "db") == 0 ||
		strcasecmp(next->str, "byte") == 0) {
		return encode_directive_dsize(
			tokenList, foutput, current_address, current_token, 1);
	}
	if (strcasecmp(next->str, "dh") == 0 ||
		strcasecmp(next->str, "half") == 0) {
		return encode_directive_dsize(
			tokenList, foutput, current_address, current_token, 2);
	}
	if (strcasecmp(next->str, "dw") == 0 ||
		strcasecmp(next->str, "word") == 0) {
		return encode_directive_dsize(
			tokenList, foutput, current_address, current_token, 4);
	}
	if (strcasecmp(next->str, "ascii") == 0) {
		return encode_directive_ascii(
			tokenList, foutput, current_address, current_token);
	}
	if (strcasecmp(next->str, "asciz") == 0) {
		return encode_directive_asciz(
			tokenList, foutput, current_address, current_token);
	}
	if (strcasecmp(next->str, "align") == 0) {
		return encode_directive_align(
			tokenList, foutput, current_address, current_token);
	}

	return false;
}

bool encode_and_write(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, FILE *foutput)
{
	int current_address = 0;
	for (int current_token = 0; current_token < tokenList->count;
		current_token++) {
		struct Token *token = &tokenList->tokens[current_token];
		if (token->type == TOKEN_PERIOD &&
			tokenList->count > current_token + 1) {
			if (encode_directives(tokenList, foutput,
				    &current_address, &current_token)) {
				printf("ERROR: directive encoding failed!\n");
				goto error;
			}
		} else if (token->type == TOKEN_INSTRUCTION) {
			if (encode_instructions(tokenList, symbolTable,
				    foutput, &current_address,
				    &current_token)) {
				printf("ERROR: instruction encoding "
				       "failed!\n");
				goto error;
			}
		} /*else {
			printf("ERROR: %d: Unexpected token.\n", token->line);
			return true;
		}*/
	}
	return false;
error:
	return true;
}