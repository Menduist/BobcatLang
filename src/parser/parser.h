#ifndef PARSER_H

#include "../tokenizer/tokenizer.h"

enum nonterminal_node {
	TRANSLATION_UNIT = TOKEN_LAST,
	EXTERN_DECLARATION,
	FUNCTION_DEFINITION,
	FUNCTION_CALL,
	VARIABLE_DECLARATION,
	DECLARATOR,
	EXPRESSION_LIST,
	COMPOUND_STATEMENT,
	OPERATOR,
	PREFIX_OPERATOR,
	POSTFIX_OPERATOR,
	IF_STATEMENT,
	WHILE_STATEMENT
};

struct ast_node {
	int type;
	int childcount;
	int childsize;
	struct ast_node *childs[];
};

struct parser {
	struct ast_node *root;
	struct SimpleToken *tokens;
};

struct ast_node *parse(struct SimpleToken *tokens);

#endif
