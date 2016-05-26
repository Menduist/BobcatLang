#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

struct ast_node *parse_statement(struct parser *parser) {
	parser->tokens++;
	struct ast_node *result = calloc(sizeof(struct ast_node), 1);
	return 0;
}

struct ast_node *parse_compound_statement(struct parser *parser) {
	struct ast_node *result = calloc(sizeof(struct ast_node), 1);
	int i = 0;

	result->type = COMPOUND_STATEMENT;
	if (parser->tokens->type != TOKEN_BLOCK_START) {
		printf("excepted '{', got %d\n", parser->tokens->type);
	}
	parser->tokens++;
	while (parser->tokens->type != TOKEN_BLOCK_END) {
		result->childs[i++] = parse_statement(parser);
	}
	parser->tokens++;
	return result;
}

struct ast_node *parse_declarator(struct parser *parser) {
	struct ast_node *result = calloc(sizeof(struct ast_node), 1);

	result->type = DECLARATOR;
	if (parser->tokens->type != TOKEN_IDENTIFIER) {
		printf("excepted identifier\n");
	}
	result->childs[0] = (struct ast_node *)parser->tokens;
	parser->tokens += 3;
	return result;
}

struct ast_node *parse_function_declaration(struct parser *parser) {
	struct ast_node *result = calloc(sizeof(struct ast_node), 1);

	result->type = FUNCTION_DEFINITION;
	parser->tokens++;

	result->childs[0] = parse_declarator(parser);
	result->childs[1] = parse_compound_statement(parser);
		printf("lolol %d \n", result->childs[0]->type);

	return result;
}

struct ast_node *parse_translation_unit(struct parser *parser) {
	struct ast_node *result = calloc(sizeof(struct ast_node), 1);
	int i = 0;

	result->type = TRANSLATION_UNIT;
	parser->root = result;
	while (parser->tokens->type == TOKEN_FUNC) {
		result->childs[i++] = parse_function_declaration(parser);
	}
	return result;
}

struct ast_node *parse(struct SimpleToken *tokens) {
	struct parser parser;

	parser.tokens = tokens;
	parse_translation_unit(&parser);
	return parser.root;
}

#define TEST_PARSER
#ifdef TEST_PARSER

void print_node(struct ast_node *node, int level) {
	int i;

	for (i = 0; i < level; i++)
		printf("\t");
	if (node->type >= TOKEN_LAST) {
		printf("[%d,\n", node->type);
		i = 0;
		while (node->childs[i]) {
			print_node(node->childs[i], level + 1);
			i++;
		}
		for (i = 0; i < level; i++)
			printf("\t");
		printf("]\n");
	}
	else {
		printf("[%d, '%s']\n", node->type, ((struct SimpleToken *)node)->value);
	}
}

int main() {
	struct SimpleToken tokens[30];
	struct ast_node *node;
	int i;
	
	memset(tokens, 0, sizeof(struct SimpleToken) * 30);
	tokenize("func main() { echo(\"Hello World!\") }", tokens);
	for (i = 0; i < 30; i++) {
		printf("%d: %s\n", tokens[i].type, tokens[i].value);
	}
	node = parse(tokens);
	print_node(node, 0);
}

#endif
