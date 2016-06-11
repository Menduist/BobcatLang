#ifndef PARSER_H
#define PARSER_H

#include "../tokenizer/tokenizer.h"

enum nonterminal_node {
	TRANSLATION_UNIT = TOKEN_LAST,
	EXTERN_DECLARATION,
	STRUCT_DEFINITION,
	FUNCTION_DEFINITION,
	FUNCTION_CALL,
	FUNCTION_ARG_LIST,
	VARIABLE_DECLARATION,
	DECLARATOR,
	EXPRESSION_LIST,
	COMPOUND_STATEMENT,
	OPERATOR,
	PREFIX_OPERATOR,
	POSTFIX_OPERATOR,
	IF_STATEMENT,
	WHILE_STATEMENT,
	RETURN_STATEMENT,
	NODES_END
};

struct ast_node {
	int type;
	int childcount;
	int childsize;
	void *sem_val;
	struct ast_node *childs[];
};

struct parser {
	struct ast_node *root;
	struct SimpleToken *tokens;
};

struct ast_node *parse(struct SimpleToken *tokens);

extern char *all_names[];

#endif
