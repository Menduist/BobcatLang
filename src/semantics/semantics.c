#include "semantics.h"
#include <string.h>
#include "../utils.h"

typedef void (*node_semanticalizer)(struct semantics *, struct ast_node *);

node_semanticalizer first_pass[NODES_END];
node_semanticalizer second_pass[NODES_END];
node_semanticalizer third_pass[NODES_END];

static struct sem_scope *generate_scope(struct semantics *sem) {
	struct sem_scope *result = memalloc(struct sem_scope, 1);

	result->type = SEM_SCOPE;
	result->parent_scope = sem->current_scope;

	vector_init(&result->types, 2);
	vector_init(&result->variables, 2);
	vector_init(&result->functions, 2);

	sem->current_scope = result;
	return result;
}

static void sem_fp_function_definition(struct semantics *sem, struct ast_node *node) {
	node->sem_val = generate_scope(sem);
}

static void sem_fp_variable_declaration(struct semantics *sem, struct ast_node *node) {
	struct sem_variable *result = memalloc(struct sem_variable, 1);

	result->type = SEM_VARIABLE;
	result->name = ((struct SimpleToken *)node->childs[1])->value;
	
	vector_append(&sem->current_scope->variables, &result);
}

/*
** Detect types and generate scopes
*/
static int sem_first_pass(struct semantics *sem, struct ast_node *node) {
	int i;

	if (first_pass[node->type] != NULL) {
		first_pass[node->type](sem, node);
	}
	if (node->type < TOKEN_LAST)
		return 0;

	for (i = 0; i < node->childcount; i++) {
		sem_first_pass(sem, node->childs[i]);
	}
	return 0;
}

static struct sem_scope *generate_global_scope(struct semantics *sem) {
	return generate_scope(sem);
}

int run_semantical_analyzer(struct ast_node *ast) {
	struct semantics sem;

	sem.ast = ast;
	sem.global_scope = generate_global_scope(&sem);
	sem.current_scope = sem.global_scope;
	sem_first_pass(&sem, ast);
	return 0;
}

void init_semantical_analyzer(void) {
	memset(first_pass, 0, sizeof(first_pass));
	first_pass[FUNCTION_DEFINITION] = sem_fp_function_definition;
	first_pass[VARIABLE_DECLARATION] = sem_fp_variable_declaration;
}

#ifdef TEST_SEM

#include <stdio.h>

static void print_var(struct sem_variable *var) {
	printf("{ VAR \"%s\" TYPE \"%s\" }", var->name, var->datatype ?
			var->datatype->name : 0);
}

static void print_scope(struct sem_scope *scope, int level) {
	int i;

	for (i = 0; i < level; i++) printf("\t");
	printf("{ SCOPE %p (parent %p)\n", scope, scope->parent_scope);
	for (i = 0; i < level; i++) printf("\t");

	if (scope->types.count) {
		for (i = 0; i < level; i++) printf("\t");
		printf("Types:\n");
		for (i = 0; i < scope->types.count; i++) {
			//print_type(scope->types.data[i], level + 1);
		}
	}

	if (scope->variables.count) {
		printf("Vars:%c", scope->variables.count > 4 ? '\n' : ' ');
		for (i = 0; i < scope->variables.count; i++) {
			print_var(scope->variables.data[i]);

			if (i + 1 < scope->variables.count)
				break;

			if (scope->variables.count > 4) {
				printf("\n");
				for (i = 0; i < level; i++) printf("\t");
			} else {
				printf(", ");
			}
		}
		printf("\n");
	}
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
			break;
		case SEM_VARIABLE:
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
		while (node->childs[i] && i < node->childcount) {
			print_node(node->childs[i], level + 1);
			i++;
		}
		if (node->sem_val != NULL) {
			//print_sem(node->sem_val, level);
		}
		for (i = 0; i < level; i++)
			printf("\t");
		printf("]\n");
	}
	else {
		printf("[%s, '%s']\n", all_names[node->type], ((struct SimpleToken *)node)->value);
	}
}

int main(int argc, char **argv) {
	struct SimpleToken tokens[300];
	char *source;
	struct ast_node *node;
	int i;
	
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 100);
	tokenize(source, tokens);
	for (i = 0; i < 100 && tokens[i].type; i++) {
		printf("%s (%d): %s (%d)\n", all_names[tokens[i].type], tokens[i].type, tokens[i].value, tokens[i].line);
	}
	node = parse(tokens);
	init_semantical_analyzer();
	run_semantical_analyzer(node);
	print_node(node, 0);
}

#endif
