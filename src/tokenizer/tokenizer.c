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
	strncpy(token->value, code, (size_t) length);
	token->value[length] = 0;
	return length;
}

static int try_discard_comment(const char *code) {
	int length;

	if (code[0] != '/')
		return 0;
	length = 1;
	if (code[1] == '*') {
		while (code[length + 1] != '\0' &&
				(code[length] != '*' || code[length + 1] != '/'))
			length++;
		length += 2;
	}
	else if (code[1] == '/') {
		while (code[length] != '\0' && code[length] != '\n')
			length++;
	}
	else {
		return 0;
	}
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
	strncpy(token->value, code, (size_t) length);

	token->value[length] = '\0';
	return length;
}

inline static int is_operator(char c) {
	return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' ||
		c == '<' || c == '>' || c == '.' || c == '[' || c == ']';
}

static int try_tokenize_operator(const char *code, struct SimpleToken *token) {
	int length;

	length = 0;
	while (is_operator(code[length]) && length <= 2) {
		length++;
		if (code[length - 1] == ']')
			break;
	}

	if (length > 0) {
		token->type = TOKEN_OPERATOR;
		strncpy(token->value, code, (size_t) length);
		token->value[length] = 0;
	}
	return length;
}


static int checkkeyword(const char *code, const char *keyword, int size) {
	return strncmp(code, keyword, (size_t) size) || !isspace(code[size]);
}

void tokenize(const char *code, struct SimpleToken *tokens) {
	int i, tokenid, tmp;
	int line, linestart;

	i = tokenid = linestart = 0;
	line = 1;
	while (code[i] != '\0') {
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
		tmp = try_discard_comment(code + i);
		if (tmp > 0) {
			i += tmp;
			continue;
		}
#define SET_TOKEN(etype, size) { tokens[tokenid].type = etype; tokens[tokenid].line = line; tokens[tokenid].col = (i - linestart); strncpy(tokens[tokenid].value, code + i, size); tokens[tokenid].value[size] = 0; tokenid++;}
#define HANDLE_SINGLE_CHAR(val, etype) if (code[i] == val) {SET_TOKEN(etype, 1); i++; continue;}
		HANDLE_SINGLE_CHAR('{', TOKEN_BLOCK_START);
		HANDLE_SINGLE_CHAR('}', TOKEN_BLOCK_END);
		HANDLE_SINGLE_CHAR('(', TOKEN_PARENTHESE_START);
		HANDLE_SINGLE_CHAR(')', TOKEN_PARENTHESE_END);
		HANDLE_SINGLE_CHAR(':', TOKEN_COLON);
		HANDLE_SINGLE_CHAR(',', TOKEN_COMMA);
		HANDLE_SINGLE_CHAR(';', TOKEN_SEMICOLON);
#define HANDLE_KEYWORD(keyword, size, etype) if (checkkeyword(code + i, keyword, size) == 0) {\
	SET_TOKEN(etype, size); \
	i += size; continue; }
		HANDLE_KEYWORD("func", 4, TOKEN_FUNC);
		HANDLE_KEYWORD("struct", 6, TOKEN_STRUCT);
		HANDLE_KEYWORD("if", 2, TOKEN_IF);
		HANDLE_KEYWORD("else", 4, TOKEN_ELSE);
		HANDLE_KEYWORD("while", 5, TOKEN_WHILE);
		HANDLE_KEYWORD("return", 6, TOKEN_RETURN);
		HANDLE_KEYWORD("break", 5, TOKEN_BREAK);
		HANDLE_KEYWORD("continue", 8, TOKEN_CONTINUE);

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
	tokens[tokenid].type = TOKEN_NONE;
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
