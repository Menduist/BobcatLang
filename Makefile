NAME=bob

SRCS=src/bob.c \
src/parser/parser.c \
src/tokenizer/tokenizer.c \
src/utils.c \
src/vector.c \
src/semantics/semantics.c \
src/semantics/sem_typefinder.c \
src/cgenerator/cgenerator.c

.PHONY: all fclean clean tests re
all: $(NAME)

$(NAME): $(SRCS)
	gcc $(SRCS) -g -rdynamic -o $(NAME) -Wall -Wextra

clean:

fclean: clean
	rm -f $(NAME)

re: fclean all

tests: re
	cd tests; ./tests.sh
