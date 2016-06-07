#include "cgenerator.h"
#include <stdlib.h>
#include <string.h>
#include "../utils.h"

enum passes {
	PASS_ALL,
	PASS_END
};

typedef int (*node_cgen)(struct cgen *, struct ast_node *, int pass);

static void indent(struct cgen *cgen) {
	int i;

	for (i = 0; i < cgen->indentlevel; i++) fputc('\t', cgen->file);
}

node_cgen passes[NODES_END];

static int cgen_pass(struct cgen *cgen, struct ast_node *node, int pass) {
	int i;

	if (passes[node->type] != NULL) {
		if (passes[node->type](cgen, node, pass) != 0) {
			goto end;
		}
	}
	if (node->type < TOKEN_LAST)
		goto end;

	for (i = 0; i < node->childcount; i++) {
		cgen_pass(cgen, node->childs[i], pass);
	}
end:
	return 0;
}

static int cgen_function_call(struct cgen *cgen, struct ast_node *node, int pass) {
	int i;
	struct sem_function *func = node->sem_val;

	fputs(func->name, cgen->file);
	fputs("(", cgen->file);
	for (i = 0; i < node->childs[2]->childcount; i++) {
		cgen_pass(cgen, node->childs[2]->childs[i], pass);
	}
	fputs(")", cgen->file);
	return 1;
}

static int cgen_expression_list(struct cgen *cgen, struct ast_node *node, int pass) {
	int i;

	for (i = 0; i < node->childcount; i++) {
		indent(cgen);
		cgen_pass(cgen, node->childs[i], pass);
		fputs(";\n", cgen->file);
	}
	return 1;
}

static int cgen_string_literal(struct cgen *cgen, struct ast_node *node, int pass) {
	if (pass == PASS_ALL)
		fputs(((struct SimpleToken*)node)->value, cgen->file);
	return 0;
}

static int cgen_function_declaration(struct cgen *cgen, struct ast_node *node, int pass) {
	struct sem_function *func = node->sem_val;
	int i;
	
	if (func->result_type == NULL) {
		fputs("void ", cgen->file);
	}
	fputs(func->name, cgen->file);
	fputs("(", cgen->file);
	
	for (i = 1; i < node->childs[0]->childcount; i++) {

	}
	fputs(") ", cgen->file);

	cgen_pass(cgen, node->childs[1], pass);
	return 1;
}

static int cgen_identifier(struct cgen *cgen, struct ast_node *node, int pass) {
	fputs(((struct SimpleToken *)node)->value, cgen->file);
	(void) pass;
	return 0;
}

static int cgen_operator(struct cgen *cgen, struct ast_node *node, int pass) {
	cgen_pass(cgen, node->childs[1], pass);
	fputs(" ", cgen->file);
	fputs(((struct SimpleToken *)node->childs[0])->value, cgen->file);
	fputs(" ", cgen->file);
	cgen_pass(cgen, node->childs[2], pass);
	return 1;
}

static int cgen_compound_statement(struct cgen *cgen, struct ast_node *node, int pass) {
	struct sem_scope *scope;
	int i;

	fputs("{\n", cgen->file);
	cgen->indentlevel++;

	scope = node->sem_val;
	for (i = 0; i < scope->variables.count; i++) {
		indent(cgen);
		fputs(scope->variables.data[i]->datatype->name, cgen->file);
		fputs(" ", cgen->file);
		fputs(scope->variables.data[i]->name, cgen->file);
		fputs(";\n", cgen->file);
	}
	fputs("\n", cgen->file);

	for (i = 0; i < node->childcount; i++)
		cgen_pass(cgen, node->childs[i], pass);

	cgen->indentlevel--;
	fputs("}\n", cgen->file);
	return 1;
}

static int cgen_nop(struct cgen *cgen, struct ast_node *node, int pass) {
	(void) cgen;
	(void) node;
	(void) pass;
	return 1;
}

static void put_header(struct cgen *gen) {
	fputs("#include <stdio.h>\n", gen->file);
	fputs("void print(char *s) {\nprintf(\"%s\", s);\n}\n\n", gen->file);
}

void compile(struct ast_node *node) {
	struct cgen gen;
	int i;

	memset(&gen, 0, sizeof(struct cgen));
	gen.ast = node;
	gen.file = fopen(".generated.c", "w+");
	put_header(&gen);

	for (i = 0; i < PASS_END; i++)
		cgen_pass(&gen, node, i);

	fclose(gen.file);

	system("cc .generated.c");
}

void init_cgenerator(void) {
	memset(passes, 0, sizeof(passes));

	passes[FUNCTION_DEFINITION] = cgen_function_declaration;
	passes[FUNCTION_CALL] = cgen_function_call;
	passes[EXPRESSION_LIST] = cgen_expression_list;
	passes[TOKEN_STRING_LITERAL] = cgen_string_literal;
	passes[COMPOUND_STATEMENT] = cgen_compound_statement;
	passes[OPERATOR] = cgen_operator;
	passes[TOKEN_IDENTIFIER] = cgen_identifier;

	passes[VARIABLE_DECLARATION] = cgen_nop;
}

#ifdef TEST_CGEN

int main(int argc, char **argv) {
	struct SimpleToken tokens[300];
	char *source;
	struct ast_node *node;
	
	if (argc < 2) {
		printf("usage %s [file]\n", argv[0]);
		return 1;
	}
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 100);
	tokenize(source, tokens);
	node = parse(tokens);
	init_semantical_analyzer();
	run_semantical_analyzer(node);
	init_cgenerator();
	compile(node);
}

#endif
