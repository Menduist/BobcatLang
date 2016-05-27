#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../parser/parser.h"

enum interpreter_data_type {
	INTER_INT,
	INTER_STRING
};

struct interpreter_data {
	enum interpreter_data_type type;
	void *data;
};

struct interpreter {
	struct ast_node *program;
	struct ast_node *function_list[20];
	struct interpreter_data *args[5];
	int argcount;
};

typedef struct interpreter_data * (*node_intepretator)(struct interpreter *, struct ast_node *);

void interpret(struct ast_node *program);
#endif
