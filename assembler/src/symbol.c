#include "structures.h"
#include "symbol.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

static bool symbol_construct(struct SymbolTable *symbolTable,
	int current_address, struct Token *prev, bool allow_redef)
{
	for (int i = 0; i < symbolTable->count; i++) {
		if (!strcmp(symbolTable->symbols[i].name, prev->str)) {
			if (allow_redef) {
				printf("INFO: redefinition of label %s, value "
				       "0x%X, was 0x%X\n",
					prev->str, current_address,
					symbolTable->symbols[i].address);
				symbolTable->symbols[i].address =
					current_address;
				return false;
			}
			printf("ERROR: %d: label %s already exists, "
			       "redefinition not allowed.\n",
				prev->line, prev->str);
			return true;
		}
	}

	symbolTable->symbols[symbolTable->count].address = current_address;
	(void)strcpy(symbolTable->symbols[symbolTable->count].name, prev->str);
	symbolTable->count++;

	return false;
}

static bool symbol_directive_asciz(
	struct TokenList *tokenList, int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			(*current_address) += token->len + 1;
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool symbol_directive_ascii(
	struct TokenList *tokenList, int *current_address, int *current_token)
{
	*current_token += 2;
	while (*current_token < tokenList->count) {
		struct Token *token = &tokenList->tokens[*current_token];
		if (token->type == TOKEN_STRING) {
			(*current_address) += token->len;
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool symbol_directive_dsize(struct TokenList *tokenList,
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
		if (token->type == TOKEN_NUMBER ||
			token->type == TOKEN_SYMBOL) {
			(*current_address) += size;
		} else if (token->type == TOKEN_STRING) {
			(*current_address) += token->len;
		} else if (token->type == TOKEN_GT_SIGN ||
			token->type == TOKEN_LT_SIGN) {
			(*current_address) += 1;
			(*current_token)++;
		} else if (token->type == TOKEN_COMMA) {
		} else {
			(*current_token)--;
			break;
		}
		(*current_token)++;
	}
	return false;
}

static bool symbol_directive_align(
	struct TokenList *tokenList, int *current_address, int *current_token)
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
	*current_address += (align - (*current_address % align)) % align;

	return false;
}

static bool symbol_directive_origin(
	struct TokenList *tokenList, int *current_address, int *current_token)
{
	if (*current_token + 2 >= tokenList->count) { return false; }

	struct Token *next2 = &tokenList->tokens[*current_token + 2];

	if (next2->type != TOKEN_NUMBER) { return false; }
	if (next2->num_value % 2) {
		printf("ERROR: origin not aligned in line %d\n", next2->line);
		return true;
	}
	*current_address = (int)next2->num_value;
	*current_token += 2;

	return false;
}

static bool symbol_directive_space(
	struct TokenList *tokenList, int *current_address, int *current_token)
{
	*current_token += 2;
	struct Token *token = &tokenList->tokens[*current_token];

	if (*current_token >= tokenList->count) { return false; }
	if (token->type != TOKEN_NUMBER) {
		printf("ERROR: %d: space expects a positive number\n",
			token->line);
		return true;
	}
	*current_address += (int)token->num_value;
	return false;
}

static bool symbol_directive_equ(struct TokenList *tokenList,
	int *current_token, struct SymbolTable *symbolTable)
{
	if (*current_token + 4 >= tokenList->count) { return false; }

	struct Token *sym_token = &tokenList->tokens[*current_token + 2];
	struct Token *comma_token = &tokenList->tokens[*current_token + 3];
	struct Token *num_token = &tokenList->tokens[*current_token + 4];

	if (sym_token->type != TOKEN_SYMBOL ||
		comma_token->type != TOKEN_COMMA ||
		num_token->type != TOKEN_NUMBER) {
		printf("ERROR: %d: equ expects a symbol, a comma, and a "
		       "number\n",
			sym_token->line);
		return true;
	}

	if (symbol_construct(
		    symbolTable, (int)num_token->num_value, sym_token, true)) {
		return true;
	}

	*current_token += 4;
	return false;
}

static bool symbol_directives(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, int *current_address,
	int *current_token)
{
	struct Token *next = &tokenList->tokens[*current_token + 1];
	if (next->type != TOKEN_SYMBOL) {
		printf("ERROR: %d: directive expects a symbol\n", next->line);
		return true;
	}
	if (strcasecmp(next->str, "origin") == 0) {
		return symbol_directive_origin(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "db") == 0 ||
		strcasecmp(next->str, "byte") == 0) {
		return symbol_directive_dsize(
			tokenList, current_address, current_token, 1);
	}
	if (strcasecmp(next->str, "dh") == 0 ||
		strcasecmp(next->str, "half") == 0) {
		return symbol_directive_dsize(
			tokenList, current_address, current_token, 2);
	}
	if (strcasecmp(next->str, "dw") == 0 ||
		strcasecmp(next->str, "word") == 0) {
		return symbol_directive_dsize(
			tokenList, current_address, current_token, 4);
	}
	if (strcasecmp(next->str, "ascii") == 0) {
		return symbol_directive_ascii(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "asciz") == 0) {
		return symbol_directive_asciz(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "align") == 0) {
		return symbol_directive_align(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "space") == 0 ||
		strcasecmp(next->str, "fill") == 0 ||
		strcasecmp(next->str, "resb") == 0) {
		return symbol_directive_space(
			tokenList, current_address, current_token);
	}
	if (strcasecmp(next->str, "equ") == 0 ||
		strcasecmp(next->str, "define") == 0) {
		return symbol_directive_equ(
			tokenList, current_token, symbolTable);
	}
	if (strcasecmp(next->str, "bankw0") == 0 ||
		strcasecmp(next->str, "bankw1") == 0) {
		*current_token += 2;
		return false;
	}
	printf("ERROR: unknown directive %s in line %d\n", next->str,
		next->line);
	return true;

	return false;
}

bool symbol_build_table(
	struct TokenList *tokenList, struct SymbolTable *symbolTable)
{
	int current_address = 0;

	for (int current_token = 0; current_token < tokenList->count;
		current_token++) {
		enum TokenType type = tokenList->tokens[current_token].type;
		if (type == TOKEN_INSTRUCTION) {
			current_address += 2;
		} else if (type == TOKEN_PERIOD &&
			tokenList->count > current_token + 1) {
			if (symbol_directives(tokenList, symbolTable,
				    &current_address, &current_token)) {
				return true;
			}
		} else if (type == TOKEN_COLON && current_token > 0) {
			struct Token *prev =
				&tokenList->tokens[current_token - 1];
			if (prev->type == TOKEN_SYMBOL) {
				if (current_address % 2) {
					printf("Warning: %d: label is at an "
					       "odd address.\n",
						prev->line);
				}
				if (symbol_construct(symbolTable,
					    current_address, prev, false)) {
					return true;
				}
			}
		}
	}
	return false;
}