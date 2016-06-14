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
	JUMP_STATEMENT,
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
	struct simple_token *token;
	struct simple_token *peek;
	struct simple_token last_token;
	struct tokenizer tokenizer;
};

void free_ast_node(struct ast_node *node);
struct ast_node *parse(const char *code);

extern char *all_names[];

#endif
