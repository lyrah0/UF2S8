#include "structures.h"
#include "handle.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

struct EncodeState {
	uint8_t *buffer;
	int bank_w0;
	int bank_w1;
};

static void write_to_buffer(
	struct EncodeState *state, int addr, const void *data, int size)
{
	int offset = 0;
	if (addr < 0x8000) {
		offset = (state->bank_w0 * 0x8000) + addr;
	} else if (addr < 0xC000) {
		offset = EXT_MEMORY_W0_SIZE + (state->bank_w1 * 0x4000) +
			(addr - 0x8000);
	} else {
		offset = EXT_MEMORY_W0_SIZE + EXT_MEMORY_W1_SIZE +
			(addr - 0xC000);
	}
	if (offset + size > TOTAL_BINARY_SIZE) { return; }
	memcpy(state->buffer + offset, data, size);
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
static bool encode_instructions(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, struct EncodeState *state,
	int *current_address, int *current_token)
{
	struct Token *token = &tokenList->tokens[*current_token];
	uint16_t machine_code = 0;
	if (!strcasecmp(token->str, "NOP")) {
		machine_code = 0x0000;
	} else if (!strcasecmp(token->str, "RET")) {
		machine_code = 0x2000;
	} else if (!strcasecmp(token->str, "WFI")) {
		machine_code = 0x4000;
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
		if (handle_pp(
			    tokenList, current_token, &machine_code, false)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "PUSH")) {
		if (handle_pp(tokenList, current_token, &machine_code, true)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "MOV")) {
		if (handle_mov(tokenList, current_token, &machine_code)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "CMP")) {
		if (handle_crr(
			    tokenList, current_token, &machine_code, 0x0110)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "CMA")) {
		if (handle_crr(
			    tokenList, current_token, &machine_code, 0x0190)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SUB")) {
		if (handle_rrr(
			    tokenList, current_token, &machine_code, 0x0001)) {
			goto error;
		}
	} else if (!strcasecmp(token->str, "SBB")) {
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
		write_to_buffer(state, *current_address, &empty, 1);
		(*current_address)++;
	}
	write_to_buffer(state, *current_address, &machine_code, 2);
	*current_address += 2;
	return false;
error:
	printf("ERROR: %d: failed to encode %s\n", token->line, token->str);
	return true;
}
// NOLINTEND(readability-function-cognitive-complexity)

static bool encode_directive_asciz(struct TokenList *tokenList,
	struct EncodeState *state, int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			for (int i = 0; i < token->len + 1; i++) {
				write_to_buffer(state, *current_address,
					&token->str[i], 1);
				(*current_address)++;
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

static bool encode_directive_ascii(struct TokenList *tokenList,
	struct EncodeState *state, int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			for (int i = 0; i < token->len; i++) {
				write_to_buffer(state, *current_address,
					&token->str[i], 1);
				(*current_address)++;
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

static bool encode_directive_symbol(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, struct EncodeState *state,
	int *current_address, int *current_token, int size)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next = &tokenList->tokens[*current_token + 1];

	if (token->type == TOKEN_GT_SIGN || token->type == TOKEN_LT_SIGN) {
		(*current_token)++;
	} else if (token->type != TOKEN_SYMBOL) {
		return false;
	}

	struct Token *sym = (token->type == TOKEN_SYMBOL) ? token : next;

	long long sym_value = 0;
	bool found = false;
	for (int j = 0; j < symbolTable->count; j++) {
		if (strcmp(symbolTable->symbols[j].name, sym->str) == 0) {
			sym_value = symbolTable->symbols[j].address;
			found = true;
			break;
		}
	}
	if (!found) {
		printf("ERROR: %d: undefined symbol %s\n", sym->line,
			sym->str);
		return true;
	}

	if (token->type == TOKEN_GT_SIGN) {
		uint8_t temp = (sym_value >> 8) & 0xFF;
		write_to_buffer(state, *current_address, &temp, 1);
		(*current_address)++;
	} else if (token->type == TOKEN_LT_SIGN) {
		uint8_t temp = sym_value & 0xFF;
		write_to_buffer(state, *current_address, &temp, 1);
		(*current_address)++;
	} else {
		write_to_buffer(state, *current_address, &sym_value, size);
		(*current_address) += size;
	}
	return false;
}

static bool encode_directive_dsize(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, struct EncodeState *state,
	int *current_address, int *current_token, int size)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_SYMBOL &&
			*current_token + 1 < tokenList->count &&
			tokenList->tokens[*current_token + 1].type ==
				TOKEN_COLON) {
			(*current_token)--;
			break;
		}
		if (token->type == TOKEN_NUMBER) {
			if (token->num_value > (1LL << (8 * size)) - 1) {
				printf("Warning: %d: value %lld truncated",
					token->line, token->num_value);
			}
			write_to_buffer(state, *current_address,
				&token->num_value, size);
			(*current_address) += size;
		} else if (token->type == TOKEN_STRING) {
			for (int i = 0; i < token->len; i++) {
				write_to_buffer(state, *current_address,
					&token->str[i], 1);
				(*current_address)++;
			}
		} else if (token->type == TOKEN_SYMBOL ||
			token->type == TOKEN_GT_SIGN ||
			token->type == TOKEN_LT_SIGN) {
			if (encode_directive_symbol(tokenList, symbolTable,
				    state, current_address, current_token,
				    size)) {
				return true;
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

static bool encode_directive_align(struct TokenList *tokenList,
	struct EncodeState *state, int *current_address, int *current_token)
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
		write_to_buffer(state, *current_address, &empty, 1);
		(*current_address)++;
	}
	return false;
}

static bool encode_directive_origin(
	struct TokenList *tokenList, int *current_address, int *current_token)
{
	if (*current_token + 2 >= tokenList->count) { return false; }
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	if (next2->type != TOKEN_NUMBER) { return false; }

	*current_address = (int)next2->num_value;
	*current_token += 2;

	return false;
}

static bool encode_directive_space(struct TokenList *tokenList,
	struct EncodeState *state, int *current_address, int *current_token)
{
	*current_token += 2;
	struct Token *token = &tokenList->tokens[*current_token];

	if (*current_token >= tokenList->count) { return false; }
	if (token->type != TOKEN_NUMBER) {
		printf("ERROR: %d: space expects a positive number\n",
			token->line);
		return true;
	}
	const int empty = 0;
	for (int i = 0; i < token->num_value; i++) {
		write_to_buffer(state, *current_address, &empty, 1);
		(*current_address)++;
	}
	return false;
}

static bool encode_directive_bank(struct TokenList *tokenList,
	struct EncodeState *state, int *current_token,
	const unsigned char window)
{
	*current_token += 2;
	struct Token *token = &tokenList->tokens[*current_token];
	if (token->type != TOKEN_NUMBER) {
		printf("ERROR: %d: bank expects a number\n", token->line);
		return true;
	}
	switch (window) {
	case 0:
		state->bank_w0 = (int)token->num_value & 0x0F;
		break;
	case 1:
		state->bank_w1 = (int)token->num_value & 0x0F;
		break;
	default:
		printf("ERROR: %d: invalid window\n", token->line);
		return true;
	}
	return false;
}

static bool encode_directive_equ(
	struct TokenList *tokenList, int *current_token)
{
	if (*current_token + 4 >= tokenList->count) { return true; }
	*current_token += 4;
	return false;
}

static bool encode_directives(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, struct EncodeState *state,
	int *current_address, int *current_token)
{
	struct Token *next = &tokenList->tokens[*current_token + 1];
	if (next->type != TOKEN_SYMBOL) {
		printf("ERROR: %d: directive expects a symbol\n", next->line);
		return true;
	}
	if (strcasecmp(next->str, "origin") == 0) {
		return encode_directive_origin(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "db") == 0 ||
		strcasecmp(next->str, "byte") == 0) {
		return encode_directive_dsize(tokenList, symbolTable, state,
			current_address, current_token, 1);
	}
	if (strcasecmp(next->str, "dh") == 0 ||
		strcasecmp(next->str, "half") == 0) {
		return encode_directive_dsize(tokenList, symbolTable, state,
			current_address, current_token, 2);
	}
	if (strcasecmp(next->str, "dw") == 0 ||
		strcasecmp(next->str, "word") == 0) {
		return encode_directive_dsize(tokenList, symbolTable, state,
			current_address, current_token, 4);
	}
	if (strcasecmp(next->str, "ascii") == 0) {
		return encode_directive_ascii(
			tokenList, state, current_address, current_token);
	}
	if (strcasecmp(next->str, "asciz") == 0) {
		return encode_directive_asciz(
			tokenList, state, current_address, current_token);
	}
	if (strcasecmp(next->str, "align") == 0) {
		return encode_directive_align(
			tokenList, state, current_address, current_token);
	}
	if (strcasecmp(next->str, "space") == 0 ||
		strcasecmp(next->str, "fill") == 0 ||
		strcasecmp(next->str, "resb") == 0) {
		return encode_directive_space(
			tokenList, state, current_address, current_token);
	}
	if (strcasecmp(next->str, "equ") == 0 ||
		strcasecmp(next->str, "define") == 0) {
		return encode_directive_equ(tokenList, current_token);
	}
	if (strcasecmp(next->str, "bankw0") == 0) {
		return encode_directive_bank(
			tokenList, state, current_token, 0);
	}
	if (strcasecmp(next->str, "bankw1") == 0) {
		return encode_directive_bank(
			tokenList, state, current_token, 1);
	}

	return false;
}

bool encode_and_write(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, FILE *foutput)
{
	int current_address = 0;
	struct EncodeState state;
	state.buffer = calloc(1, TOTAL_BINARY_SIZE);
	if (!state.buffer) {
		printf("ERROR: out of memory\n");
		return true;
	}
	state.bank_w0 = 0;
	state.bank_w1 = 0;

	for (int current_token = 0; current_token < tokenList->count;
		current_token++) {
		struct Token *token = &tokenList->tokens[current_token];
		if (token->type == TOKEN_PERIOD &&
			tokenList->count > current_token + 1) {
			if (encode_directives(tokenList, symbolTable, &state,
				    &current_address, &current_token)) {
				printf("ERROR: directive encoding failed!\n");
				free(state.buffer);
				goto error;
			}
		} else if (token->type == TOKEN_INSTRUCTION) {
			if (encode_instructions(tokenList, symbolTable, &state,
				    &current_address, &current_token)) {
				printf("ERROR: instruction encoding "
				       "failed!\n");
				free(state.buffer);
				goto error;
			}
		}
	}
	if (!fwrite(state.buffer, TOTAL_BINARY_SIZE, 1, foutput)) {
		printf("ERROR: failed to write output file\n");
	}
	free(state.buffer);
	return false;
error:
	return true;
}