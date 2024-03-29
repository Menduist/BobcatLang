#ifndef TOKENIZER_H
#define TOKENIZER_H

enum TokensTypes {
	TOKEN_NONE,
	TOKEN_COMMENT,
	TOKEN_FUNC,
	TOKEN_STRUCT,
	TOKEN_IDENTIFIER,
	TOKEN_OPERATOR,
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_WHILE,
	TOKEN_BLOCK_START,
	TOKEN_BLOCK_END,
	TOKEN_PARENTHESE_START,
	TOKEN_PARENTHESE_END,
	TOKEN_STRING_LITERAL,
	TOKEN_COLON,
	TOKEN_COMMA,
	TOKEN_SEMICOLON,
	TOKEN_EXPRESSION_END,
	TOKEN_RETURN,
	TOKEN_BREAK,
	TOKEN_CONTINUE,
	TOKEN_LAST
};

struct simple_token {
	int type;
	int line;
	int col;
	void *sem_val;
	char value[255];
};

struct tokenizer {
	const char *code;
	int position;
	int line, linestart;
};

void init_tokenizer(struct tokenizer *tok, const char *code);
struct simple_token *get_next_token(struct tokenizer *tokenizer);

#endif
