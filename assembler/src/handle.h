#pragma once
#include "structures.h"

enum BitOpType { BIT_OP_BST = 0, BIT_OP_BIC = 1, BIT_OP_BTS = 2 };

bool handle_ss(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_sd(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, uint16_t base);
bool handle_unary(const struct TokenList *tokenList, int *current_token,
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
	uint16_t *machine_code, uint16_t reg_base, uint16_t imm_base);
bool handle_bit_ops(const struct TokenList *tokenList, int *current_token,
	uint16_t *machine_code, enum BitOpType op_type);
bool handle_li(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code);
bool handle_loadstore(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool load);
bool handle_branch(const struct TokenList *tokenList,
	const struct SymbolTable *symbolTable, int *current_token,
	uint16_t *machine_code, bool link, uint16_t current_address);
