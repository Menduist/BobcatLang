#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../utils.h"

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

struct SimpleToken *consume_token(struct parser *parser) {
	parser->last_token = parser->token;
	if (parser->peek != NULL) {
		parser->token = parser->peek;
		parser->peek = 0;
	}
	else {
		parser->token = get_next_token(&parser->tokenizer);
	}
	return parser->last_token;
}

void ignore_token(struct parser *parser) {
	/*free(*/consume_token(parser)/*)*/;
}

struct SimpleToken *peek_token(struct parser *parser) {
	if (!parser->token)
		parser->token = get_next_token(&parser->tokenizer);
	if (!parser->peek)
		parser->peek = get_next_token(&parser->tokenizer);
	return parser->peek;
}

static struct ast_node *new_node() {
	struct ast_node *result = calloc(sizeof(struct ast_node) + sizeof(struct ast_node *) * 4, 1);
	if (result == NULL)
		return allocfailed(sizeof(struct ast_node) + sizeof(struct ast_node *) * 4, 1);
	result->childsize = 4;
	return result;
}

static void add_child(struct ast_node **pnode, struct ast_node *child) {
	struct ast_node *node = *pnode;
	if (node->childcount + 1 == node->childsize) {
		node->childsize *= 2;
		node = realloc(node, sizeof(struct ast_node) + sizeof(struct ast_node *) * node->childsize);
		if (node == NULL) {
			(void) allocfailed(-1, 1);
			return;
		}
	}
	node->childs[node->childcount++] = child;
	*pnode = node;
}

static void unexcepted(char *excepted, struct SimpleToken *got) {
	printf("Excepted %s, got '%s' at %d:%d\n", excepted, got->value, got->line, got->col);
	print_backtrace();
	exit(EXIT_FAILURE);
}

static struct ast_node *parse_expression(struct parser *parser);
static struct ast_node *parse_expression_list(struct parser *parser);
static struct ast_node *parse_primary(struct parser *parser);
static struct ast_node *parse_statement(struct parser *parser);

static struct ast_node *parse_parentheses_expression(struct parser *parser) {
	struct ast_node *result;

	if (parser->token->type != TOKEN_PARENTHESE_START) {
		unexcepted("'('", parser->token);
	}
	ignore_token(parser);
	result = parse_expression_list(parser);

	if (parser->token->type != TOKEN_PARENTHESE_END) {
		unexcepted("')'", parser->token);
	}
	ignore_token(parser);
	return result;
}

static struct ast_node *parse_function_arguments(struct parser *parser) {
	struct ast_node *result;

	result = new_node();
	result->type = FUNCTION_ARG_LIST;

	while (parser->token->type != TOKEN_PARENTHESE_END) {
		add_child(&result, parse_expression(parser));

		if (parser->token->type == TOKEN_COMMA) {
			ignore_token(parser);
		}
	}
	if (parser->token->type != TOKEN_PARENTHESE_END) {
		unexcepted("')'", parser->token);
	}
	ignore_token(parser);
	return result;
}

static struct ast_node *parse_variable_multiple_declaration(struct parser *parser) {
	struct ast_node *result;

	result = new_node();

	result->type = VARIABLE_DECLARATION;
	add_child(&result, (struct ast_node *)consume_token(parser));

	do {
		if (parser->token->type == TOKEN_COMMA)
			ignore_token(parser);
		if (parser->token->type != TOKEN_IDENTIFIER) {
			unexcepted("identifier", parser->token);
		}
		if (peek_token(parser)->type == TOKEN_OPERATOR) {
			struct ast_node *op = new_node();
			struct ast_node *name = (struct ast_node *)consume_token(parser);
			op->type = OPERATOR;
			add_child(&op, (struct ast_node *)consume_token(parser));
			add_child(&op, (struct ast_node *)name);

			add_child(&op, parse_expression(parser));
			add_child(&result, op);
		}
		else {
			add_child(&result, (struct ast_node *)consume_token(parser));
		}
	} while (parser->token->type == TOKEN_COMMA);
	return result;
}

static struct ast_node *parse_variable_declaration(struct parser *parser) {
	struct ast_node *result;

	result = new_node();

	result->type = VARIABLE_DECLARATION;
	add_child(&result, (struct ast_node *)consume_token(parser));

	if (parser->token->type != TOKEN_IDENTIFIER) {
		unexcepted("identifier", parser->token);
	}
	if (peek_token(parser)->type == TOKEN_OPERATOR) {
		struct ast_node *op = new_node();
		struct ast_node *name = (struct ast_node *)consume_token(parser);
		op->type = OPERATOR;
		add_child(&op, (struct ast_node *)consume_token(parser));
		add_child(&op, (struct ast_node *)name);

		add_child(&op, parse_expression(parser));
		add_child(&result, op);
	}
	else {
		add_child(&result, (struct ast_node *)consume_token(parser));
	}
	return result;
}

static struct ast_node *parse_identifier(struct parser *parser) {
	struct ast_node *result;

	if (parser->token->type != TOKEN_IDENTIFIER) {
		unexcepted("identifier", parser->token);
		result = 0;
	}
	else {
		result = (struct ast_node *)consume_token(parser);
	}
	return result;
}

static struct ast_node *parse_expression_1(struct parser *parser, struct ast_node *lhs, int min_precedence);

static struct ast_node *parse_prefix_operator(struct parser *parser) {
	struct ast_node *result;

	result = new_node();
	result->type = PREFIX_OPERATOR;
	add_child(&result, (struct ast_node *)consume_token(parser));
	add_child(&result, parse_expression_1(parser, parse_primary(parser), 3001));
	return result;
}

static struct ast_node *parse_suffix_operator(struct parser *parser, struct ast_node *lhs) {
	struct ast_node *result;
	struct SimpleToken *op;

	result = new_node();
	op = consume_token(parser);
	
	add_child(&result, (struct ast_node *)op);
	add_child(&result, lhs);
	switch (op->value[0]) {
		case '.':
			result->type = OPERATOR;
			add_child(&result, parse_primary(parser));
			break;
		case '(':
			result->type = FUNCTION_CALL;
			add_child(&result, parse_function_arguments(parser));
			break;
		case '[':
			result->type = OPERATOR;
			add_child(&result, parse_expression(parser));
			if (parser->token->value[0] != ']')
				unexcepted("]", parser->token);
			else
				ignore_token(parser);
			break;
		default:
			unexcepted(".([", parser->token);
			break;
	}
	return result;
}

static int is_suffix_operator(struct SimpleToken *tok) {
	if (strcmp(tok->value, "[") == 0) return 1;
	if (strcmp(tok->value, "(") == 0) return 1;
	if (strcmp(tok->value, ".") == 0) return 1;
	return 0;
}

static int can_be_primary(struct SimpleToken *tok) {
	switch (tok->type) {
		case TOKEN_IDENTIFIER:
		case TOKEN_STRING_LITERAL:
		case TOKEN_PARENTHESE_START:
		case TOKEN_OPERATOR:
			return 1;
		default:
			return 0;
	}
}

static struct ast_node *parse_primary(struct parser *parser) {
	struct ast_node *result;
	switch (parser->token->type) {
		case TOKEN_IDENTIFIER:
			result = parse_identifier(parser);
			break;
		case TOKEN_STRING_LITERAL:
			result = (struct ast_node *)consume_token(parser);
			break;
		case TOKEN_PARENTHESE_START:
			result = parse_parentheses_expression(parser);
			break;
		case TOKEN_OPERATOR:
			result = parse_prefix_operator(parser);
			break;
		default:
			unexcepted("identifier,string_literal,(,operator", parser->token);
			return 0;
	}
	while (is_suffix_operator(parser->token) != 0)
		result = parse_suffix_operator(parser, result);
	return result;
}

static int get_token_precedence(struct SimpleToken *tok) {
	if (tok->type != TOKEN_OPERATOR || tok->value[0] == ']')
		return -1;
	if (tok->value[0] == '.' || tok->value[0] == '[')
		return 4000;
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

static struct ast_node *parse_expression_1(struct parser *parser, struct ast_node *lhs, int min_precedence) {
	int tokprec;
	struct ast_node *operator;
	struct ast_node *rhs;

	while (1) {
		tokprec = get_token_precedence(parser->token);

		if (parser->last_token->line != parser->token->line) {
			return lhs;
		}

		if (tokprec < min_precedence)
			return lhs;

		operator = new_node();
		operator->type = OPERATOR;
		add_child(&operator, (struct ast_node *)parser->token);

		if (consume_token(parser)->value[0] == '[') {
			rhs = parse_expression(parser);
			if (parser->token->value[0] != ']') {
				printf("excepted ']'\n");
			}
			ignore_token(parser);
		}
		else
			rhs = parse_primary(parser);

		if (tokprec < get_token_precedence(parser->token)) {
			rhs = parse_expression_1(parser, rhs, tokprec + 1);
		}

		add_child(&operator, lhs);
		add_child(&operator, rhs);
		lhs = operator;
	}
	return 0;
}

static struct ast_node *parse_expression(struct parser *parser) {
	struct ast_node *result = parse_expression_1(parser, parse_primary(parser), 0);
	return result;
}

static struct ast_node *parse_iteration_statement(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = WHILE_STATEMENT;
	add_child(&result, (struct ast_node *)consume_token(parser));

	if (parser->token->type == TOKEN_PARENTHESE_START)
		ignore_token(parser);
	add_child(&result, parse_expression_list(parser));
	if (parser->token->type == TOKEN_PARENTHESE_END)
		ignore_token(parser);

	add_child(&result, parse_statement(parser));
	return result;
}

static struct ast_node *parse_selection_statement(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = IF_STATEMENT;
	add_child(&result, (struct ast_node *)consume_token(parser));
	add_child(&result, parse_expression_list(parser));
	add_child(&result, parse_statement(parser));

	if (parser->token->type == TOKEN_ELSE) {
		ignore_token(parser);
		add_child(&result, parse_statement(parser));
	}
	return result;
}

static struct ast_node *parse_compound_statement(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = COMPOUND_STATEMENT;
	if (parser->token->type != TOKEN_BLOCK_START) {
		unexcepted("'{'", parser->token);
	}
	ignore_token(parser);
	while (parser->token->type != TOKEN_BLOCK_END) {
		add_child(&result, parse_statement(parser));
	}
	ignore_token(parser);
	return result;
}

static struct ast_node *parse_jump_statement(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = JUMP_STATEMENT;
	add_child(&result, (struct ast_node *)parser->token);
	if (consume_token(parser)->type== TOKEN_RETURN) {
		if (can_be_primary(parser->token) && parser->token->line == ((struct SimpleToken *)result->childs[0])->line)
			add_child(&result, parse_expression(parser));
	}
	return result;
}

static struct ast_node *parse_expression_list(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = EXPRESSION_LIST;
	add_child(&result, parse_expression(parser));
	while (parser->token->type == TOKEN_COMMA) {
		ignore_token(parser);
		add_child(&result, parse_expression(parser));
	}
	return result;
}

static struct ast_node *parse_expression_statement(struct parser *parser) {
	struct ast_node *result;
	if (parser->token->type == TOKEN_SEMICOLON) {
		return (struct ast_node *)consume_token(parser);
	}
	
	result = parse_expression_list(parser);
	if (parser->token->type == TOKEN_SEMICOLON) {
		ignore_token(parser);
	}
	return result;
}

static struct ast_node *parse_statement(struct parser *parser) {
	switch (parser->token->type) {
	case TOKEN_IF:
		return parse_selection_statement(parser);
	case TOKEN_WHILE:
		return parse_iteration_statement(parser);
	case TOKEN_BLOCK_START:
		return parse_compound_statement(parser);
	case TOKEN_RETURN:
	case TOKEN_BREAK:
	case TOKEN_CONTINUE:
		return parse_jump_statement(parser);
	case TOKEN_IDENTIFIER:
		if (peek_token(parser)->type == TOKEN_IDENTIFIER)
			return parse_variable_multiple_declaration(parser);
	}
	return parse_expression_statement(parser);
}

static struct ast_node *parse_declarator(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = DECLARATOR;
	if (parser->token->type != TOKEN_IDENTIFIER) {
		printf("excepted identifier\n");
	}
	add_child(&result, (struct ast_node *)consume_token(parser));

	if (parser->token->type != TOKEN_PARENTHESE_START) {
		unexcepted("'('", parser->token);
	}
	ignore_token(parser);

	while (parser->token->type != TOKEN_PARENTHESE_END) {
		add_child(&result, parse_variable_declaration(parser));

		if (parser->token->type == TOKEN_COMMA)
			ignore_token(parser);
		else
			break;
	}
	if (parser->token->type != TOKEN_PARENTHESE_END) {
		unexcepted("')'", parser->token);
	}
	ignore_token(parser);
	return result;
}

static struct ast_node *parse_struct_declaration(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = STRUCT_DEFINITION;
	ignore_token(parser);

	if (parser->token->type != TOKEN_IDENTIFIER) {
		unexcepted("identifier", parser->token);
	}
	add_child(&result, (struct ast_node *)consume_token(parser));
	if (parser->token->type != TOKEN_BLOCK_START) {
		unexcepted("{", parser->token);
	}
	ignore_token(parser);
	while (parser->token->type != TOKEN_BLOCK_END && parser->token->type != TOKEN_NONE) {
		add_child(&result, parse_variable_multiple_declaration(parser));
	}
	if (parser->token->type != TOKEN_BLOCK_END) {
		unexcepted("}", parser->token);
	}
	ignore_token(parser);
	return result;
}

static struct ast_node *parse_function_declaration(struct parser *parser) {
	struct ast_node *result = new_node();
	struct ast_node *returntype = 0;

	result->type = FUNCTION_DEFINITION;
	ignore_token(parser);

	add_child(&result, parse_declarator(parser));
	
	if (parser->token->type != TOKEN_BLOCK_START) {
		returntype = (struct ast_node *)consume_token(parser);
	}

	add_child(&result, parse_compound_statement(parser));
	if (returntype)
		add_child(&result, returntype);

	return result;
}

static struct ast_node *parse_translation_unit(struct parser *parser) {
	struct ast_node *result = new_node();

	result->type = TRANSLATION_UNIT;
	parser->root = result;
	while (parser->token->type != TOKEN_NONE) {
		switch (parser->token->type) {
			case TOKEN_FUNC:
				add_child(&result, parse_function_declaration(parser));
				break;
			case TOKEN_STRUCT:
				add_child(&result, parse_struct_declaration(parser));
				break;
			default:
				add_child(&result, parse_statement(parser));
		}
		parser->root = result;
	}
	if (parser->token->type != TOKEN_NONE) {
		unexcepted("EOF", parser->token);		
	}
	return result;
}

struct ast_node *parse(const char *code) {
	struct parser parser;

	init_tokenizer(&parser.tokenizer, code);
	parser.last_token = parser.token = parser.peek = 0;
	peek_token(&parser);
	parse_translation_unit(&parser);
	return parser.root;
}

char *all_names[] = {
	"TOKEN_NONE",
	"TOKEN_COMMENT",
	"TOKEN_FUNC",
	"TOKEN_STRUCT",
	"TOKEN_IDENTIFIER",
	"TOKEN_OPERATOR",
	"TOKEN_IF",
	"TOKEN_ELSE",
	"TOKEN_WHILE",
	"TOKEN_BLOCK_START",
	"TOKEN_BLOCK_END",
	"TOKEN_PARENTHESE_START",
	"TOKEN_PARENTHESE_END",
	"TOKEN_STRING_LITERAL",
	"TOKEN_COLON",
	"TOKEN_COMMA",
	"TOKEN_SEMICOLON",
	"TOKEN_EXPRESSION_END",
	"TOKEN_RETURN",
	"TOKEN_BREAK",
	"TOKEN_CONTINUE",
	"TRANSLATION_UNIT",
	"EXTERN_DECLARATION",
	"STRUCT_DEFINITION",
	"FUNCTION_DEFINITION",
	"FUNCTION_CALL",
	"FUNCTION_ARG_LIST",
	"VARIABLE_DECLARATION",
	"DECLARATOR",
	"EXPRESSION_LIST",
	"COMPOUND_STATEMENT",
	"OPERATOR",
	"PREFIX_OPERATOR",
	"POSTFIX_OPERATOR",
	"IF_STATEMENT",
	"WHILE_STATEMENT",
	"JUMP_STATEMENT",
	"NODES_END"
};

