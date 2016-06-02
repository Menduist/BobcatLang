NAME=tim

SRCS=src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/interpreter/interpreter_nodes.c \
src/interpreter/interpreter.c 

.PHONY: all fclean clean tests
all: $(NAME)

$(NAME):
	gcc $(SRCS) -D TEST_INTERPRETER -o $(NAME) -Wall -Wextra

clean:

fclean: clean
	rm $(NAME)

tests:
	cd tests; ./tests.sh
