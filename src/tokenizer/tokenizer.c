#include "tokenizer.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "../utils.h"

static int try_tokenize_identifier(const char *code, struct simple_token *token) {
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

static int try_tokenize_string(const char *code, struct simple_token *token) {
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

static int try_tokenize_operator(const char *code, struct simple_token *token) {
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

struct simple_token *get_next_token(struct tokenizer *tokenizer) {
	struct simple_token *result = memalloc(struct simple_token, 1);
	int tmp;

	while (tokenizer->code[tokenizer->position] != '\0') {
		if (tokenizer->code[tokenizer->position] == '\n') {
			tokenizer->line++;
			tokenizer->linestart = tokenizer->position;
			tokenizer->position++;
			continue;
		}
		if (isspace(tokenizer->code[tokenizer->position])) {
			tokenizer->position++;
			continue;
		}
		tmp = try_discard_comment(tokenizer->code + tokenizer->position);
		if (tmp > 0) {
			tokenizer->position += tmp;
			continue;
		}
#define SET_TOKEN(etype, size) { result->type = etype; \
	result->line = tokenizer->line; \
	result->col = (tokenizer->position - tokenizer->linestart); \
	strncpy(result->value, tokenizer->code + tokenizer->position, size); \
	result->value[size] = 0; \
	tokenizer->position += size; \
	return result; }

#define HANDLE_SINGLE_CHAR(val, etype) if (tokenizer->code[tokenizer->position] == val) {\
	SET_TOKEN(etype, 1); }

		HANDLE_SINGLE_CHAR('{', TOKEN_BLOCK_START);
		HANDLE_SINGLE_CHAR('}', TOKEN_BLOCK_END);
		HANDLE_SINGLE_CHAR('(', TOKEN_PARENTHESE_START);
		HANDLE_SINGLE_CHAR(')', TOKEN_PARENTHESE_END);
		HANDLE_SINGLE_CHAR(':', TOKEN_COLON);
		HANDLE_SINGLE_CHAR(',', TOKEN_COMMA);
		HANDLE_SINGLE_CHAR(';', TOKEN_SEMICOLON);
#define HANDLE_KEYWORD(keyword, size, etype) if (checkkeyword(tokenizer->code + tokenizer->position, keyword, size) == 0) {\
	SET_TOKEN(etype, size); \
	}
		HANDLE_KEYWORD("func", 4, TOKEN_FUNC);
		HANDLE_KEYWORD("struct", 6, TOKEN_STRUCT);
		HANDLE_KEYWORD("if", 2, TOKEN_IF);
		HANDLE_KEYWORD("else", 4, TOKEN_ELSE);
		HANDLE_KEYWORD("while", 5, TOKEN_WHILE);
		HANDLE_KEYWORD("return", 6, TOKEN_RETURN);
		HANDLE_KEYWORD("break", 5, TOKEN_BREAK);
		HANDLE_KEYWORD("continue", 8, TOKEN_CONTINUE);

#define HANDLE_OTHER(name) if ((tmp = name(tokenizer->code + tokenizer->position, result)) > 0) { \
			tokenizer->position += tmp; \
			result->line = tokenizer->line; \
			result->col = (tokenizer->position - tokenizer->linestart); \
			return result; \
		}

		HANDLE_OTHER(try_tokenize_string);
		HANDLE_OTHER(try_tokenize_operator);
		HANDLE_OTHER(try_tokenize_identifier);

		printf("warning, unhandled %c\n", tokenizer->code[tokenizer->position]);
		tokenizer->position++;
	}
	result->type = TOKEN_NONE;
	return result;
}

void init_tokenizer(struct tokenizer *tok, const char *code) {
	tok->position = 0;
	tok->code = code;
	tok->line = 1;
	tok->linestart = 0;
}
