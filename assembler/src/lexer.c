#include "lexer.h"
#include "structures.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "globals.h"

static void get_dir(const char *filepath, char *dir)
{
	const char *last_slash = strrchr(filepath, '/');
	if (last_slash) {
		size_t len = last_slash - filepath + 1;
		(void)snprintf(dir, len + 1, "%s", filepath);
	} else {
		dir[0] = '\0';
	}
}

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

static void get_token_string(
	struct TokenList *tokenList, char **cursor, int line_num)
{
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
				while (isxdigit((unsigned char)**cursor)) {
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
					.str[iter++] = (char)(val & 0xFF);
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
					val = (val << 3) | (**cursor - '0');
					(*cursor)++;
					count++;
				}
				tokenList->tokens[tokenList->count]
					.str[iter++] = (char)(val & 0xFF);
				(*cursor)--;
				break;
			}
			default:
				tokenList->tokens[tokenList->count]
					.str[iter++] = **cursor;
				break;
			}
		} else {
			tokenList->tokens[tokenList->count].str[iter++] =
				**cursor;
		}
		(*cursor)++;
	}
	if (**cursor == '"') { (*cursor)++; }
	tokenList->tokens[tokenList->count].str[iter] = '\0';
	tokenList->tokens[tokenList->count].len = iter;
	tokenList->count++;
}

static bool get_token_complex(
	struct TokenList *tokenList, char **cursor, int line_num)
{
	if (**cursor == '"') {
		get_token_string(tokenList, cursor, line_num);
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
		printf("ERROR: %d: unknown character %c\n", line_num,
			**cursor);
		return true;
	}

	return false;
}

static bool process_line_tokens(
	struct TokenList *tokenList, char *line, int line_num)
{
	char *cursor = line;
	while (*cursor != '\0') {
		if (isspace(*cursor)) { //NOLINT
			cursor++;
			continue;
		}
		if (get_token(tokenList, &cursor, line_num)) { return true; }
	}
	return false;
}

// NOLINTNEXTLINE(misc-no-recursion)
static bool lexer_recursive(const char *filepath, struct TokenList *tokenList,
	int depth, const char **include_stack);

// NOLINTNEXTLINE(misc-no-recursion)
static bool process_include(struct TokenList *tokenList,
	int tokens_before_line, const char *dir, int depth,
	const char **include_stack)
{
	struct Token *token = &tokenList->tokens[tokens_before_line];
	struct Token *next = &tokenList->tokens[tokens_before_line + 1];
	struct Token *next2 = &tokenList->tokens[tokens_before_line + 2];

	if (token->type == TOKEN_PERIOD && next->type == TOKEN_SYMBOL &&
		strcasecmp(next->str, "include") == 0 &&
		next2->type == TOKEN_STRING) {
		char inc_path[MAX_LINE_LEN];
		if (next2->str[0] == '/') {
			(void)snprintf(
				inc_path, MAX_LINE_LEN, "%s", next2->str);
		} else {
			(void)snprintf(inc_path, MAX_LINE_LEN, "%s%s", dir,
				next2->str);
		}
		tokenList->count = tokens_before_line;
		if (lexer_recursive(
			    inc_path, tokenList, depth + 1, include_stack)) {
			return true;
		}
	}
	return false;
}

// NOLINTNEXTLINE(misc-no-recursion)
static bool lexer_recursive(const char *filepath, struct TokenList *tokenList,
	int depth, const char **include_stack)
{
	if (depth >= 10) {
		printf("ERROR: maximum include depth exceeded\n");
		return true;
	}
	for (int i = 0; i < depth; i++) {
		if (strcmp(include_stack[i], filepath) == 0) {
			printf("ERROR: circular include detected: %s\n",
				filepath);
			return true;
		}
	}
	include_stack[depth] = filepath;

	FILE *finput = fopen(filepath, "r");
	if (!finput) {
		printf("ERROR: failed to open file: %s\n", filepath);
		return true;
	}

	char dir[MAX_LINE_LEN];
	get_dir(filepath, dir);

	char line[MAX_LINE_LEN];
	int line_num = 1;

	for (int i = 0; i < MAX_LINES; i++, line_num++) {
		if (!fgets(line, MAX_LINE_LEN, finput)) { break; }
		char *comment = strchr(line, ';');
		if (comment) {
			*comment = '\0';
		} else {
			line[MAX_LINE_LEN - 1] = '\0';
		}

		int tokens_before_line = tokenList->count;

		if (process_line_tokens(tokenList, line, line_num)) {
			(void)fclose(finput);
			return true;
		}

		int tokens_after_line = tokenList->count;

		if (tokens_after_line - tokens_before_line == 3) {
			if (process_include(tokenList, tokens_before_line, dir,
				    depth, include_stack)) {
				(void)fclose(finput);
				return true;
			}
		}
	}

	(void)fclose(finput);
	return false;
}

bool lexer(const char *filepath, struct TokenList *tokenList)
{
	const char *include_stack[10];

	if (lexer_recursive(filepath, tokenList, 0, include_stack)) {
		return true;
	}

	int dst = 0;
	for (int src = 0; src < tokenList->count; src++) {
		if (tokenList->tokens[src].type == TOKEN_MINUS &&
			src + 1 < tokenList->count &&
			tokenList->tokens[src + 1].type == TOKEN_NUMBER) {
			tokenList->tokens[src + 1].num_value =
				-tokenList->tokens[src + 1].num_value;
			size_t len = strlen(tokenList->tokens[src + 1].str);
			if (len + 1 < MAX_TOKEN_LEN) {
				memmove(tokenList->tokens[src + 1].str + 1,
					tokenList->tokens[src + 1].str,
					len + 1);
				tokenList->tokens[src + 1].str[0] = '-';
			}

			continue;
		}
		if (dst != src) {
			tokenList->tokens[dst] = tokenList->tokens[src];
		}
		dst++;
	}
	tokenList->count = dst;

	for (int i = 0; i < 7; i++) {
		tokenList->tokens[tokenList->count].type = TOKEN_NONE;
		tokenList->count++;
		if (tokenList->count >= MAX_TOKENS) { break; }
	}

	return false;
}