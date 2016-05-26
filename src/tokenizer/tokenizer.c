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
	token->type = TOKEN_IDENTIFIER;
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
	token->type = TOKEN_STRING_LITERAL;
	strncpy(token->value, code, length);
	return length;
}

static int __always_inline is_operator(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=';
}

static int try_tokenize_operator(const char *code, struct SimpleToken *token) {
	int length;

	length = 0;
	while (is_operator(code[length]) && length <= 3) {
		length++;
	}

	if (length > 0) {
		token->type = TOKEN_OPERATOR;
		strncpy(token->value, code, length);
	}
	return length;
}


static int checkkeyword(const char *code, const char *keyword, int size) {
	return strncmp(code, keyword, size) || !isspace(code[size]);
}

void tokenize(const char *code, struct SimpleToken *tokens) {
	int i, tokenid, tmp;
	int line, linestart;

	i = tokenid = line = linestart = 0;
	while (code[i]) {
		if (code[i] == '\n') {
			line++;
			linestart = i;
			i++;
			continue;
		}
		if (isspace(code[i])) {
			i++;
			continue;
		}
#define SET_TOKEN(etype, size) { tokens[tokenid].type = etype; tokens[tokenid].line = line; tokens[tokenid].col = (i - linestart); strncpy(tokens[tokenid].value, code + i, size); tokenid++;}
#define HANDLE_SINGLE_CHAR(val, etype) if (code[i] == val) {SET_TOKEN(etype, 1); i++; continue;}
		HANDLE_SINGLE_CHAR('{', TOKEN_BLOCK_START);
		HANDLE_SINGLE_CHAR('}', TOKEN_BLOCK_END);
		HANDLE_SINGLE_CHAR('(', TOKEN_PARENTHESE_START);
		HANDLE_SINGLE_CHAR(')', TOKEN_PARENTHESE_END);
		HANDLE_SINGLE_CHAR(':', TOKEN_COLON);
		HANDLE_SINGLE_CHAR(',', TOKEN_COMMA);
#define HANDLE_KEYWORD(keyword, size, etype) if (checkkeyword(code + i, keyword, size) == 0) {\
	SET_TOKEN(etype, size); \
	i += size; continue; }
		HANDLE_KEYWORD("func", 4, TOKEN_FUNC);
		HANDLE_KEYWORD("int", 3, TOKEN_INT);

#define HANDLE_OTHER(name) if ((tmp = name(code + i, tokens + tokenid)) > 0) { \
			i += tmp; \
			tokens[tokenid].line = line; tokens[tokenid].col = (i - linestart); \
			tokenid++; \
			continue; \
		}

		HANDLE_OTHER(try_tokenize_string);
		HANDLE_OTHER(try_tokenize_operator);
		HANDLE_OTHER(try_tokenize_identifier);

		printf("warning, unhandled %c\n", code[i]);
		i++;
	}
}

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
		printf("%d: %s\n", tokens[i].type, tokens[i].value);
	}
	return 0;
}

#endif
