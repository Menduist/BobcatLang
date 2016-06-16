#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "../parser/parser.h"
#include "../vector.h"

DECLAREVECTOR(sem_var, struct sem_variable *);
DECLAREVECTOR(sem_func, struct sem_function *);
DECLAREVECTOR(sem_data_types, struct sem_type *);

enum sem_types {
	SEM_SCOPE,
	SEM_FUNCTION,
	SEM_VARIABLE,
	SEM_TYPE
};

enum sem_data_types {
	SEM_INTEGER,
	SEM_FLOATING,
	SEM_STRUCT
};

struct sem_type {
	enum sem_types type;
	char *name;
	void *gendata;
	struct ast_node *creatornode;
	enum sem_data_types datatype;
	int size;
	struct sem_var_vector fields;
};

struct sem_variable {
	enum sem_types type;
	char *name;
	void *gendata;
	struct ast_node *creatornode;
	struct sem_type *datatype;
};

struct sem_function {
	enum sem_types type;
	char *name;
	void *gendata;
	struct ast_node *creatornode;
	struct sem_var_vector args;
	struct sem_type *result_type;
};

struct sem_scope {
	enum sem_types type;
	char *name;
	void *gendata;
	struct ast_node *creatornode;
	struct sem_scope *parent_scope;
	struct sem_func_vector functions;
	struct sem_var_vector variables;
	struct sem_data_types_vector types;

	int implicit_var_count;
};

struct semantics {
	struct ast_node *ast;
	struct sem_scope *current_scope;
	struct sem_scope *global_scope;
};

int run_semantical_analyzer(struct ast_node *ast);

struct sem_type *find_node_type(struct semantics *sem, struct ast_node *node);
struct sem_type *get_type(struct semantics *sem, char *name);
struct sem_variable *get_variable(struct semantics *sem, char *name);

void print_node(struct ast_node *node, int level);
void init_semantical_analyzer(void);
void init_sem_typefinder(void);

void free_sem_ast_node(struct ast_node *node);

#endif
