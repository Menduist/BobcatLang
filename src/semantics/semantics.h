#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "../parser/parser.h"
#include "../vector.h"

DECLAREVECTOR(sem_var, struct sem_variable);
DECLAREVECTOR(sem_func, struct sem_function);

enum sem_types {
	SEM_INTEGER,
	SEM_FLOATING,
	SEM_STRUCT
};

struct sem_type {
	char *name;
	enum sem_types type;
	int size;
	struct sem_var_vector fields;
};

struct sem_variable {
	char *name;
	struct sem_type *type;
};

struct sem_function {
	struct sem_var_vector args;
	struct sem_type *result_type;
};

struct sem_scope {
	struct sem_func_vector functions;
	struct sem_var_vector variables;
};

struct semantics {
	struct ast_node *ast;
};
#endif
