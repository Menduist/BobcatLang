#ifndef TOKENIZER_H
#define TOKENIZER_H

enum TokensTypes {
	NONE,
	FUNC,
	IDENTIFIER,
	INT,
	BLOCK_START,
	BLOCK_END,
	PARENTHESE_START,
	PARENTHESE_END,
	STRING_LITERAL,
	COLON,
	EXPRESSION_END
};

struct SimpleToken {
	enum TokensTypes type;
	char value[255];
};

void tokenize(const char *code, struct SimpleToken *tokens);
#endif
