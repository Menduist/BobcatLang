#include "semantics.h"
#include <string.h>

typedef void (*node_semanticalizer)(struct semantics *, struct ast_node *);

node_semanticalizer first_pass[NODES_END];
node_semanticalizer second_pass[NODES_END];
node_semanticalizer third_pass[NODES_END];

/*
** Detect types and generate scopes
*/
static int sem_first_pass(struct semantics *sem, struct ast_node *node) {
	int i;

	if (first_pass[node->type] != NULL) {
		first_pass[node->type](sem, node);
	}
	for (i = 0; i < node->childcount; i++) {
		sem_first_pass(sem, node->childs[i]);
	}
	return 0;
}

static struct sem_scope *generate_global_scope() {
	return 0;
}

int run_semantical_analyzer(struct ast_node *ast) {
	struct semantics sem;

	sem.ast = ast;
	sem.global_scope = generate_global_scope();
	sem.current_scope = sem.global_scope;
	sem_first_pass(&sem, ast);
	return 0;
}

void init_semantical_analyzer(void) {
	memset(first_pass, 0, sizeof(first_pass));
}

#ifdef TEST_SEM

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
	print_node(node, 0);
}

#endif
