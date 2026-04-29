#define GNU_SOURCE
#define _XOPEN_SOURCE 1 //NOLINT
#include <unistd.h> //NOLINT
#include <bits/getopt_core.h> //NOLINT
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "structures.h"
#include "encode.h"
#include "symbol.h"
#include "lexer.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("ERROR: no argument given.\n");
		return 1;
	}

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
			return 1;
		}
	}

	if (!ifilepath) {
		printf("ERROR: no input file provided\n");
		return 1;
	}

	if (ofilepath) {
		foutput = fopen(ofilepath, "wb");
	} else {
		foutput = fopen("a.out", "wb");
	}

	if (foutput == nullptr) {
		printf("ERROR: failed to open output file\n");
		return 1;
	}

	struct TokenList *tokenList = malloc(sizeof(struct TokenList));
	if (!tokenList) { goto error; }
	tokenList->count = 0;
	if (lexer(ifilepath, tokenList)) { goto error; }

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
	if (foutput) { (void)fclose(foutput); }
	exit(1);
}
