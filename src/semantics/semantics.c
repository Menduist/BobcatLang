#include "semantics.h"
#include <string.h>
#include "../utils.h"
#include <stdio.h>

enum passes {
	PASS_TYPES,
	PASS_FIELDS,
	PASS_FUNCS = PASS_FIELDS,
	PASS_VARS,
	PASS_IDENTIFIERS,
	PASS_ASSIGN,
	PASS_END
};

typedef int (*node_semanticalizer)(struct semantics *, struct ast_node *, int pass);

node_semanticalizer passes[NODES_END];

static int sem_pass(struct semantics *sem, struct ast_node *node, int pass) {
	int i;
	struct sem_scope *scopebackup = sem->current_scope;

	if (passes[node->type] != NULL) {
		if (passes[node->type](sem, node, pass) != 0) {
			goto end;
		}
	}
	if (node->type < TOKEN_LAST)
		goto end;

	for (i = 0; i < node->childcount; i++) {
		sem_pass(sem, node->childs[i], pass);
	}
end:
	sem->current_scope = scopebackup;
	return 0;
}

static struct sem_type *get_type(struct semantics *sem, char *name) {
	struct sem_scope *scope = sem->current_scope;
	struct sem_type *result = 0;
	int i;

	while (scope) {
		for (i = 0; i < scope->types.count; i++) {
			if (strcmp(name, scope->types.data[i]->name) == 0) {
				if (result) {
					printf("conflict\n");
				}
				result = scope->types.data[i];
			}

		}
		if (result)
			return result;

		scope = scope->parent_scope;
	}
	return 0;
}

static struct sem_variable *get_variable(struct semantics *sem, char *name) {
	struct sem_scope *scope = sem->current_scope;
	struct sem_variable *result = 0;
	int i;

	while (scope) {
		for (i = 0; i < scope->variables.count; i++) {
			if (strcmp(name, scope->variables.data[i]->name) == 0) {
				if (result) {
					printf("conflict\n");
				}
				result = scope->variables.data[i];
			}

		}
		if (result)
			return result;

		scope = scope->parent_scope;
	}
	return 0;
}

static struct sem_function *get_function(struct semantics *sem, char *name) {
	struct sem_scope *scope = sem->current_scope;
	struct sem_function *result = 0;
	int i;

	while (scope) {
		for (i = 0; i < scope->functions.count; i++) {
			if (strcmp(name, scope->functions.data[i]->name) == 0) {
				if (result) {
					printf("conflict\n");
				}
				result = scope->functions.data[i];
			}

		}
		if (result)
			return result;

		scope = scope->parent_scope;
	}
	return 0;
}

static struct sem_variable *generate_variable(struct semantics *sem, char *name, char *type) {
	struct sem_variable *result = memalloc(struct sem_variable, 1);

	result->type = SEM_VARIABLE;
	result->name = name;
	result->datatype = get_type(sem, type);
	return result;
}

static struct sem_function *generate_simple_function(struct semantics *sem, char *name, char *returntype) {
	struct sem_function *result = memalloc(struct sem_function, 1);

	result->type = SEM_FUNCTION;

	vector_init(&result->args, 2);
	result->name = name;
	if (returntype)
		result->result_type = get_type(sem, returntype);

	vector_append(&sem->current_scope->functions, &result);
	return result;
}

static struct sem_function *generate_function(struct semantics *sem, struct ast_node *func) {
	struct sem_function *result = memalloc(struct sem_function, 1);
	char *argname;
	char *argtype;
	int i;

	result->type = SEM_FUNCTION;

	vector_init(&result->args, 2);
	result->name = ((struct SimpleToken *)func->childs[0]->childs[0])->value;
	result->result_type = 0;

	for (i = 1; i < func->childs[0]->childcount; i++) {
		argname = ((struct SimpleToken *)func->childs[0]->childs[i]->childs[1])->value;
		argtype = ((struct SimpleToken *)func->childs[0]->childs[i]->childs[0])->value;
		*vector_append(&result->args, 0) = generate_variable(sem, argname, argtype);
	}

	vector_append(&sem->current_scope->functions, &result);
	return result;
}

static struct sem_type *generate_basic_type(enum sem_data_types type, char *name, int size) {
	struct sem_type *result = memalloc(struct sem_type, 1);

	result->type = SEM_TYPE;
	result->datatype = type;
	result->name = name;
	result->size = size;
	result->fields.count = 0;
	return result;
}

static struct sem_scope *generate_scope(struct semantics *sem, char *name) {
	struct sem_scope *result = memalloc(struct sem_scope, 1);

	result->type = SEM_SCOPE;
	result->name = name;
	result->parent_scope = sem->current_scope;

	vector_init(&result->types, 2);
	vector_init(&result->variables, 2);
	vector_init(&result->functions, 2);

	sem->current_scope = result;
	return result;
}

static int sem_fp_function_definition(struct semantics *sem, struct ast_node *node, int pass) {
	int i;

	if (pass == PASS_FUNCS) {
		node->sem_val = generate_function(sem, node);
		node->childs[1]->sem_val = generate_scope(sem,
				((struct SimpleToken *)node->childs[0]->childs[0])->value);
	}
	sem->current_scope = node->childs[1]->sem_val;
	for (i = 0; i < node->childs[1]->childcount; i++) {
		sem_pass(sem, node->childs[1]->childs[i], pass);
	}
	return 1;
}

static int sem_fp_struct_definition(struct semantics *sem, struct ast_node *node, int pass) {
	struct sem_type *type;
	int i;
	int size;

	if (pass == PASS_TYPES) {
		type = generate_basic_type(SEM_STRUCT,
				((struct SimpleToken *)node->childs[0])->value, -1);
		vector_init(&type->fields, node->childcount);

		node->sem_val = type;
		vector_append_value(&sem->current_scope->types, type);
	}
	else if (pass == PASS_FIELDS) {
		type = node->sem_val;
		size = 0;

		for (i = 1; i < node->childcount; i++) {
			struct sem_variable *field = generate_variable(sem,
						((struct SimpleToken *)node->childs[i]->childs[1])->value,
						((struct SimpleToken *)node->childs[i]->childs[0])->value
						);
			vector_append_value(&type->fields, field);
			if (field->datatype)
				size += field->datatype->size;
		}

		type->size = size;
	}
	return 1;
}

static int sem_functioncall(struct semantics *sem, struct ast_node *node, int pass) {
	if (pass == PASS_IDENTIFIERS) {
		node->sem_val = get_function(sem, ((struct SimpleToken *)node->childs[1])->value);
	}
	return 0;
}

static int sem_sp_variable_declaration(struct semantics *sem, struct ast_node *node, int pass) {
	if (pass != PASS_VARS)
		return 0;

	char *name = ((struct SimpleToken *)node->childs[1])->value;
	char *type = ((struct SimpleToken *)node->childs[0])->value;

	struct sem_variable *var = generate_variable(sem, name, type);
	*vector_append(&sem->current_scope->variables, 0) = var;
	return 0;
}

static int sem_identifier(struct semantics *sem, struct ast_node *node, int pass) {
	if (pass == PASS_IDENTIFIERS) {
		struct sem_variable *var = get_variable(sem, ((struct SimpleToken *)node)->value);
		node->sem_val = var;
	}
	return 0;
}

static struct sem_scope *generate_global_scope(struct semantics *sem) {
	struct sem_scope *result;

	sem->current_scope = 0;
	result = generate_scope(sem, "global_scope");

	vector_append_value(&result->types, generate_basic_type(SEM_INTEGER, "int", 4));
	vector_append_value(&result->types, generate_basic_type(SEM_INTEGER, "char", 1));

	struct sem_function *print = generate_simple_function(sem, "print", 0);
	vector_append_value(&print->args, generate_variable(sem, "i", "int"));
	return result;
}

int run_semantical_analyzer(struct ast_node *ast) {
	struct semantics sem;
	int i;

	sem.ast = ast;
	sem.global_scope = generate_global_scope(&sem);
	sem.current_scope = sem.global_scope;
	ast->sem_val = sem.global_scope;

	for (i = 0; i < PASS_END; i++)
		sem_pass(&sem, ast, i);
	return 0;
}

void init_semantical_analyzer(void) {
	memset(passes, 0, sizeof(passes));
	passes[TOKEN_IDENTIFIER] = sem_identifier;

	passes[FUNCTION_DEFINITION] = sem_fp_function_definition;
	passes[STRUCT_DEFINITION] = sem_fp_struct_definition;
	
	passes[VARIABLE_DECLARATION] = sem_sp_variable_declaration;
	passes[FUNCTION_CALL] = sem_functioncall;
}

#ifdef TEST_SEM

#include <stdio.h>

static void print_var(struct sem_variable *var) {
	printf("{ VAR \"%s\"", var->name);

	if (var->datatype != NULL)
		printf(" TYPE \"%s\" }", var->datatype ? var->datatype->name : 0);
	else
		printf(" }");
}

static void print_var_vector(struct sem_var_vector *v, int level) {
	int i, y;
	
	printf("%c", v->count > 4 ? '\n' : ' ');
	if (v->count > 4) {
		for (y = 0; y < level; y++) printf("\t");
	}

	for (i = 0; i < v->count; i++) {
		print_var(v->data[i]);

		if (i + 1 >= v->count)
			break;

		if (v->count > 4) {
			printf("\n");
			for (y = 0; y < level; y++) printf("\t");
		} else {
			printf(", ");
		}
	}
	if (v->count > 4) {
		printf("\n");
		for (y = 0; y < level - 1; y++) printf("\t");
	}
}

static void print_type(struct sem_type *type, int level) {
	static char *types[] = { "INTEGER", "FLOATING", "STRUCT"};
	int i;
	
	printf("\t{ TYPE \"%s\" %s (size %d) ",
			type->name, types[type->datatype], type->size);
	if (type->datatype == SEM_STRUCT) {
		print_var_vector(&type->fields, level + 1);
	}
	printf("}\n");
	for (i = 0; i < level - 1; i++) printf("\t");
}

static void print_function(struct sem_function *func, int level) {
	int i;

	printf("{ FUNC \"%s\" RETURNTYPE \"%s\"", func->name, func->result_type ?
			func->result_type->name : 0);
	if (func->args.count) {
		printf("\n");
		for (i = 0; i < level + 1; i++) printf("\t");
		printf("Args:");
		print_var_vector(&func->args, level + 1);
		printf("\n");
		for (i = 0; i < level; i++) printf("\t");
	}
	else printf(" ");
	printf("}\n");
	for (i = 0; i < level; i++) printf("\t");
}

static void print_scope(struct sem_scope *scope, int level) {
	int i;

	for (i = 0; i < level; i++) printf("\t");

	printf("{ SCOPE %s (parent %s):\n", scope->name, scope->parent_scope ?
			scope->parent_scope->name : 0);
	level++;
	for (i = 0; i < level - 1; i++) printf("\t");

	if (scope->types.count) {
		printf("\tTypes:\n");
		for (i = 0; i < level - 1; i++) printf("\t");
		for (i = 0; i < scope->types.count; i++) {
			print_type(scope->types.data[i], level);
		}
		printf("\n");
		for (i = 0; i < level - 1; i++) printf("\t");
	}

	if (scope->functions.count) {
		printf("\tFunctions:\n");
		for (i = 0; i < level; i++) printf("\t");
		for (i = 0; i < scope->functions.count; i++) {
			print_function(scope->functions.data[i], level);
		}
		printf("\n");
		for (i = 0; i < level - 1; i++) printf("\t");
	}

	if (scope->variables.count) {
		printf("\tVars:");
		print_var_vector(&scope->variables, level);
		printf("\n");
		for (i = 0; i < level - 1; i++) printf("\t");
	}
	level--;
	printf("}\n");
}

static void print_sem(void *node, int level) {
	if (node == NULL)
		return;
	switch (((struct sem_variable *)node)->type) {
		case SEM_SCOPE:
			print_scope((struct sem_scope *)node, level);
			break;
		case SEM_FUNCTION:
			while (level--) printf("\t");
			printf("{ FUNCTION \"%s\" }\n", (((struct sem_function *)node)->name));
			break;
		case SEM_VARIABLE:
			print_var((struct sem_variable *)node);
			break;
		case SEM_TYPE:
			break;
	}
}

void print_node(struct ast_node *node, int level) {
	int i;

	for (i = 0; i < level; i++)
		printf("\t");
	if (node->type >= TOKEN_LAST) {
		printf("[%s,\n", all_names[node->type]);
		i = 0;
		while (i < node->childcount) {
			print_node(node->childs[i], level + 1);
			i++;
		}
		if (node->sem_val != NULL) {
			print_sem(node->sem_val, level + 1);
		}
		for (i = 0; i < level; i++)
			printf("\t");
		printf("]\n");
	}
	else {
		printf("[%s, '%s'", all_names[node->type], ((struct SimpleToken *)node)->value);
		if (node->sem_val) {
			printf(" ");
			print_sem(node->sem_val, level + 1);
			printf(" ");
		}
		printf("]\n");
	}
}

int main(int argc, char **argv) {
	struct SimpleToken tokens[300];
	char *source;
	struct ast_node *node;
	int i;
	
	(void) argc;
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 300);
	tokenize(source, tokens);
	for (i = 0; i < 300 && tokens[i].type; i++) {
		printf("%s (%d): %s (%d)\n", all_names[tokens[i].type], tokens[i].type, tokens[i].value, tokens[i].line);
	}
	node = parse(tokens);
	init_semantical_analyzer();
	run_semantical_analyzer(node);
	print_node(node, 0);
}

#endif
