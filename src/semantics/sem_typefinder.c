#include "semantics.h"
#include <string.h>
#include <ctype.h>

typedef struct sem_type *(*node_typefinder)(struct semantics *, struct ast_node *);

node_typefinder finders[NODES_END];

static struct sem_type *finder(struct semantics *sem, struct ast_node *node) {
	int i;
	struct sem_type *result;

	if (finders[node->type] != NULL) {
		result = finders[node->type](sem, node);
		if (result != 0) {
			return result;
		}
	}
	if (node->type < TOKEN_LAST)
		goto end;

	for (i = 0; i < node->childcount; i++) {
		result = finder(sem, node->childs[i]);
		if (result)
			return result;
	}
end:
	return 0;
}


static struct sem_type *finder_identifier(struct semantics *sem, struct ast_node *node) {
	struct SimpleToken *tok = (struct SimpleToken *)node;
	struct sem_variable *var;

	if (isdigit(tok->value[0]))
		return get_type(sem, "int");
	
	var = get_variable(sem, tok->value);
	if (var)
		return var->datatype;
	return 0;
}

struct sem_type *find_node_type(struct semantics *sem, struct ast_node *node) {
	return finder(sem, node);
}


void init_sem_typefinder(void) {
	memset(finders, 0, sizeof(finders));
	finders[TOKEN_IDENTIFIER] = finder_identifier;
}

