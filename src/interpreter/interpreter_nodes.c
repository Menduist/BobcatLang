#include "interpreter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct interpreter_data *execute_node(struct interpreter *inter, struct ast_node *node);
struct interpreter_data *call_function(struct interpreter *inter, char *funcname);

node_intepretator nodes_interpretor[NODES_END];

struct interpreter_data *interpret_function_print(struct interpreter *inter, struct ast_node *node) {
	printf("%s", inter->args[0]->data);
	return 0;
}


struct interpreter_data *interpret_function_definition(struct interpreter *inter, struct ast_node *node) {
	execute_node(inter, node->childs[1]);
	return 0;
}

struct interpreter_data *interpret_compound_statement(struct interpreter *inter, struct ast_node *node) {
	return execute_node(inter, node->childs[0]);
}

struct interpreter_data *interpret_expression_list(struct interpreter *inter, struct ast_node *node) {
	int i;

	for (i = 0; i < node->childcount; i++) {
		execute_node(inter, node->childs[i]);
	}
	return 0;
}

struct interpreter_data *interpret_string_literal(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *result = malloc(sizeof(struct interpreter_data));
	char *string = strdup(((struct SimpleToken *)node)->value + 1);
	int i;
	int y;

	y = 0;
	for (i = 0; string[y]; i++, y++) {
		if (string[y] == '\\' && string[y + 1] == 'n') {
			string[i] = '\n';
			y += 2;
			continue;
		}
		string[i] = string[y];
	}
	string[i] = string[y];

	result->type = INTER_STRING;
	result->data = string;
	return result;
}

struct interpreter_data *interpret_call_function(struct interpreter *inter, struct ast_node *node) {
	int i = 1;
	
	for(; i < node->childcount; i++) {
		inter->args[i - 1] = execute_node(inter, node->childs[i]);
	}
	inter->argcount = node->childcount - 1;
	return call_function(inter, ((struct SimpleToken *)node->childs[0])->value);
}

void init_interpreter_nodes(void) {
	memset(nodes_interpretor, 0, sizeof(nodes_interpretor));

	nodes_interpretor[FUNCTION_DEFINITION] = interpret_function_definition;
	nodes_interpretor[COMPOUND_STATEMENT] = interpret_compound_statement;
	nodes_interpretor[EXPRESSION_LIST] = interpret_expression_list;
	nodes_interpretor[FUNCTION_CALL] = interpret_call_function;
	nodes_interpretor[TOKEN_STRING_LITERAL] = interpret_string_literal;
}
