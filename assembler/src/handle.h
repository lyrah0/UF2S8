#pragma once
#include "structures.h"

bool handle_ss(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_sd(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_sdss(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_mov(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code);
bool handle_ds(const struct TokenList *tokenList, int *current_token,
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
	uint16_t *machine_code, bool load);
bool handle_branch_cond(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool link, uint16_t current_address);
