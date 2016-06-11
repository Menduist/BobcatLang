#include "interpreter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../utils.h"
#include "../semantics/semantics.h"

void init_interpreter_nodes(void);

void init_interpreter(void) {
	init_interpreter_nodes();
}

extern node_intepretator nodes_interpretor[NODES_END];
struct interpreter_data *execute_node(struct interpreter *inter, struct ast_node *node) {
	node_intepretator node_inter;

	node_inter = nodes_interpretor[node->type];
	if (!node_inter) {
		printf("error ! not implemented %s\n", all_names[node->type]);
		return NULL;
	}
	return node_inter(inter, node);
}

struct ast_node *get_function(struct interpreter *inter, char *funcname) {
	int i;
	struct SimpleToken *tok;

	for (i = 0; inter->function_list[i]; i++) {
		tok = (struct SimpleToken *)inter->function_list[i]->childs[0]->childs[0];
		if (strcmp(tok->value, funcname) == 0) {
			return inter->function_list[i];
		}
	}
	return 0;
}

struct interpreter_data *interpret_function_print(struct interpreter *inter, struct ast_node *node);
struct interpreter_data *call_function(struct interpreter *inter, char *funcname) {
	struct ast_node *function;

	if (strncmp(funcname, "print", 5) == 0 && strlen(funcname) > 5) {
		return interpret_function_print(inter, 0);
	}
	function = get_function(inter, funcname);
	if (function == 0) {
		printf("function not found '%s'\n", funcname);
		return NULL;
	}
	return execute_node(inter, function);
}

struct interpreter_data *call_function_node(struct interpreter *inter, struct ast_node *node) {
	return execute_node(inter, node);
}

void register_all_functions(struct interpreter *inter) {
	int i;
	int y;

	y = 0;
	for (i = 0; i < inter->program->childcount; i++) {
		if (inter->program->childs[i]->type == FUNCTION_DEFINITION) {
			inter->function_list[y++] = inter->program->childs[i];
		}
	}
}

static void create_global_scope(struct interpreter *inter) {
	inter->global_scope = memalloc(struct interpreter_scope, 1);
	inter->scope = inter->global_scope;
}

struct interpreter_variable *get_var(struct interpreter *inter, char *name) {
	struct interpreter_scope *scope = inter->scope;
	int i;

	while (scope) {
		i = 0;
		while (scope->variables[i].name) {
			if (strcmp(name, scope->variables[i].name) == 0)
				return &scope->variables[i];
			i++;
		}
		scope = scope->parent;
	}
	return 0;
}

struct interpreter_variable *create_var(struct interpreter *inter, char *name, enum interpreter_data_type type) {
	struct interpreter_variable *result;
	int i = 0;

	while (inter->scope->variables[i].name)
		i++;
	result = inter->scope->variables + i;
	result->type = type;
	result->name = name;
	switch (type) {
		case INTER_INT:
			result->value = memalloc(int, 1);
			break;
		case INTER_STRING:
		case INTER_VAR:
			result->value = 0;
			break;
	}
	return result;
}

struct interpreter_variable *get_or_create_var(struct interpreter *inter, char *name, enum interpreter_data_type type) {
	struct interpreter_variable *result = get_var(inter, name);
	if (result)
		return result;
	return create_var(inter, name, type);
}

void execute_top_level(struct interpreter *inter) {
	int i;

	for (i = 0; i < inter->program->childcount; i++) {
		if (inter->program->childs[i]->type != FUNCTION_DEFINITION &&
			inter->program->childs[i]->type != STRUCT_DEFINITION) {
			execute_node(inter, inter->program->childs[i]);
		}
	}
}

void interpret(struct ast_node *program) {
	struct interpreter inter;

	init_interpreter_nodes();
	memset(&inter, 0, sizeof(struct interpreter));
	inter.program = program;
	create_global_scope(&inter);
	register_all_functions(&inter);
	execute_top_level(&inter);
	call_function(&inter, "main");
}

#ifdef TEST_INTERPRETER

int main(int argc, char **argv) {
	struct SimpleToken tokens[300];
	char *source;
	struct ast_node *node;
	
	if (argc < 2) {
		printf("usage %s [file]\n", argv[0]);
		return 1;
	}
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 100);
	tokenize(source, tokens);
	node = parse(tokens);
	init_semantical_analyzer();
	run_semantical_analyzer(node);
	interpret(node);
}

#endif
