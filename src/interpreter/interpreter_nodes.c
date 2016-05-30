#include "interpreter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

struct interpreter_data *execute_node(struct interpreter *inter, struct ast_node *node);
struct interpreter_data *call_function(struct interpreter *inter, char *funcname);

node_intepretator nodes_interpretor[NODES_END];

void dump_data(enum interpreter_data_type type, void *data) {
	static char *types[] = {
		"INTER_INT",
		"INTER_STRING",
		"INTER_VAR"
		};

	printf("%s\n value: ", types[type]);
	switch (type) {
		case INTER_INT:
			printf("%d\n", *(int *)data);
			break;
		case INTER_STRING:
			printf("%s\n", data);
			break;
		case INTER_VAR:
			printf("var '%s':\n", ((struct interpreter_variable *)data)->name);
			dump_data(((struct interpreter_variable *)data)->type, ((struct interpreter_variable *)data)->value);
			printf("\n");
			break;
	}
}

struct interpreter_data *get_rvalue(struct interpreter_data *data) {
	if (data->type == INTER_VAR) {
		data->type = ((struct interpreter_variable *)data->data)->type;
		data->data = ((struct interpreter_variable *)data->data)->value;
	}
	return data;
}

struct interpreter_variable *get_lvalue(struct interpreter_data *data) {
	if (data->type == INTER_VAR) {
		return (struct interpreter_variable *)data->data;
	}
	return 0;
}

struct interpreter_data *interpret_function_print(struct interpreter *inter, struct ast_node *node) {
	switch (inter->args[0]->type) {
		case INTER_STRING:
			printf("%s", inter->args[0]->data);
			break;
		case INTER_INT:
			printf("%d\n", *(int *)inter->args[0]->data);
			break;
		default:
			printf("print: unhandled %d\n", inter->args[0]->type);
			break;
	}
	return 0;
}


struct interpreter_data *interpret_function_definition(struct interpreter *inter, struct ast_node *node) {
	execute_node(inter, node->childs[1]);
	return 0;
}

struct interpreter_data *interpret_compound_statement(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_scope *newscope = calloc(sizeof(struct interpreter_scope), 1);
	int i;


	newscope->parent = inter->scope;
	newscope->creator = node;
	inter->scope = newscope;

	for (i = 0; i < node->childcount; i++) {
		execute_node(inter, node->childs[i]);
	}

	inter->scope = newscope->parent;
	free(newscope);
	return 0;
}

struct interpreter_data *interpret_expression_list(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *result;
	int i;

	for (i = 0; i < node->childcount; i++) {
		result = execute_node(inter, node->childs[i]);
	}
	return result;
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

struct interpreter_data *interpret_identifier(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *result = calloc(sizeof(struct interpreter_data), 1);
	char *identifier = ((struct SimpleToken *)node)->value;

	if (isdigit(identifier[0])) {
		result->type = INTER_INT;
		result->data = calloc(sizeof(int), 1);
		*((int *)result->data) = atoi(identifier);
	}
	else {
		result->type = INTER_VAR;
		result->data = get_or_create_var(inter, identifier, INTER_INT);
	}
	return result;
}

struct interpreter_data *interpret_variable_declaration(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *result = calloc(sizeof(struct interpreter_data), 1);
	char *identifier = ((struct SimpleToken *)node)->value;

	result->type = INTER_VAR;
	result->data = get_or_create_var(inter, identifier, INTER_INT);

	return result;
}

struct interpreter_data *interpret_operator(struct interpreter *inter, struct ast_node *node) {
	char *operator = ((struct SimpleToken *)node->childs[0])->value;
	struct interpreter_data *result;
	
	if (strcmp(operator, "=") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = execute_node(inter, node->childs[1]);

		if (left->type != INTER_VAR) {
			printf("excepted variable !\n");
		}
		memcpy(((struct interpreter_variable *)left->data)->value, right->data, 4);
		result = right->data;
	}
	else if(strcmp(operator, "*") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data *), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)right->data * *(int *)left->data;
	}
	else if(strcmp(operator, "/") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data *), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data / *(int *)right->data;
	}
	else if (strcmp(operator, "==") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data *), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data == *(int *)right->data;
	}
	else {
		printf("unhandled operator %s\n", operator);
		return 0;
	}
	return result;
}

struct interpreter_data *interpret_prefix_operator(struct interpreter *inter, struct ast_node *node) {
	char *operator = ((struct SimpleToken *)node->childs[0])->value;
	struct interpreter_data *result;
	
	if (strcmp(operator, "++") == 0) {
		struct interpreter_data *right = execute_node(inter, node->childs[1]);
		struct interpreter_variable *right_val = get_lvalue(right);

		(*(int *)right_val->value)++;
		return right;
	}
	else {
		printf("unhandled prefix operator %s\n", operator);
		return 0;
	}
	return result;
}

struct interpreter_data *interpret_call_function(struct interpreter *inter, struct ast_node *node) {
	int i = 1;
	
	for(; i < node->childcount; i++) {
		inter->args[i - 1] = get_rvalue(execute_node(inter, node->childs[i]));
	}
	inter->argcount = node->childcount - 1;
	return call_function(inter, ((struct SimpleToken *)node->childs[0])->value);
}

struct interpreter_data *interpret_if_statement(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *condition;

	condition = get_rvalue(execute_node(inter, node->childs[1]));
	if (*(int *)condition->data)
		execute_node(inter, node->childs[2]);
	else if (node->childs[3])
		execute_node(inter, node->childs[3]);
	return 0;
}

void init_interpreter_nodes(void) {
	memset(nodes_interpretor, 0, sizeof(nodes_interpretor));

	nodes_interpretor[FUNCTION_DEFINITION] = interpret_function_definition;
	nodes_interpretor[COMPOUND_STATEMENT] = interpret_compound_statement;
	nodes_interpretor[EXPRESSION_LIST] = interpret_expression_list;
	nodes_interpretor[FUNCTION_CALL] = interpret_call_function;
	nodes_interpretor[TOKEN_STRING_LITERAL] = interpret_string_literal;
	nodes_interpretor[TOKEN_IDENTIFIER] = interpret_identifier;
	nodes_interpretor[OPERATOR] = interpret_operator;
	nodes_interpretor[PREFIX_OPERATOR] = interpret_prefix_operator;
	nodes_interpretor[VARIABLE_DECLARATION] = interpret_variable_declaration;
	nodes_interpretor[IF_STATEMENT] = interpret_if_statement;
}
