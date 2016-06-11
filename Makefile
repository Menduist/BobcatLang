NAME=bob

SRCS=src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/interpreter/interpreter_nodes.c \
src/interpreter/interpreter.c \
src/utils.c \
src/vector.c \
src/semantics/semantics.c \
src/semantics/sem_typefinder.c \
src/cgenerator/cgenerator.c

.PHONY: all fclean clean tests re
all: $(NAME)

$(NAME): $(SRCS) bobinterpreter bobparser bobcompiler
	gcc $(SRCS) -g -rdynamic -D TEST_INTERPRETER -o $(NAME) -Wall -Wextra

bobinterpreter: $(SRCS)
	gcc $(SRCS) -g -rdynamic -D TEST_INTERPRETER -o bobinterpreter -Wall -Wextra

bobparser: $(SRCS)
	gcc $(SRCS) -g -rdynamic -D TEST_SEM -o bobparser -Wall -Wextra

bobcompiler: $(SRCS)
	gcc $(SRCS) -g -rdynamic -D TEST_CGEN -o bobcompiler -Wall -Wextra

clean:

fclean: clean
	rm -f $(NAME) bobinterpreter bobparser bobcompiler

re: fclean all

tests: re
	cd tests; ./tests.sh
