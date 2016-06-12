#include "../semantics/semantics.h"
#include "interpreter.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define RETURNED (struct interpreter_data *)1
#define BREAKED (struct interpreter_data *)2
#define CONTINUED (struct interpreter_data *)3

struct interpreter_data *execute_node(struct interpreter *inter, struct ast_node *node);
struct interpreter_data *call_function(struct interpreter *inter, char *funcname);

node_intepretator nodes_interpretor[NODES_END];

static int is_jumped(struct interpreter_data *p) {
	return p != NULL && p <= CONTINUED;
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

struct interpreter_data *interpret_function_print(struct interpreter *inter, int type) {
	if (type)
		printf("%d\n", *(int *)inter->args[0]->data);
	else
		printf("%s", (char *)inter->args[0]->data);
	return 0;
}


struct interpreter_data *interpret_function_definition(struct interpreter *inter, struct ast_node *node) {
	/*printf("called %s with [", ((struct SimpleToken *)node->childs[0]->childs[0])->value);
	int i;
	for (i = 0; i < inter->argcount; i++) {
		dump_data(inter->args[i]->type, inter->args[i]->data);
	}
	printf("]\n");*/
	struct interpreter_scope *newscope = calloc(sizeof(struct interpreter_scope), 1);
	struct sem_scope *sem_scope = node->childs[1]->sem_val;
	int i;

	newscope->parent = inter->scope;
	newscope->creator = node;
	inter->scope = newscope;

	for (i = 0; i < sem_scope->variables.count; i++) {
		struct interpreter_variable *var = create_var(inter, sem_scope->variables.data[i]);
		
		if (i < inter->argcount)
			var->value = inter->args[i]->data;
	}

	for (i = 0; i < node->childs[1]->childcount; i++) {
		if (execute_node(inter, node->childs[1]->childs[i]) == RETURNED)
			break;
	}

	inter->scope = newscope->parent;
	free(newscope);
	return inter->returnvalue;
}

struct interpreter_data *interpret_compound_statement(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_scope *newscope = calloc(sizeof(struct interpreter_scope), 1);
	struct sem_scope *sem_scope = node->sem_val;
	struct interpreter_data *r;
	int i;

	newscope->parent = inter->scope;
	newscope->creator = node;
	inter->scope = newscope;

	for (i = 0; i < sem_scope->variables.count; i++) {
		create_var(inter, sem_scope->variables.data[i]);
	}

	for (i = 0; i < node->childcount; i++) {
		r = execute_node(inter, node->childs[i]);
		if (is_jumped(r))
			return r;
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
		if (is_jumped(result))
			return result;
	}
	return result;
}

struct interpreter_data *interpret_string_literal(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *result = malloc(sizeof(struct interpreter_data));
	char *string = strdup(((struct SimpleToken *)node)->value + 1);
	int i;
	int y;

	(void) inter;
	y = 0;
	for (i = 0; string[y] != '\"'; i++, y++) {
		if (string[y] == '\\' && string[y + 1] == 'n') {
			string[i] = '\n';
			y += 1;
			continue;
		}
		string[i] = string[y];
	}
	string[i] = '\0';

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
		result->data = get_var(inter, node->sem_val);
	}
	return result;
}

struct interpreter_data *interpret_variable_declaration(struct interpreter *inter, struct ast_node *node) {
	int i;

	for (i = 1; i < node->childcount; i++) {
		if (node->childs[i]->type == OPERATOR) {
			execute_node(inter, node->childs[i]);
		}
	}
	return 0;
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

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)right->data * *(int *)left->data;
	}
	else if(strcmp(operator, "+") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)right->data + *(int *)left->data;
	}
	else if(strcmp(operator, "/") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data / *(int *)right->data;
	}
	else if (strcmp(operator, "==") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data == *(int *)right->data;
	}
	else if (strcmp(operator, "<") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data < *(int *)right->data;
	}
	else if (strcmp(operator, ">") == 0) {
		struct interpreter_data *right = get_rvalue(execute_node(inter, node->childs[2]));
		struct interpreter_data *left = get_rvalue(execute_node(inter, node->childs[1]));

		result = calloc(sizeof(struct interpreter_data), 1);
		result->type = INTER_INT;
		result->data = malloc(sizeof(int));
		*(int *)result->data = *(int *)left->data > *(int *)right->data;
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
	else if (strcmp(operator, "--") == 0) {
		struct interpreter_data *right = execute_node(inter, node->childs[1]);
		struct interpreter_variable *right_val = get_lvalue(right);

		(*(int *)right_val->value)--;
		return right;
	}
	else {
		printf("unhandled prefix operator %s\n", operator);
		return 0;
	}
	return result;
}

struct interpreter_data *call_function_node(struct interpreter *inter, struct ast_node *node);
struct interpreter_data *interpret_call_function(struct interpreter *inter, struct ast_node *node) {
	int i = 0;
	
	for(; i < node->childs[2]->childcount; i++) {
		inter->args[i] = get_rvalue(execute_node(inter, node->childs[2]->childs[i]));
	}
	inter->argcount = node->childs[2]->childcount;
	if (((struct sem_function *)node->sem_val)->creatornode)
		return call_function_node(inter, ((struct sem_function *)node->sem_val)->creatornode);
	return call_function(inter, ((struct SimpleToken *)node->childs[1])->value);
}

struct interpreter_data *interpret_if_statement(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *condition;

	condition = get_rvalue(execute_node(inter, node->childs[1]));
	if (*(int *)condition->data)
		return execute_node(inter, node->childs[2]);
	else if (node->childs[3])
		return execute_node(inter, node->childs[3]);
	return 0;
}

struct interpreter_data *interpret_while_statement(struct interpreter *inter, struct ast_node *node) {
	struct interpreter_data *condition;
	struct interpreter_data *returnv;

	condition = get_rvalue(execute_node(inter, node->childs[1]));
	while (*(int *)condition->data) {
		returnv = execute_node(inter, node->childs[2]);
		if (returnv == RETURNED)
			return RETURNED;
		if (returnv == BREAKED)
			break;

		condition = get_rvalue(execute_node(inter, node->childs[1]));
	}
	return 0;
}

struct interpreter_data *interpret_jump_statement(struct interpreter *inter, struct ast_node *node) {
	if (node->childcount > 1) {
		inter->returnvalue = get_rvalue(execute_node(inter, node->childs[1]));
	}
	else
		inter->returnvalue = 0;
	switch (((struct SimpleToken *)node->childs[0])->type) {
		case TOKEN_BREAK:
			return BREAKED;
		case TOKEN_CONTINUE:
			return CONTINUED;
		case TOKEN_RETURN:
			return RETURNED;
	}
	return (struct interpreter_data *)-1;
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
	nodes_interpretor[WHILE_STATEMENT] = interpret_while_statement;
	nodes_interpretor[JUMP_STATEMENT] = interpret_jump_statement;
}
