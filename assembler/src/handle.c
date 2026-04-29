#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "structures.h"

static bool handle_errors(const struct TokenList *const tokenList,
	const int *const current_token, const int count)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];
	if (!(count % 2) || count > 5) {
		printf("ERROR: invalid error handling parameters count: %d\n",
			count);
		return true;
	}
	if (next1->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next1->num_value > 9) {
		printf("ERROR: %d: invalid register '%s'.\n", token->line,
			next1->str);
		return true;
	}
	if (count > 1) {
		if (next2->type != TOKEN_COMMA) {
			printf("ERROR: %d: expected comma after operand.\n",
				token->line);
			return true;
		}
		if (next3->type != TOKEN_REGISTER) {
			printf("ERROR: %d: expected register after comma.\n",
				token->line);
			return true;
		}
		if (next3->num_value > 9) {
			printf("ERROR: %d: invalid register '%s' in "
			       "instruction %s.\n",
				token->line, next3->str, token->str);
			return true;
		}
	}
	if (count > 4) {
		if (next4->type != TOKEN_COMMA) {
			printf("ERROR: %d: expected comma after operand.\n",
				token->line);
			return true;
		}
		if (next5->type != TOKEN_REGISTER) {
			printf("ERROR: %d: expected register after comma.\n",
				token->line);
			return true;
		}
		if (next5->num_value > 9) {
			printf("ERROR: %d: invalid register '%s' in "
			       "instruction %s.\n",
				token->line, next5->str, token->str);
			return true;
		}
	}
	return false;
}
static int handle_symbol(
	const struct SymbolTable *symbolTable, const char *symbol)
{
	int symbol_num = -1;
	for (int i = 0; i < symbolTable->count; i++) {
		if (!strcmp(symbolTable->symbols[i].name, symbol)) {
			symbol_num = i;
			break;
		}
	}
	return symbol_num;
}

static bool handle_immediate(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint8_t *immediate)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	if (token->type == TOKEN_LT_SIGN || token->type == TOKEN_GT_SIGN) {
		if (next1->type != TOKEN_SYMBOL) {
			printf("ERROR: %d:expected label after <.\n",
				token->line);
			return true;
		}
		int symbol_num = handle_symbol(symbolTable, next1->str);
		if (symbol_num == -1) {
			printf("ERROR: %d: unknown label %s.\n", next1->line,
				next1->str);
			return true;
		}
		if (token->type == TOKEN_LT_SIGN) {
			*immediate = symbolTable->symbols[symbol_num].address &
				0xFF;
		} else {
			*immediate =
				(symbolTable->symbols[symbol_num].address >>
					8) &
				0xFF;
		}
		(*current_token)++;
	} else if (token->type == TOKEN_SYMBOL) {
		int symbol_num = handle_symbol(symbolTable, token->str);
		if (symbol_num == -1) {
			printf("ERROR: %d: unknown label %s.\n", token->line,
				token->str);
			return true;
		}
		*immediate = symbolTable->symbols[symbol_num].address & 0xFF;
	} else if (token->type == TOKEN_NUMBER) {
		if (token->num_value > 127 && token->str[1] != 'x') {
			printf("Warning: %d: value %d bigger than max "
			       "immediate, will wrap around.\n",
				token->line, token->num_value);
		}
		*immediate = token->num_value & 0xFF;
	} else if (token->type == TOKEN_MINUS) {
		if (next1->type != TOKEN_NUMBER) {
			printf("ERROR: %d: expected number after minus "
			       "sign.\n",
				next1->line);
			return true;
		}
		if (next1->num_value > 128 && next1->str[1] != 'x') {
			printf("Warning: %d: value -%d smaller than min "
			       "immediate, will wrap around.\n",
				next1->line, next1->num_value);
		}
		*immediate = (~next1->num_value + 1) & 0xFF;
		(*current_token)++;
	} else {
		printf("ERROR: %d: expected number or "
		       "high/low label after comma.\n",
			token->line);
		return true;
	}

	return false;
}

static bool handle_bracketparse(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint8_t *base_reg, uint8_t *immediate)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	if (token->type == TOKEN_REGISTER) {
		if (token->num_value < 10 || token->num_value > 19) {
			printf("ERROR: %d: invalid register %s, only a0-3 "
			       "allowed.\n",
				token->line, token->str);
			return true;
		}
		*base_reg = next1->num_value - 10;
		return false;
	}
	*current_token += 2;
	if (token->type != TOKEN_BRACKET_OPEN) {
		printf("ERROR: %d: expected '[' after comma.\n", token->line);
		return true;
	}
	if (next1->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after '['.\n",
			token->line);
		return true;
	}
	if (next1->num_value < 10 || next1->num_value > 19) {
		printf("ERROR: %d: invalid register %s, only a0-3 allowed.\n",
			token->line, next1->str);
		return true;
	}
	*base_reg = next1->num_value - 10;
	if (next2->type == TOKEN_BRACKET_CLOSE) {
		*immediate = 0;
		return false;
	}
	if (next2->type != TOKEN_PLUS && next2->type != TOKEN_MINUS) {
		printf("ERROR: %d: expected '+' or '-' after register.\n",
			token->line);
		return true;
	}
	(*current_token)++;
	if (handle_immediate(
		    tokenList, symbolTable, current_token, immediate)) {
		printf("ERROR: %d: failed to parse immediate.\n", token->line);
		return true;
	}
	if (next2->type == TOKEN_MINUS) {
		if (*immediate > 64) {
			printf("Warning: %d: offset less than -64, "
			       "truncated.\n",
				token->line);
		}
		*immediate = (~*immediate) + 1;
	} else if (*immediate > 63) {
		printf("Warning: %d: offset greater than 63, truncated.\n",
			token->line);
	}
	(*current_token)++;
	if (tokenList->tokens[*current_token].type != TOKEN_BRACKET_CLOSE) {
		printf("ERROR: %d: expected ']' after immediate/label.\n",
			token->line);
		return true;
	}

	return false;
}

bool handle_mov(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	if (next1->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after operand.\n",
			token->line);
		return true;
	}
	if (next3->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after comma.\n",
			token->line);
		return true;
	}
	if ((next1->num_value > 9 && next1->num_value < 20) ||
		(next3->num_value > 9 && next3->num_value < 20)) {
		printf("ERROR: %d: address registers not allowed in MOV.\n",
			token->line);
		return true;
	}
	if (next1->num_value > 19) {
		if (next3->num_value > 19) {
			printf("ERROR: %d: cannot move CSR to CSR.\n",
				token->line);
			return true;
		}
		*machine_code = 0x0180 | (next1->num_value - 20) << 13 |
			next3->num_value << 10;
	} else if (next3->num_value > 19) {
		*machine_code = 0x0100 | next1->num_value << 13 |
			(next3->num_value - 20) << 10;
	} else {
		*machine_code = 0x0080 | next1->num_value << 13 |
			next3->num_value << 10;
	}
	*current_token += 3;
	return false;
}

bool handle_spp(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	if (handle_errors(tokenList, current_token, 1)) { return true; }
	*machine_code = base | next1->num_value << 13;
	*current_token += 1;
	return false;
}

// Handles compare register register instructions: CMP, CMN, CMA
bool handle_crr(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	if (handle_errors(tokenList, current_token, 3)) { return true; }
	*machine_code = base | next1->num_value << 13 | next3->num_value << 10;
	*current_token += 3;
	return false;
}

bool handle_rrr(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];
	if (handle_errors(tokenList, current_token, 5)) { return true; }
	*machine_code = base | next1->num_value << 13 |
		next3->num_value << 10 | next5->num_value << 7;
	*current_token += 5;
	return false;
}

bool handle_add(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];
	struct Token *next6 = &tokenList->tokens[*current_token + 6];
	if (next5->type == TOKEN_REGISTER) {
		if (handle_errors(tokenList, current_token, 5)) {
			return true;
		}
		*machine_code = 0x0021 | next1->num_value << 13 |
			next3->num_value << 10 | next5->num_value << 7;
		*current_token += 5;
		return false;
	}
	*current_token += 5;
	if (next1->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA || next4->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after operand.\n",
			token->line);
		return true;
	}
	if (next3->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after comma.\n",
			token->line);
		return true;
	}
	if (next5->type != TOKEN_NUMBER && next6->type != TOKEN_NUMBER) {
		printf("ERROR: %d: expected number or register after comma.\n",
			token->line);
		return true;
	}
	if (next1->num_value > 9 || next3->num_value > 9) {
		printf("ERROR: %d: only r0-7 are valid in %s.\n", token->line,
			token->str);
		return true;
	}
	uint8_t immediate = 0;
	if (handle_immediate(
		    tokenList, symbolTable, current_token, &immediate)) {
		return true;
	}
	*machine_code = 0x0019 | next1->num_value << 13 |
		next3->num_value << 10 | (immediate & 0x1F) << 5;

	return false;
}

bool handle_shift(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];
	if (next5->type == TOKEN_REGISTER) {
		if (handle_errors(tokenList, current_token, 5)) {
			return true;
		}
		*machine_code = 0x0000 | base | next1->num_value << 13 |
			next3->num_value << 10 | next5->num_value << 7;
		*current_token += 5;
		return false;
	}
	if (next1->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA || next4->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after operand.\n",
			token->line);
		return true;
	}
	if (next3->type != TOKEN_REGISTER) {
		printf("ERROR: %d: expected register after comma.\n",
			token->line);
		return true;
	}
	if (next5->type != TOKEN_NUMBER) {
		printf("ERROR: %d: expected number or register after comma.\n",
			token->line);
		return true;
	}
	if (next1->num_value > 9 || next3->num_value > 9) {
		printf("ERROR: %d: only registers r0-7 are valid in %s.\n",
			token->line, token->str);
		return true;
	}
	if (next5->num_value > 7) {
		printf("Warning: %d: shift value greater than 7, will wrap "
		       "around.\n",
			next5->line);
	}
	*machine_code = 0x0040 | base | next1->num_value << 13 |
		next3->num_value << 10 | (next5->num_value & 0x7) << 7;

	*current_token += 5;
	return false;
}

bool handle_li(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	uint8_t immediate = 0;
	if (handle_errors(tokenList, current_token, 1)) { return true; }
	*current_token += 3;
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after register.\n",
			token->line);
		return true;
	}
	if (handle_immediate(
		    tokenList, symbolTable, current_token, &immediate)) {
		return true;
	}
	*machine_code = 0x0009 | next1->num_value << 13 | immediate << 5;
	return false;
}

// Handles SB and LB
bool handle_loadstore(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	uint8_t base_reg = 0;
	uint8_t immediate = 0;
	if (handle_errors(tokenList, current_token, 1)) { return true; }
	*current_token += 3;
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after register.\n",
			token->line);
		return true;
	}
	if (next3->type == TOKEN_REGISTER) {
		if (next3->num_value < 10 || next3->num_value > 19) {
			printf("ERROR: %d: only a0-3 are valid in %s.\n",
				token->line, token->str);
			return true;
		}
		base_reg = next3->num_value - 10;
	} else if (handle_bracketparse(tokenList, symbolTable, current_token,
			   &base_reg, &immediate)) {
		return true;
	}
	immediate &= 0x7F;
	*machine_code = base | next1->num_value << 13 | base_reg << 11 |
		immediate << 4;
	return false;
}

// Handles B, BL
// NOLINTBEGIN(readability-function-cognitive-complexity)
bool handle_branch_cond(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, uint16_t base, uint16_t current_address)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	uint8_t base_reg = 0;
	uint16_t offset = 0;
	*current_token += 3;
	if (next1->type != TOKEN_CONDITION) {
		printf("ERROR: %d: expected condition after instruction.\n",
			next1->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after condition.\n",
			next2->line);
		return true;
	}
	if (next3->type == TOKEN_SYMBOL) {
		int symbol_num = handle_symbol(symbolTable, next3->str);
		if (symbol_num == -1) {
			printf("ERROR: %d: unknown label %s.\n", next3->line,
				next3->str);
			return true;
		}
		offset = (symbolTable->symbols[symbol_num].address -
				 current_address - 2) >>
			1;
		base++;
	} else if (next3->type == TOKEN_NUMBER) {
		if (next3->num_value > 511 && next3->str[1] != 'x') {
			printf("Warning: %d: value %d bigger than max "
			       "offset, will wrap around.\n",
				next3->line, next3->num_value);
		}
		if (next3->num_value % 2) {
			printf("Warning: %d: odd value %d will be rounded "
			       "down.\n",
				next3->line, next3->num_value);
		}
		offset = next3->num_value >> 1;
		base++;
	} else if (next3->type == TOKEN_MINUS) {
		if (next4->type != TOKEN_NUMBER) {
			printf("ERROR: %d: expected number after minus "
			       "sign.\n",
				next4->line);
			return true;
		}
		if (next4->num_value > 512 && next4->str[1] != 'x') {
			printf("Warning: %d: value -%d smaller than min "
			       "offset, will wrap around.\n",
				next4->line, next4->num_value);
		}
		if (next4->num_value % 2) {
			printf("Warning: %d: odd value -%d will be rounded "
			       "up.\n",
				next4->line, next4->num_value);
		}
		offset = (~next4->num_value + 1) >> 1;
		(*current_token)++;
		base++;
	} else if (next3->type == TOKEN_BRACKET_OPEN) {
		if (handle_bracketparse(tokenList, symbolTable, current_token,
			    &base_reg, &offset)) {
			return true;
		}
		if (next3->num_value > 511 && next3->str[1] != 'x') {
			printf("Warning: %d: value %d bigger than max "
			       "offset, will wrap around.\n",
				next3->line, next3->num_value);
		}
		if (next3->num_value % 2) {
			printf("Warning: %d: odd value %d will be rounded "
			       "down.\n",
				next3->line, next3->num_value);
		}
		offset = next3->num_value >> 1;
	} else if (next3->type == TOKEN_REGISTER) {
		if (next3->num_value < 10 || next3->num_value > 19) {
			printf("ERROR: %d: only a0-3 are valid in %s.\n",
				token->line, token->str);
			return true;
		}
		base_reg = next3->num_value;
		offset = 0;
	} else {
		printf("ERROR: %d: expected [r?+/-X] after comma.\n",
			token->line);
		return true;
	}
	if (base % 2) {
		offset &= 0x1FF;
		*machine_code = base | next1->num_value << 13 | offset << 4;
	} else {
		offset &= 0x7F;
		*machine_code = base | next1->num_value << 13 |
			base_reg << 11 | offset << 4;
	}
	return false;
}
// NOLINTEND(readability-function-cognitive-complexity)
