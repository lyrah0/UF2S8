#pragma once

#include <stdint.h>

enum {
	MAX_LINES = 1 << 16,
	MAX_TOKENS = 1 << 16,
	MAX_SYMBOLS = 1 << 10,
	MAX_TOKEN_LEN = 1 << 8,
	MAX_LINE_LEN = 1 << 8,
	INSTRUCTION_NUM = 27,
	MEMORY_SIZE = 65536,
	REGISTER_NUM = 15,
	CONDITION_NUM = 14
};

enum TokenType {
	TOKEN_NONE = 0,
	TOKEN_INSTRUCTION,
	TOKEN_REGISTER,
	TOKEN_CONDITION,
	TOKEN_SYMBOL,
	TOKEN_NUMBER,
	TOKEN_PARENTHESIS_OPEN,
	TOKEN_PARENTHESIS_CLOSE,
	TOKEN_BRACKET_OPEN,
	TOKEN_BRACKET_CLOSE,
	TOKEN_CURLY_BRACKET_OPEN,
	TOKEN_CURLY_BRACKET_CLOSE,
	TOKEN_COMMA,
	TOKEN_PERIOD,
	TOKEN_COLON,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_LT_SIGN,
	TOKEN_GT_SIGN,
	TOKEN_STRING
};

struct Token {
	enum TokenType type;
	int line;
	int len;
	long long num_value;
	char str[MAX_TOKEN_LEN];
};

struct TokenList {
	int count;
	struct Token tokens[MAX_TOKENS];
};

struct registers {
	const char *name;
	const int number;
};

struct conditions {
	const char *name;
	const int number;
};

struct Symbol {
	char name[MAX_TOKEN_LEN];
	uint16_t address;
};

struct SymbolTable {
	int count;
	struct Symbol symbols[MAX_SYMBOLS];
};
