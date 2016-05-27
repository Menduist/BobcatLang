#ifndef PARSER_H

#include "../tokenizer/tokenizer.h"

enum nonterminal_node {
	TRANSLATION_UNIT = TOKEN_LAST,
	EXTERN_DECLARATION,
	FUNCTION_DEFINITION,
	FUNCTION_CALL,
	DECLARATOR,
	COMPOUND_STATEMENT,
	OPERATOR,
	PREFIX_OPERATOR,
	POSTFIX_OPERATOR,
	IF_STATEMENT,
	WHILE_STATEMENT
};

struct ast_node {
	int type;
	struct ast_node *childs[20];
};

struct parser {
	struct ast_node *root;
	struct SimpleToken *tokens;
};

struct ast_node *parse(struct SimpleToken *tokens);

#endif
