NAME=tim

SRCS=src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/interpreter/interpreter_nodes.c \
src/interpreter/interpreter.c \
src/utils.c \
src/semantics/semantics.c

.PHONY: all fclean clean tests re
all: $(NAME)

$(NAME): $(SRCS) timinterpreter timparser
	gcc $(SRCS) -g -D TEST_INTERPRETER -o $(NAME) -Wall -Wextra

timinterpreter: $(SRCS)
	gcc $(SRCS) -g -D TEST_INTERPRETER -o timinterpreter -Wall -Wextra

timparser: $(SRCS)
	gcc $(SRCS) -g -D TEST_SEM -o timparser -Wall -Wextra

clean:

fclean: clean
	rm -f $(NAME) timinterpreter timparser

re: fclean all

tests: re
	cd tests; ./tests.sh
