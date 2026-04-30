#pragma once
#include "structures.h"

bool handle_spp(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_pp(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, bool isPush);
bool handle_mov(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code);
bool handle_crr(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_rrr(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_add(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code);
bool handle_shift(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_li(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code);
bool handle_loadstore(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_branch_cond(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool link, uint16_t current_address);
