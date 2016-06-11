#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "../parser/parser.h"

enum interpreter_data_type {
	INTER_INT,
	INTER_STRING,
	INTER_VAR
};

struct interpreter_data {
	enum interpreter_data_type type;
	void *data;
};

struct interpreter_variable {
	enum interpreter_data_type type;
	struct sem_variable *var;
	void *value;
};

struct interpreter_scope {
	struct interpreter_scope *parent;
	struct ast_node *creator;
	struct interpreter_variable variables[20];
};

struct interpreter {
	struct ast_node *program;
	struct ast_node *function_list[20];
	struct interpreter_data *args[5];
	struct interpreter_scope *scope;
	struct interpreter_scope *global_scope;
	int argcount;
};

typedef struct interpreter_data *(*node_intepretator)(struct interpreter *, struct ast_node *);

struct interpreter_variable *get_var(struct interpreter *inter, struct sem_variable *var);
struct interpreter_variable *create_var(struct interpreter *inter, struct sem_variable *var);

void interpret(struct ast_node *program);
#endif
