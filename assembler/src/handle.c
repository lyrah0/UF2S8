#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "handle.h"
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
	if (next1->num_value < 0 || next1->num_value > 15) {
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
		if (next3->num_value < 0 || next3->num_value > 15) {
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
		if (next5->num_value < 0 || next5->num_value > 15) {
			printf("ERROR: %d: invalid register '%s' in "
			       "instruction %s.\n",
				token->line, next5->str, token->str);
			return true;
		}
	}
	return false;
}

static inline uint8_t pack_imm8_mid(uint8_t imm)
{ return (uint8_t)((imm & 0xF0) | ((imm & 0x07) << 1) | ((imm >> 3) & 0x01)); }

static inline uint16_t pack_imm5_mem(uint8_t imm)
{ return (uint16_t)(((imm >> 1) & 0x0F) << 12 | (imm & 0x01) << 8); }

static inline uint16_t pack_imm12_branch(uint16_t imm)
{
	return (uint16_t)(((imm >> 4) & 0xFF) << 8 | (imm & 0x07) << 5 |
		((imm >> 3) & 0x01) << 4);
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
		if ((token->num_value > 255 || token->num_value < -128) &&
			token->str[1] != 'x') {
			printf("Warning: %d: value %lld outside valid 8-bit "
			       "immediate range, will wrap around.\n",
				token->line, token->num_value);
		}
		*immediate = token->num_value & 0xFF;
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
		if (token->num_value < 20 || token->num_value > 27) {
			printf("ERROR: %d: invalid register %s, only a0-7 "
			       "allowed.\n",
				token->line, token->str);
			return true;
		}
		*base_reg = (uint8_t)(token->num_value - 20);
		*immediate = 0;
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
	if (next1->num_value < 20 || next1->num_value > 27) {
		printf("ERROR: %d: invalid register %s, only a0-7 allowed.\n",
			token->line, next1->str);
		return true;
	}
	*base_reg = (uint8_t)(next1->num_value - 20);
	if (next2->type == TOKEN_BRACKET_CLOSE) {
		*immediate = 0;
		return false;
	}
	if (next2->type == TOKEN_PLUS) { (*current_token)++; }
	if (handle_immediate(
		    tokenList, symbolTable, current_token, immediate)) {
		printf("ERROR: %d: failed to parse immediate.\n", token->line);
		return true;
	}
	int8_t signed_imm = (int8_t)*immediate;
	if (signed_imm > 15) {
		printf("Warning: %d: offset greater than 15, truncated.\n",
			token->line);
	} else if (signed_imm < -16) {
		printf("Warning: %d: offset less than -16, truncated.\n",
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
	if ((next1->num_value >= 20 && next1->num_value <= 27) ||
		(next3->num_value >= 20 && next3->num_value <= 27)) {
		printf("ERROR: %d: address registers not allowed in MOV.\n",
			token->line);
		return true;
	}
	if (next1->num_value >= 40) {
		if (next3->num_value >= 40) {
			printf("ERROR: %d: cannot move CSR to CSR.\n",
				token->line);
			return true;
		}
		uint8_t csr = (uint8_t)(next1->num_value - 40);
		uint8_t s = (uint8_t)next3->num_value;
		*machine_code = 0xB000 | (s << 8) | (csr << 4);
	} else if (next3->num_value >= 40) {
		uint8_t d = (uint8_t)next1->num_value;
		uint8_t csr = (uint8_t)(next3->num_value - 40);
		*machine_code = 0xA000 | (csr << 8) | (d << 4);
	} else {
		uint8_t d = (uint8_t)next1->num_value;
		uint8_t s = (uint8_t)next3->num_value;
		*machine_code = 0x3000 | (s << 8) | (d << 4);
	}
	*current_token += 3;
	return false;
}

bool handle_ss(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	if (handle_errors(tokenList, current_token, 1)) { return true; }
	*machine_code = base | (next1->num_value << 4);
	*current_token += 1;
	return false;
}

bool handle_sd(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	if (handle_errors(tokenList, current_token, 1)) { return true; }
	*machine_code = base | (next1->num_value << 4);
	*current_token += 1;
	return false;
}

bool handle_unary(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];

	if (next1->type != TOKEN_REGISTER || next1->num_value < 0 ||
		next1->num_value > 15) {
		printf("ERROR: %d: expected register after instruction.\n",
			tokenList->tokens[*current_token].line);
		return true;
	}
	uint8_t d = (uint8_t)next1->num_value;
	uint8_t s = d;
	if (next2->type == TOKEN_COMMA) {
		if (next3->type != TOKEN_REGISTER || next3->num_value < 0 ||
			next3->num_value > 15) {
			printf("ERROR: %d: expected register after comma.\n",
				tokenList->tokens[*current_token].line);
			return true;
		}
		s = (uint8_t)next3->num_value;
		*current_token += 3;
	} else {
		*current_token += 1;
	}
	*machine_code = base | (s << 8) | (d << 4);
	return false;
}

bool handle_ds(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	if (handle_errors(tokenList, current_token, 3)) { return true; }
	*machine_code = base | (next1->num_value << 8) |
		(next3->num_value << 4);
	*current_token += 3;
	return false;
}

bool handle_rrr(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base)
{
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];

	if (next4->type == TOKEN_COMMA && next5->type == TOKEN_REGISTER) {
		if (handle_errors(tokenList, current_token, 5)) {
			return true;
		}
		uint8_t d = (uint8_t)next1->num_value;
		uint8_t m = (uint8_t)next5->num_value;
		*machine_code = base | (m << 8) | (d << 4);
		*current_token += 5;
		return false;
	}

	if (handle_errors(tokenList, current_token, 3)) { return true; }
	uint8_t d = (uint8_t)next1->num_value;
	uint8_t m = (uint8_t)next3->num_value;
	*machine_code = base | (m << 8) | (d << 4);
	*current_token += 3;
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

	if (next1->type != TOKEN_REGISTER || next1->num_value < 0 ||
		next1->num_value > 15) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after operand.\n",
			token->line);
		return true;
	}

	uint8_t d = (uint8_t)next1->num_value;

	if (next3->type == TOKEN_REGISTER && next4->type == TOKEN_COMMA) {
		if (next5->type == TOKEN_REGISTER) {
			if (next5->num_value < 0 || next5->num_value > 15) {
				printf("ERROR: %d: invalid register '%s'.\n",
					token->line, next5->str);
				return true;
			}
			uint8_t m = (uint8_t)next5->num_value;
			*machine_code = 0x2001 | (m << 8) | (d << 4);
			*current_token += 5;
			return false;
		}
		*current_token += 5;
		uint8_t immediate = 0;
		if (handle_immediate(tokenList, symbolTable, current_token,
			    &immediate)) {
			return true;
		}
		*machine_code = 0x0008 | (d << 4) |
			((uint16_t)pack_imm8_mid(immediate) << 8);
		return false;
	}

	if (next3->type == TOKEN_REGISTER) {
		if (next3->num_value < 0 || next3->num_value > 15) {
			printf("ERROR: %d: invalid register '%s'.\n",
				token->line, next3->str);
			return true;
		}
		uint8_t m = (uint8_t)next3->num_value;
		*machine_code = 0x2001 | (m << 8) | (d << 4);
		*current_token += 3;
		return false;
	}

	*current_token += 3;
	uint8_t immediate = 0;
	if (handle_immediate(
		    tokenList, symbolTable, current_token, &immediate)) {
		return true;
	}
	*machine_code = 0x0008 | (d << 4) |
		((uint16_t)pack_imm8_mid(immediate) << 8);
	return false;
}

bool handle_shift(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t reg_base, uint16_t imm_base)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];
	struct Token *next3 = &tokenList->tokens[*current_token + 3];
	struct Token *next4 = &tokenList->tokens[*current_token + 4];
	struct Token *next5 = &tokenList->tokens[*current_token + 5];

	if (next1->type != TOKEN_REGISTER || next1->num_value < 0 ||
		next1->num_value > 15) {
		printf("ERROR: %d: expected register after instruction.\n",
			token->line);
		return true;
	}
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after operand.\n",
			token->line);
		return true;
	}

	uint8_t d = (uint8_t)next1->num_value;

	if (next3->type == TOKEN_REGISTER && next4->type == TOKEN_COMMA) {
		if (next5->type == TOKEN_REGISTER) {
			if (next5->num_value < 0 || next5->num_value > 15) {
				printf("ERROR: %d: invalid register '%s'.\n",
					token->line, next5->str);
				return true;
			}
			uint8_t m = (uint8_t)next5->num_value;
			*machine_code = reg_base | (m << 8) | (d << 4);
			*current_token += 5;
			return false;
		}
		if (next5->type == TOKEN_NUMBER) {
			if (next5->num_value > 7) {
				printf("Warning: %d: shift value greater than "
				       "7, will truncate to 3 bits.\n",
					token->line);
			}
			uint8_t imm = (uint8_t)(next5->num_value & 0x07);
			*machine_code = imm_base | (imm << 8) | (d << 4);
			*current_token += 5;
			return false;
		}
		printf("ERROR: %d: expected register or shift amount after "
		       "comma.\n",
			token->line);
		return true;
	}

	if (next3->type == TOKEN_REGISTER) {
		if (next3->num_value < 0 || next3->num_value > 15) {
			printf("ERROR: %d: invalid register '%s'.\n",
				token->line, next3->str);
			return true;
		}
		uint8_t m = (uint8_t)next3->num_value;
		*machine_code = reg_base | (m << 8) | (d << 4);
		*current_token += 3;
		return false;
	}
	if (next3->type == TOKEN_NUMBER) {
		if (next3->num_value > 7) {
			printf("Warning: %d: shift value greater than 7, will "
			       "truncate to 3 bits.\n",
				token->line);
		}
		uint8_t imm = (uint8_t)(next3->num_value & 0x07);
		*machine_code = imm_base | (imm << 8) | (d << 4);
		*current_token += 3;
		return false;
	}

	printf("ERROR: %d: expected register or shift amount after comma.\n",
		token->line);
	return true;
}

bool handle_bit_ops(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, enum BitOpType op_type)
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

	if (next1->num_value == 40) {
		if (next3->type != TOKEN_NUMBER) {
			printf("ERROR: %d: expected bit number after comma.\n",
				token->line);
			return true;
		}
		uint8_t imm = (uint8_t)(next3->num_value & 0x7);
		if (op_type == BIT_OP_BST) {
			*machine_code = 0x1E00 | (imm << 5);
		} else if (op_type == BIT_OP_BIC) {
			*machine_code = 0x1E10 | (imm << 5);
		} else {
			*machine_code = 0x1F00 | (imm << 5);
		}
		*current_token += 3;
		return false;
	}

	if (next1->num_value < 0 || next1->num_value > 15) {
		printf("ERROR: %d: invalid register '%s'.\n", token->line,
			next1->str);
		return true;
	}
	uint8_t reg = (uint8_t)next1->num_value;

	if (next3->type == TOKEN_NUMBER) {
		uint8_t imm = (uint8_t)(next3->num_value & 0x7);
		if (op_type == BIT_OP_BST) {
			*machine_code = 0x4000 | (imm << 9) | (reg << 4);
		} else if (op_type == BIT_OP_BIC) {
			*machine_code = 0x4100 | (imm << 9) | (reg << 4);
		} else {
			*machine_code = 0xF011 | (imm << 9) | (reg << 4);
		}
		*current_token += 3;
		return false;
	}

	if (next3->type == TOKEN_REGISTER && next3->num_value >= 0 &&
		next3->num_value <= 15) {
		uint8_t m = (uint8_t)next3->num_value;
		if (op_type == BIT_OP_BST) {
			*machine_code = 0x0002 | (m << 8) | (reg << 4);
		} else if (op_type == BIT_OP_BIC) {
			*machine_code = 0x1002 | (m << 8) | (reg << 4);
		} else {
			*machine_code = 0x2002 | (m << 8) | (reg << 4);
		}
		*current_token += 3;
		return false;
	}

	printf("ERROR: %d: expected bit number or register after comma.\n",
		token->line);
	return true;
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
	if (next2->type != TOKEN_COMMA) {
		printf("ERROR: %d: expected comma after register.\n",
			token->line);
		return true;
	}
	*current_token += 3;
	if (handle_immediate(
		    tokenList, symbolTable, current_token, &immediate)) {
		return true;
	}
	*machine_code = 0x0009 | (next1->num_value << 4) |
		((uint16_t)pack_imm8_mid(immediate) << 8);
	return false;
}

bool handle_loadstore(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool load)
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
		if (next3->num_value < 20 || next3->num_value > 27) {
			printf("ERROR: %d: only a0-7 are valid in %s.\n",
				token->line, token->str);
			return true;
		}
		base_reg = (uint8_t)(next3->num_value - 20);
		immediate = 0;
	} else if (handle_bracketparse(tokenList, symbolTable, current_token,
			   &base_reg, &immediate)) {
		return true;
	}
	immediate &= 0x1F;
	uint16_t packed_imm = pack_imm5_mem(immediate);
	if (load) {
		*machine_code = 0x000B | (next1->num_value << 4) |
			(base_reg << 9) | packed_imm;
	} else {
		*machine_code = 0x000A | (next1->num_value << 4) |
			(base_reg << 9) | packed_imm;
	}
	return false;
}

static bool handle_branch_target_parse(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t current_address, uint8_t *base_reg, uint16_t *offset,
	bool *is_relative)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];
	struct Token *next2 = &tokenList->tokens[*current_token + 2];

	if (token->type == TOKEN_SYMBOL) {
		int symbol_num = handle_symbol(symbolTable, token->str);
		if (symbol_num == -1) {
			printf("ERROR: %d: unknown label %s.\n", token->line,
				token->str);
			return true;
		}
		int32_t diff =
			(int32_t)symbolTable->symbols[symbol_num].address -
			(int32_t)(current_address + 2);
		*offset = (uint16_t)(diff >> 1);
		*is_relative = true;
		return false;
	}
	if (token->type == TOKEN_NUMBER) {
		if (token->num_value % 2) {
			printf("Warning: %d: odd value %lld will be "
			       "rounded.\n",
				token->line, token->num_value);
		}
		*offset = (uint16_t)(token->num_value >> 1);
		*is_relative = true;
		return false;
	}
	if (token->type == TOKEN_BRACKET_OPEN) {
		if (next1->type != TOKEN_REGISTER || next1->num_value < 20 ||
			next1->num_value > 27) {
			printf("ERROR: %d: expected address register a0-a7 in "
			       "[ab].\n",
				token->line);
			return true;
		}
		if (next2->type != TOKEN_BRACKET_CLOSE) {
			printf("ERROR: %d: expected ']' after register.\n",
				token->line);
			return true;
		}
		*base_reg = (uint8_t)(next1->num_value - 20);
		*current_token += 2;
		*is_relative = false;
		return false;
	}
	if (token->type == TOKEN_REGISTER && token->num_value >= 20 &&
		token->num_value <= 27) {
		*base_reg = (uint8_t)(token->num_value - 20);
		*is_relative = false;
		return false;
	}

	printf("ERROR: %d: invalid branch target %s.\n", token->line,
		token->str);
	return true;
}

bool handle_branch(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool link, uint16_t current_address)
{
	struct Token *token = &tokenList->tokens[*current_token];
	struct Token *next1 = &tokenList->tokens[*current_token + 1];

	if (next1->type == TOKEN_CONDITION) {
		struct Token *next2 = &tokenList->tokens[*current_token + 2];
		if (next2->type != TOKEN_COMMA) {
			printf("ERROR: %d: expected comma after condition in "
			       "%s.\n",
				token->line, token->str);
			return true;
		}
		*current_token += 3;
		uint8_t base_reg = 0;
		uint16_t offset = 0;
		bool is_relative = false;
		if (handle_branch_target_parse(tokenList, symbolTable,
			    current_token, current_address, &base_reg, &offset,
			    &is_relative)) {
			return true;
		}
		uint8_t cond = (uint8_t)next1->num_value;
		if (is_relative) {
			uint8_t imm8 = (uint8_t)(offset & 0xFF);
			uint16_t base = (int)link ? 0x000D : 0x000C;
			*machine_code = base | (cond << 4) |
				((uint16_t)pack_imm8_mid(imm8) << 8);
		} else {
			uint16_t base = (int)link ? 0x2100 : 0x2000;
			*machine_code = base | (base_reg << 9) | (cond << 4);
		}
		return false;
	}

	*current_token += 1;
	uint8_t base_reg = 0;
	uint16_t offset = 0;
	bool is_relative = false;
	if (handle_branch_target_parse(tokenList, symbolTable, current_token,
		    current_address, &base_reg, &offset, &is_relative)) {
		return true;
	}
	if (is_relative) {
		uint16_t imm12 = offset & 0x0FFF;
		uint16_t base = (int)link ? 0x000F : 0x000E;
		*machine_code = base | pack_imm12_branch(imm12);
	} else {
		uint16_t base = (int)link ? 0x1D10 : 0x1D00;
		*machine_code = base | (base_reg << 5);
	}
	return false;
}
