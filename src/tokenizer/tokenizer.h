#ifndef TOKENIZER_H
#define TOKENIZER_H

enum TokensTypes {
	TOKEN_NONE,
	TOKEN_FUNC,
	TOKEN_IDENTIFIER,
	TOKEN_INT,
	TOKEN_BLOCK_START,
	TOKEN_BLOCK_END,
	TOKEN_PARENTHESE_START,
	TOKEN_PARENTHESE_END,
	TOKEN_STRING_LITERAL,
	TOKEN_COLON,
	TOKEN_EXPRESSION_END,
	TOKEN_LAST
};

struct SimpleToken {
	int type;
	int line;
	int col;
	char value[255];
};

void tokenize(const char *code, struct SimpleToken *tokens);
#endif
