#ifndef CGENERATOR_H
#define CGENERATOR_H

#include "../semantics/semantics.h"
#include <stdio.h>

struct cgen {
	struct ast_node *ast;
	FILE *file;
	int indentlevel;

	int is_expression_inline;
};

void compile(struct ast_node *node);

#endif
