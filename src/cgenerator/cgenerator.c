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

	fputs(func->gendata, cgen->file);
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
		if (cgen->is_expression_inline == 0) indent(cgen);
		cgen_pass(cgen, node->childs[i], pass);
		if (cgen->is_expression_inline == 0) fputs(";\n", cgen->file);
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
	fputs(func->gendata, cgen->file);

	fputs("(", cgen->file);
	
	for (i = 0; i < func->args.count; i++) {
		fputs(func->args.data[i]->datatype->name, cgen->file);
		fputs(" ", cgen->file);
		fputs(func->args.data[i]->name, cgen->file);
		if (i + 1 != func->args.count)
			fputs(",", cgen->file);
	}
	fputs(") ", cgen->file);

	cgen_pass(cgen, node->childs[1], pass);
	indent(cgen);
	fputs("\n", cgen->file);
	return 1;
}

static int cgen_identifier(struct cgen *cgen, struct ast_node *node, int pass) {
	if (node->sem_val)
		fputs((char *)((struct sem_variable *)node->sem_val)->gendata, cgen->file);
	else
		fputs(((struct SimpleToken *)node)->value, cgen->file);
	(void) pass;
	return 0;
}

static int cgen_if(struct cgen *cgen, struct ast_node *node, int pass) {
	indent(cgen);
	fputs("if (", cgen->file);

	cgen->is_expression_inline = 1;
	cgen_pass(cgen, node->childs[1], pass);
	cgen->is_expression_inline = 0;
	fputs(") ", cgen->file);
	cgen_pass(cgen, node->childs[2], pass);

	if (node->childcount > 3) {
		indent(cgen);
		fputs("else ", cgen->file);
		cgen_pass(cgen, node->childs[3], pass);
	}
	return 1;
}

static int cgen_while(struct cgen *cgen, struct ast_node *node, int pass) {
	indent(cgen);
	fputs("while (", cgen->file);

	cgen->is_expression_inline = 1;
	cgen_pass(cgen, node->childs[1], pass);
	cgen->is_expression_inline = 0;
	fputs(") ", cgen->file);
	cgen_pass(cgen, node->childs[2], pass);
	return 1;
}

static int cgen_operator(struct cgen *cgen, struct ast_node *node, int pass) {
	cgen_pass(cgen, node->childs[1], pass);
	fputs(" ", cgen->file);
	fputs(((struct SimpleToken *)node->childs[0])->value, cgen->file);
	fputs(" ", cgen->file);
	cgen_pass(cgen, node->childs[2], pass);
	return 1;
}

static int cgen_prefix_operator(struct cgen *cgen, struct ast_node *node, int pass) {
	fputs(((struct SimpleToken *)node->childs[0])->value, cgen->file);
	(void) pass;
	return 0;
}

static char *mangle_var(struct cgen *cgen, struct sem_variable *tomangle) {
	(void) cgen;
	tomangle->gendata = tomangle->name;
	return tomangle->gendata;
}

static char *mangle_func(struct cgen *cgen, struct sem_function *tomangle) {
	(void) cgen;
	if (strcmp(tomangle->name, "main") == 0)
		tomangle->gendata = "inner_main";
	else {
		char result[1000];
		int length;
		int i;

		length = snprintf(result, 1000, "_F%lu%s", strlen(tomangle->name), tomangle->name);
		for (i = 0; i < tomangle->args.count; i++) {
			length += snprintf(result + length, 1000 - length, "%lu%s", strlen(tomangle->args.data[i]->datatype->name),
					tomangle->args.data[i]->datatype->name);
		}
		tomangle->gendata = strdup(result);
	}
	return tomangle->gendata;
}

static void init_scope(struct cgen *cgen, struct sem_scope *scope) {
	int i, y;

	for (i = 0; i < scope->functions.count; i++) {
		mangle_func(cgen, scope->functions.data[i]);

		indent(cgen);
		if (scope->functions.data[i]->result_type == 0)
			fputs("void", cgen->file);
		else
			fputs(scope->functions.data[i]->result_type->name, cgen->file);
		fputs(" ", cgen->file);

		fputs(scope->functions.data[i]->gendata, cgen->file);

		fputs("(", cgen->file);

		for (y = 0; y < scope->functions.data[i]->args.count; y++) {
			fputs(scope->functions.data[i]->args.data[y]->datatype->name, cgen->file);
			if (y + 1 != scope->functions.data[i]->args.count)
				fputs(",", cgen->file);
		}

		fputs(");\n", cgen->file);
	}

	for (i = 0; i < scope->variables.count; i++) {
		mangle_var(cgen, scope->variables.data[i]);

		indent(cgen);
		fputs(scope->variables.data[i]->datatype->name, cgen->file);
		fputs(" ", cgen->file);
		fputs(scope->variables.data[i]->gendata, cgen->file);
		fputs(" = 0;\n", cgen->file);
	}
	if (scope->variables.count > 0)
		fputs("\n", cgen->file);
}

static int cgen_compound_statement(struct cgen *cgen, struct ast_node *node, int pass) {
	struct sem_scope *scope;
	int i;

	fputs("{\n", cgen->file);
	cgen->indentlevel++;

	scope = node->sem_val;
	init_scope(cgen, scope);

	for (i = 0; i < node->childcount; i++)
		cgen_pass(cgen, node->childs[i], pass);

	cgen->indentlevel--;
	indent(cgen);
	fputs("}\n", cgen->file);
	return 1;
}

static int cgen_translation_unit(struct cgen *cgen, struct ast_node *node, int pass) {
	int i;

	init_scope(cgen, node->sem_val);
	fputs("void toplevel(void) {\n", cgen->file);
	cgen->indentlevel++;

	for (i = 0; i < node->childcount; i++) {
		if (node->childs[i]->type != FUNCTION_DEFINITION)
			cgen_pass(cgen, node->childs[i], pass);
	}

	fputs("}\n\n", cgen->file);
	cgen->indentlevel--;

	for (i = 0; i < node->childcount; i++) {
		if (node->childs[i]->type == FUNCTION_DEFINITION)
			cgen_pass(cgen, node->childs[i], pass);
	}

	fputs("int main() {\n\ttoplevel();\n\tinner_main();\n}\n\n", cgen->file);
	(void) pass;
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
	fputs("typedef char * string;\n", gen->file);
	fputs("void _F6prints6string(string s) {\nprintf(\"%s\", s);\n}\n\n", gen->file);
	fputs("void _F6printi3int(int i) {\nprintf(\"%d\\n\", i);\n}\n\n", gen->file);
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
	passes[TRANSLATION_UNIT] = cgen_translation_unit;
	passes[PREFIX_OPERATOR] = cgen_prefix_operator;

	passes[VARIABLE_DECLARATION] = cgen_nop;
	passes[IF_STATEMENT] = cgen_if;
	passes[WHILE_STATEMENT] = cgen_while;
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
