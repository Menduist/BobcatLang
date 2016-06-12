#include "bob.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include "interpreter/interpreter.h"
#include "cgenerator/cgenerator.h"

void usage(char *prog) {
	printf("Usage: %s [MODE] [OPTION]... MAINFILE\n\n", prog);

	printf("Modes:\n");
	printf("  tokenizer  Tokenize the input file and print the resulint tokens\n");
	printf("  parse      Parse the input file and print the resulint AST\n");
	printf("  semantics  Semantic analyze the input file and print the resulint AST\n");
	printf("  interpret  Interpret the input file\n");
	printf("  compile    Compile the input file\n");
	printf("  execute    Compile and execute the input file\n\n");

	printf("Options:\n");
	printf("  -o         Set the compiling output file\n");
	printf("  --help     Show this message and exit\n");
	exit(EXIT_SUCCESS);
}

int parse_args(struct bob *bob, int argc, char **argv) {
	int i, y;

	if (argc < 2)
		usage(argv[0]);
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp(argv[i], "-o") == 0) {
				i++;
			}
			else if (strcmp(argv[i], "--help") == 0) {
				usage(argv[i]);
			}
		}
		else if (strlen(argv[i]) >= 4 && strcmp(strlen(argv[i]) - 4 + argv[i], ".bob") == 0) {
			bob->file = argv[i];
		}
		else {
			y = 0;
			while (y < MODE_END) {
				if (strncmp(argv[i], bob_compile_mode_names[y], strlen(argv[i])) == 0) {
					if (bob->mode != MODE_NONE) {
						fprintf(stderr, "ERROR: two modes: %s and %s\n", bob_compile_mode_names[bob->mode],
								argv[i]);
						usage(argv[0]);
					}
					bob->mode = y;
				}
				y++;
			}
		}
	}
	if (bob->mode == MODE_NONE)
		bob->mode = MODE_EXECUTE;
	fprintf(stderr, "mode %s\n", bob_compile_mode_names[bob->mode]);
	return 0;
}

int main(int argc, char **argv) {
	struct bob bob;
	struct SimpleToken tokens[300];
	struct ast_node *node;
	int i;
	
	memset(&bob, 0, sizeof(struct bob));
	parse_args(&bob, argc, argv);

	bob.source = readfile(bob.file);
	memset(tokens, 0, sizeof(struct SimpleToken) * 300);

	init_semantical_analyzer();
	init_cgenerator();
	if (bob.mode >= MODE_TOKENIZE) {
		tokenize(bob.source, tokens);
		
		if (bob.mode >= MODE_PARSE) {
			node = parse(tokens);

			if (bob.mode >= MODE_SEM) {
				run_semantical_analyzer(node);
				
				if (bob.mode == MODE_INTERPRET) {
					interpret(node);
				}
				else if (bob.mode >= MODE_COMPILE) {
					compile(node);

					if (bob.mode >= MODE_EXECUTE) {
						fflush(stdout);
						system("./a.out");
					}
				}
				else {
					print_node(node, 0);
				}
			}
			else {
				print_node(node, 0);
			}
		}
		else {
			for (i = 0; i < 300; i++) {
				if (!tokens[i].type)
					break;
				printf("%s (%d): %s (%d)\n", all_names[tokens[i].type], tokens[i].type,
						tokens[i].value, tokens[i].line);
			}
		}
	}

	return 0;
}
