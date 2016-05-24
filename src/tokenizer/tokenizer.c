#include "tokenizer.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static int try_tokenize_identifier(const char *code, struct SimpleToken *token) {
	int length;
	if (!isalnum(code[0]))
		return 0;
	length = 1;
	while (isalnum(code[length]) || code[length] == '_')
		length++;
	token->type = IDENTIFIER;
	strncpy(token->value, code, length);
	return length;
}

static int try_tokenize_string(const char *code, struct SimpleToken *token) {
	int length;

	if (code[0] != '"')
		return 0;
	length = 1;
	while (code[length] != '"') {
		length++;
	}
	length++;
	token->type = STRING_LITERAL;
	strncpy(token->value, code, length);
	return length;
}


static int checkkeyword(const char *code, const char *keyword, int size) {
	return strncmp(code, keyword, size) || !isspace(code[size]);
}

void tokenize(const char *code, struct SimpleToken *tokens) {
	int i, tokenid, tmp;

	i = tokenid = 0;
	while (code[i]) {
		if (isspace(code[i])) {
			i++;
			continue;
		}
#define HANDLE_SINGLE_CHAR(val, etype) if (code[i] == val) {tokens[tokenid++].type = etype; i++; continue;}
		HANDLE_SINGLE_CHAR('{', BLOCK_START);
		HANDLE_SINGLE_CHAR('}', BLOCK_END);
		HANDLE_SINGLE_CHAR('(', PARENTHESE_START);
		HANDLE_SINGLE_CHAR(')', PARENTHESE_END);
		HANDLE_SINGLE_CHAR(':', COLON);
#define HANDLE_KEYWORD(keyword, size, etype) if (checkkeyword(code + i, keyword, size) == 0) {\
	tokens[tokenid++].type = etype; \
	i += size; continue; }
		HANDLE_KEYWORD("func", 4, FUNC);
		HANDLE_KEYWORD("int", 3, INT);

		if ((tmp = try_tokenize_string(code + i, tokens + tokenid)) > 0) {
			i += tmp;
			tokenid++;
			continue;
		}

		if ((tmp = try_tokenize_identifier(code + i, tokens + tokenid)) > 0) {
			i += tmp;
			tokenid++;
			continue;
		}

		i++;
	}
}

#define TEST_TOKENIZER
#ifdef TEST_TOKENIZER

char *readfile(char *path) {
	char *target;
	FILE *source;
	int size;
	
	source = fopen(path, "r");
	fseek(source, 0, SEEK_END);
	size = ftell(source);
	target = malloc(sizeof(char) * (size + 1));
	fseek(source, 0, SEEK_SET);
	fread(target, 1, size, source);
	target[size] = '\0';
	return target;
}

int main(int argc, char **argv) {
	char *source = readfile(argv[1]);
	struct SimpleToken tokens[100];
	
	tokenize(source, tokens);
	int i;
	for (i = 0; i < 30; i++) {
		printf("%d", tokens[i].type);
		if (tokens[i].type == IDENTIFIER) printf(": %s", tokens[i].value);
		if (tokens[i].type == STRING_LITERAL) printf(": '%s'", tokens[i].value);
		printf("\n");
	}
	return 0;
}

#endif
