NAME=tim

SRCS=src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/interpreter/interpreter_nodes.c \
src/interpreter/interpreter.c 

.PHONY: all fclean clean tests re
all: $(NAME)

$(NAME):
	gcc $(SRCS) -g -D TEST_INTERPRETER -o $(NAME) -Wall -Wextra

clean:

fclean: clean
	rm $(NAME)

re: fclean all

tests: re
	cd tests; ./tests.sh
