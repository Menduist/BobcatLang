#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*

expression
: primary
;

expression_statement
: expression
| ';'
;

statement
: expression_statement
;

compound_statement
: TOKEN_BLOCK_START statement_list TOKEN_BLOCK_END
| TOKEN_BLOCK_START TOKEN_BLOCK_END
;

declarator
: TOKEN_IDENTIFIER TOKEN_PARENTHESES_START TOKEN_PARENTHSES_END
;


function_declaration
: TOKEN_FUNC declarator compound_statement
;

translation_unit
: function_declaration
| translation_unit function_declaration
;

*/

void unexcepted(char *excepted, struct SimpleToken *got) {
	printf("Excepted %s, got '%s' at %d:%d\n", excepted, got->value, got->line, got->col);
}

struct ast_node *parse_expression(struct parser *parser);
struct ast_node *parse_primary(struct parser *parser);

struct ast_node *parse_parentheses_expression(struct parser *parser) {
	struct ast_node *result;

	if (parser->tokens->type != TOKEN_PARENTHESE_START) {
		unexcepted("'('", parser->tokens);
	}
	parser->tokens++;
	result = parse_expression(parser);

	if (parser->tokens->type != TOKEN_PARENTHESE_END) {
		unexcepted("')'", parser->tokens);
	}
	parser->tokens++;
	return result;
}

struct ast_node *parse_function_call(struct parser *parser) {
	struct ast_node *result;
	int i;

	result = calloc(sizeof(struct ast_node), 1);
	result->type = FUNCTION_CALL;
	if (parser->tokens->type != TOKEN_IDENTIFIER) {
		unexcepted("identifier", parser->tokens);
	}

	result->childs[0] = (struct ast_node *)parser->tokens++;
	if (parser->tokens->type != TOKEN_PARENTHESE_START) {
		unexcepted("'('", parser->tokens);
	}

	parser->tokens++;

	i = 1;
	while (parser->tokens->type != TOKEN_PARENTHESE_END) {
		result->childs[i++] = parse_expression(parser);

		if (parser->tokens->type == TOKEN_COMMA) {
			parser->tokens++;
		}
	}
	parser->tokens++;
	return result;
}

struct ast_node *parse_identifier(struct parser *parser) {
	struct ast_node *result;

	if (parser->tokens->type != TOKEN_IDENTIFIER) {
		unexcepted("identifier", parser->tokens);
	}
	if (parser->tokens[1].type == TOKEN_PARENTHESE_START) {
		result = parse_function_call(parser);
	}
	else {
		result = (struct ast_node *)parser->tokens++;
	}
	return result;
}

struct ast_node *parse_prefix_operator(struct parser *parser) {
	struct ast_node *result;

	result = calloc(sizeof(struct ast_node), 1);
	result->type = PREFIX_OPERATOR;
	result->childs[0] = (struct ast_node *)parser->tokens++;
	result->childs[1] = parse_primary(parser);
	return result;
}

struct ast_node *parse_postfix_operator(struct parser *parser, struct ast_node *child) {
	struct ast_node *result;

	result = calloc(sizeof(struct ast_node), 1);
	result->type = POSTFIX_OPERATOR;
	result->childs[0] = (struct ast_node *)parser->tokens++;
	result->childs[1] = child; 
	return result;
}

struct ast_node *parse_primary(struct parser *parser) {
	struct ast_node *result;
	switch (parser->tokens->type) {
		case TOKEN_IDENTIFIER:
			result = parse_identifier(parser);
			break;
		case TOKEN_STRING_LITERAL:
			result = parser->tokens++;
			break;
		case TOKEN_PARENTHESE_START:
			result = parse_parentheses_expression(parser);
			break;
		case TOKEN_OPERATOR:
			result = parse_prefix_operator(parser);
			break;
		default:
			unexcepted("identifier,string_literal,(,operator", parser->tokens);
			return 0;
	}
	if (parser->tokens->type == TOKEN_OPERATOR && parser->tokens[1].type == TOKEN_PARENTHESE_END) {
		result = parse_postfix_operator(parser, result);
	}
	return result;
}

int get_token_precedence(struct SimpleToken *tok) {
	if (tok->type != TOKEN_OPERATOR)
		return -1;
	if (tok->value[0] == '*' || tok->value[0] == '/')
		return 3000;
	if (tok->value[0] == '+' || tok->value[0] == '-')
		return 2000;
	if (strchr("&|^", tok->value[0]) != 0)
		return 30;
	if (strlen(tok->value) == 2)
		return 10;
	return 2;
}

struct ast_node *parse_expression_1(struct parser *parser, struct ast_node *lhs, int min_precedence) {
	int tokprec;
	struct ast_node *operator;
	struct ast_node *rhs;

	while (1) {
		tokprec = get_token_precedence(parser->tokens);

		if (parser->tokens[-1].line != parser->tokens->line) {
			return lhs;
		}

		if (tokprec < min_precedence)
			return lhs;

		operator = calloc(sizeof(struct ast_node), 1);
		operator->type = OPERATOR;
		operator->childs[0] = (struct ast_node *)parser->tokens;

		parser->tokens++;

		rhs = parse_primary(parser);

		if (tokprec < get_token_precedence(parser->tokens)) {
			rhs = parse_expression_1(parser, rhs, tokprec + 1);
		}

		operator->childs[1] = lhs;
		operator->childs[2] = rhs;
		lhs = operator;
	}
	return 0;
}

struct ast_node *parse_expression(struct parser *parser) {
	struct ast_node *result = parse_expression_1(parser, parse_primary(parser), 0);
	return result;
}

struct ast_node *parse_statement(struct parser *parser) {
	return parse_expression(parser);
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
	if (parser->tokens->type != TOKEN_NONE) {
		unexcepted("EOF", parser->tokens);		
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

char *lol[] = {
	"TOKEN_NONE",
	"TOKEN_FUNC",
	"TOKEN_IDENTIFIER",
	"TOKEN_OPERATOR",
	"TOKEN_INT",
	"TOKEN_BLOCK_START",
	"TOKEN_BLOCK_END",
	"TOKEN_PARENTHESE_START",
	"TOKEN_PARENTHESE_END",
	"TOKEN_STRING_LITERAL",
	"TOKEN_COLON",
	"TOKEN_COMMA",
	"TOKEN_EXPRESSION_END",
	"TRANSLATION_UNIT",
	"EXTERN_DECLARATION",
	"FUNCTION_DEFINITION",
	"FUNCTION_CALL",
	"DECLARATOR",
	"COMPOUND_STATEMENT",
	"OPERATOR",
	"PREFIX_OPERATOR",
	"POSTFIX_OPERATOR"
};

void print_node(struct ast_node *node, int level) {
	int i;

	for (i = 0; i < level; i++)
		printf("\t");
	if (node->type >= TOKEN_LAST) {
		printf("[%s,\n", lol[node->type]);
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
		printf("[%s, '%s']\n", lol[node->type], ((struct SimpleToken *)node)->value);
	}
}

char *readfile(char *path) {
	char *target;
	FILE *source;
	int size;
	
	source = fopen(path, "r");
	fseek(source, 0, SEEK_END);
	size = ftell(source);
	target = malloc(sizeof(char) * (size + 1));
	fseek(source, 0, SEEK_SET);
	fread(target, 1, size, source);
	target[size] = '\0';
	return target;
}

int main(int argc, char **argv) {
	struct SimpleToken tokens[100];
	char *source;
	struct ast_node *node;
	int i;
	
	source = readfile(argv[1]);
	memset(tokens, 0, sizeof(struct SimpleToken) * 100);
	tokenize(source, tokens);
	for (i = 0; i < 30 && tokens[i].type; i++) {
		printf("%s (%d): %s (%d)\n", lol[tokens[i].type], tokens[i].type, tokens[i].value, tokens[i].line);
	}
	node = parse(tokens);
	print_node(node, 0);
}

#endif
