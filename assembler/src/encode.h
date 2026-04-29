#pragma once
#include <stdio.h>
#include "structures.h"

bool encode_and_write(struct TokenList *tokenList,
	struct SymbolTable *symbolTable, FILE *foutput);