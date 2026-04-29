#define GNU_SOURCE
#define _XOPEN_SOURCE 1 //NOLINT
#include <unistd.h> //NOLINT
#include <bits/getopt_core.h> //NOLINT
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "structures.h"
#include "globals.h"
#include "encode.h"
#include "symbol.h"

static void get_instr_reg_cond_sym(struct TokenList *tokenList)
{
	for (int i = 0; i < INSTRUCTION_NUM; i++) {
		if (!strcasecmp(tokenList->tokens[tokenList->count].str,
			    instr_list[i])) {
			tokenList->tokens[tokenList->count].type =
				TOKEN_INSTRUCTION;
			return;
		}
	}
	if (tokenList->tokens[tokenList->count].type == TOKEN_NONE) {
		for (int i = 0; i < REGISTER_NUM; i++) {
			if (!strcasecmp(
				    tokenList->tokens[tokenList->count].str,
				    registers[i].name)) {
				tokenList->tokens[tokenList->count].type =
					TOKEN_REGISTER;
				tokenList->tokens[tokenList->count].num_value =
					registers[i].number;
				return;
			}
		}
	}
	if (tokenList->tokens[tokenList->count].type == TOKEN_NONE) {
		for (int i = 0; i < CONDITION_NUM; i++) {
			if (!strcasecmp(
				    tokenList->tokens[tokenList->count].str,
				    conditions[i].name)) {
				tokenList->tokens[tokenList->count].type =
					TOKEN_CONDITION;
				tokenList->tokens[tokenList->count].num_value =
					conditions[i].number;
				return;
			}
		}
	}
	if (tokenList->tokens[tokenList->count].type == TOKEN_NONE) {
		tokenList->tokens[tokenList->count].type = TOKEN_SYMBOL;
	}
}

static bool get_token_complex(
	struct TokenList *tokenList, char **cursor, int line_num)
{
	if (**cursor == '"') {
		tokenList->tokens[tokenList->count].type = TOKEN_STRING;
		tokenList->tokens[tokenList->count].line = line_num;
		(*cursor)++;
		int iter = 0;
		while ((iter < MAX_TOKEN_LEN - 1) && (**cursor != '"') &&
			(**cursor != '\0')) {
			if (**cursor == '\\') {
				(*cursor)++;
				if (**cursor == '\0') { break; }
				switch (**cursor) {
				case 'n':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\n';
					break;
				case 't':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\t';
					break;
				case 'r':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\r';
					break;
				case 'v':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\v';
					break;
				case 'f':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\f';
					break;
				case 'b':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\b';
					break;
				case 'a':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\a';
					break;
				case '\\':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\\';
					break;
				case '\'':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\'';
					break;
				case '"':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\"';
					break;
				case '?':
					tokenList->tokens[tokenList->count]
						.str[iter++] = '\?';
					break;
				case 'x': {
					(*cursor)++;
					int val = 0;
					while (isxdigit(
						(unsigned char)**cursor)) {
						int digit =
							isdigit((
								unsigned char)**cursor) ?
							(**cursor - '0') :
							(tolower((
								 unsigned char)**cursor) -
								'a' + 10);
						val = (val << 4) | digit;
						(*cursor)++;
					}
					tokenList->tokens[tokenList->count]
						.str[iter++] =
						(char)(val & 0xFF);
					(*cursor)--;
					break;
				}
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7': {
					int val = 0;
					int count = 0;
					while (count < 3 && **cursor >= '0' &&
						**cursor <= '7') {
						val = (val << 3) |
							(**cursor - '0');
						(*cursor)++;
						count++;
					}
					tokenList->tokens[tokenList->count]
						.str[iter++] =
						(char)(val & 0xFF);
					(*cursor)--;
					break;
				}
				default:
					tokenList->tokens[tokenList->count]
						.str[iter++] = **cursor;
					break;
				}
			} else {
				tokenList->tokens[tokenList->count]
					.str[iter++] = **cursor;
			}
			(*cursor)++;
		}
		if (**cursor == '"') { (*cursor)++; }
		tokenList->tokens[tokenList->count].str[iter] = '\0';
		tokenList->tokens[tokenList->count].len = iter;
		tokenList->count++;
	} else if (isalpha(**cursor) || **cursor == '_') {
		tokenList->tokens[tokenList->count].type = TOKEN_NONE;
		tokenList->tokens[tokenList->count].line = line_num;
		int iter = 0;
		while ((iter < MAX_TOKEN_LEN - 2) &&
			(isalnum(**cursor) || **cursor == '_')) {
			tokenList->tokens[tokenList->count].str[iter++] =
				**cursor;
			(*cursor)++;
		}
		tokenList->tokens[tokenList->count].str[iter] = '\0';

		get_instr_reg_cond_sym(tokenList);

		tokenList->count++;
	} else if (isdigit(**cursor)) {
		tokenList->tokens[tokenList->count].type = TOKEN_NUMBER;
		tokenList->tokens[tokenList->count].line = line_num;
		int iter = 0;
		while ((iter < MAX_TOKEN_LEN - 2) && (isalnum(**cursor))) {
			tokenList->tokens[tokenList->count].str[iter++] =
				**cursor;
			(*cursor)++;
		}
		tokenList->tokens[tokenList->count].str[iter] = '\0';
		tokenList->tokens[tokenList->count].num_value = (int)strtol(
			tokenList->tokens[tokenList->count].str, nullptr, 0);
		tokenList->count++;
	} else {
		return false;
	}
	return true;
}

static bool get_token(struct TokenList *tokenList, char **cursor, int line_num)
{
	if (**cursor == '(') {
		tokenList->tokens[tokenList->count].type =
			TOKEN_PARENTHESIS_OPEN;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == ')') {
		tokenList->tokens[tokenList->count].type =
			TOKEN_PARENTHESIS_CLOSE;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '[') {
		tokenList->tokens[tokenList->count].type = TOKEN_BRACKET_OPEN;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == ']') {
		tokenList->tokens[tokenList->count].type = TOKEN_BRACKET_CLOSE;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '{') {
		tokenList->tokens[tokenList->count].type =
			TOKEN_CURLY_BRACKET_OPEN;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '}') {
		tokenList->tokens[tokenList->count].type =
			TOKEN_CURLY_BRACKET_CLOSE;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == ',') {
		tokenList->tokens[tokenList->count].type = TOKEN_COMMA;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '.') {
		tokenList->tokens[tokenList->count].type = TOKEN_PERIOD;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == ':') {
		tokenList->tokens[tokenList->count].type = TOKEN_COLON;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '+') {
		tokenList->tokens[tokenList->count].type = TOKEN_PLUS;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '-') {
		tokenList->tokens[tokenList->count].type = TOKEN_MINUS;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '<') {
		tokenList->tokens[tokenList->count].type = TOKEN_LT_SIGN;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (**cursor == '>') {
		tokenList->tokens[tokenList->count].type = TOKEN_GT_SIGN;
		tokenList->tokens[tokenList->count].line = line_num;
		tokenList->count++;
		(*cursor)++;
	} else if (get_token_complex(tokenList, cursor, line_num)) {
	} else {
		printf("ERROR: %d: unknown charactr %c\n", line_num, **cursor);
		goto error;
	}

	return false;
error:
	return true;
}

static bool lexer(FILE *finput, struct TokenList *tokenList)
{
	char line[MAX_LINE_LEN];
	char *cursor = nullptr;
	int line_num = 1;

	for (int i = 0; i < MAX_LINES; i++, line_num++) {
		if (!fgets(line, MAX_LINE_LEN, finput)) { break; }
		char *comment = strchr(line, ';');
		if (comment) {
			*comment = '\0';
		} else {
			line[MAX_LINE_LEN - 1] = '\0';
		}
		cursor = &line;
		while (*cursor != '\0') {
			if (isspace(*cursor)) { //NOLINT
				cursor++;
				continue;
			}
			if (get_token(tokenList, &cursor, line_num)) {
				goto error;
			}
		}
	}

	for (int i = 0; i < 7; i++) {
		tokenList->tokens[tokenList->count].type = TOKEN_NONE;
		tokenList->count++;
		if (tokenList->count >= MAX_TOKENS) { break; }
	}

	return false;
error:
	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("ERROR: no argument given.\n");
		return 1;
	}

	FILE *finput = nullptr;
	char *ifilepath = nullptr;
	FILE *foutput = nullptr;
	char *ofilepath = nullptr;

	int opt = 0;

	while ((opt = getopt(argc, argv, "i:o:")) != -1) {
		switch (opt) {
		case 'i':
			ifilepath = optarg;
			break;
		case 'o':
			ofilepath = optarg;
			break;
		default:
			printf("Usage: %s [-i assembly file] [-o output "
			       "file]\n",
				argv[0]);
		}
	}

	if (ifilepath) {
		finput = fopen(ifilepath, "r");
	} else {
		printf("ERROR: no input file\n");
		goto error;
	}
	if (finput == nullptr) {
		printf("ERROR: failed to open input file\n");
		goto error;
	}
	if (ofilepath) {
		foutput = fopen(ofilepath, "w");
	} else {
		foutput = fopen("a.out", "w");
	}
	if (foutput == nullptr) {
		printf("ERROR: failed to open output file\n");
		goto error;
	}

	struct TokenList *tokenList = malloc(sizeof(struct TokenList));
	if (!tokenList) { goto error; }
	tokenList->count = 0;
	if (lexer(finput, tokenList)) { goto error; }

	(void)fclose(finput);
	finput = nullptr;

	struct SymbolTable *symbolTable = malloc(sizeof(struct SymbolTable));
	if (!symbolTable) { goto error; }
	symbolTable->count = 0;
	if (symbol_build_table(tokenList, symbolTable)) { goto error; }

	if (encode_and_write(tokenList, symbolTable, foutput)) { goto error; }

	(void)fclose(foutput);
	free(symbolTable);
	free(tokenList);

	exit(0);
error:
	if (finput) { (void)fclose(finput); }
	if (foutput) { (void)fclose(foutput); }
	exit(1);
}
