#include "interpreter.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

struct interpreter_data *call_function(struct interpreter *inter, char *funcname) {
	struct ast_node *function;

	if (strcmp(funcname, "print") == 0) {
		return interpret_function_print(inter, 0);
	}
	function = get_function(inter, funcname);
	return execute_node(inter, function);
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

void interpret(struct ast_node *program) {
	struct interpreter inter;

	init_interpreter_nodes();
	memset(&inter, 0, sizeof(struct interpreter));
	inter.program = program;
	register_all_functions(&inter);
	call_function(&inter, "main");
}

#ifdef TEST_INTERPRETER
char *readfile(char *path) {
	char *target;
	FILE *source;
	int size;
	
	source = fopen(path, "r");
	fseek(source, 0, SEEK_END);
	size = ftell(source);
	target = malloc(sizeof(char) * (size + 1));
	fseek(source, 0, SEEK_SET);
	fread(target, 1, size, source);
	target[size] = '\0';
	return target;
}

int main(int argc, char **argv) {
	struct SimpleToken tokens[300];
	char *source;
	struct ast_node *node;
	
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 100);
	tokenize(source, tokens);
	node = parse(tokens);
	interpret(node);
}

#endif
